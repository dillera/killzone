/**
 * KillZone Network Module - Atari 8-bit
 * Real FujiNet HTTP communication with on-device JSON parsing
 */

#ifdef _CMOC_VERSION_
#include <cmoc.h>
#include <coco.h>
#include "snprintf.h"
#include "conio_wrapper.h"
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif
#include "network.h"
#include "state.h"
#include "fujinet-network.h"

static network_status_t current_status = NET_DISCONNECTED;
static char device_spec[DEVICE_SPEC_SIZE];
static char path_buf[256];
static char body_buf[256];
static char value_buf[256]; /* Buffer for JSON values */
static char query_buf[64];  /* Buffer for JSON keys/paths */

static void build_device_spec(const char *path) {
    snprintf(device_spec, DEVICE_SPEC_SIZE, "N:HTTP://%s:%d%s", SERVER_HOST, SERVER_PORT, path);
}

/* Helper to query an integer value */
static uint8_t query_int(const char *query, uint32_t *val) {
    if (network_json_query(device_spec, (char *)query, value_buf) != FN_ERR_IO_ERROR) {
        *val = (uint32_t)strtoul(value_buf, NULL, 10);
        return 1;
    }
    return 0;
}

/* Helper to query a string value */
static uint8_t query_string(const char *query, char *dest, size_t max_len) {
    if (network_json_query(device_spec, (char *)query, value_buf) != FN_ERR_IO_ERROR) {
        strncpy(dest, value_buf, max_len);
        dest[max_len - 1] = '\0';
        return 1;
    }
    return 0;
}

/* Helper to query a boolean value */
static uint8_t query_bool(const char *query, uint8_t *val) {
    if (network_json_query(device_spec, (char *)query, value_buf) != FN_ERR_IO_ERROR) {
        if (strcmp(value_buf, "true") == 0) {
            *val = 1;
        } else {
            *val = 0;
        }
        return 1;
    }
    return 0;
}

uint8_t kz_network_init(void) {
    uint8_t err;
    err = network_init();
    if (err != FN_ERR_OK) {
        current_status = NET_ERROR;
        return err;
    }
    current_status = NET_CONNECTED;
    return 0;
}

uint8_t kz_network_close(void) {
    current_status = NET_DISCONNECTED;
    return 0;
}

network_status_t kz_network_get_status(void) {
    return current_status;
}

uint8_t kz_network_health_check(void) {
    uint8_t err;
    char status[16];
    
    if (current_status != NET_CONNECTED) return 0;
    
    build_device_spec("/api/health");
    
    err = network_open(device_spec, OPEN_MODE_HTTP_GET, OPEN_TRANS_NONE);
    if (err != FN_ERR_OK) return 0;
    
    err = network_json_parse(device_spec);
    if (err != FN_ERR_OK) {
        network_close(device_spec);
        return 0;
    }

    if (!query_string("/status", status, sizeof(status))) {
        network_close(device_spec);
        return 0;
    }
    
    network_close(device_spec);

    return (strcmp(status, "healthy") == 0);
}

uint8_t kz_network_join_player(const char *name, player_state_t *player) {
    uint8_t err;
    uint32_t val;
    
    if (current_status != NET_CONNECTED) return 0;
    
    build_device_spec("/api/player/join");
    err = network_open(device_spec, OPEN_MODE_HTTP_POST, OPEN_TRANS_NONE);
    if (err != FN_ERR_OK) return 0;
    
    /* Add headers */
    network_http_start_add_headers(device_spec);
    network_http_add_header(device_spec, "Content-Type: application/json");
    network_http_end_add_headers(device_spec);
    
    /* Send body */
    snprintf(body_buf, sizeof(body_buf), "{\"name\":\"%s\"}", name);
    network_http_post(device_spec, body_buf);
    
    /* Parse response */
    err = network_json_parse(device_spec);
    if (err != FN_ERR_OK) {
        network_close(device_spec);
        return 0;
    }
    
    /* Check success */
    if (query_string("/success", value_buf, sizeof(value_buf))) {
        if (strcmp(value_buf, "false") == 0) {
            network_close(device_spec);
            return 0;
        }
    }
    
    /* Extract player data */
    query_string("/id", player->id, sizeof(player->id));
    if (!query_string("/name", player->name, sizeof(player->name))) {
        strncpy(player->name, player->id, sizeof(player->name));
    }
    
    if (query_int("/x", &val)) player->x = (uint8_t)val;
    if (query_int("/y", &val)) player->y = (uint8_t)val;
    if (query_int("/health", &val)) player->health = (uint8_t)val;
    else player->health = 100;
    
    strcpy(player->status, "alive");
    strcpy(player->type, "player");
    player->isHunter = 0;
    
    /* Update local state immediately */
    state_set_local_player(player);
    
    network_close(device_spec);
    return 1;
}

/* Helper to parse a single player from the array */
static void parse_single_player(int index, player_state_t *player) {
    uint32_t val;
    uint8_t bval;
    
    snprintf(query_buf, sizeof(query_buf), "/players/%d/x", index);
    if (query_int(query_buf, &val)) player->x = (uint8_t)val;
    
    snprintf(query_buf, sizeof(query_buf), "/players/%d/y", index);
    if (query_int(query_buf, &val)) player->y = (uint8_t)val;
    
    snprintf(query_buf, sizeof(query_buf), "/players/%d/type", index);
    if (!query_string(query_buf, player->type, sizeof(player->type))) {
        strcpy(player->type, "mob");
    }
    
    snprintf(query_buf, sizeof(query_buf), "/players/%d/isHunter", index);
    if (query_bool(query_buf, &bval)) {
        player->isHunter = bval;
    } else {
        player->isHunter = 0;
    }
    
    player->health = 100;
    strcpy(player->status, "alive");
}

static player_state_t other_players[MAX_OTHER_PLAYERS];
static char id_buf[32];

uint8_t kz_network_get_world_state(void) {
    uint8_t err;
    uint32_t width, height, ticks;
    uint8_t count = 0;
    const player_state_t *local = state_get_local_player();
    int i;
    
    if (current_status != NET_CONNECTED) return 0;
    
    build_device_spec("/api/world/state");
    err = network_open(device_spec, OPEN_MODE_HTTP_GET, OPEN_TRANS_NONE);
    if (err != FN_ERR_OK) return 0;
    err = network_json_parse(device_spec);
    if (err != FN_ERR_OK) {
        network_close(device_spec);
        return 0;
    }
    
    /* Global properties */
    if (query_int("/width", &width) && query_int("/height", &height)) {
        state_set_world_dimensions((uint8_t)width, (uint8_t)height);
    }
    
    if (query_int("/ticks", &ticks)) {
        state_set_world_ticks((uint16_t)ticks);
    }
    
    /* Players array */
    /* We iterate until we fail to find an ID or reach MAX */
    for (i = 0; count < MAX_OTHER_PLAYERS; i++) {
        snprintf(query_buf, sizeof(query_buf), "/players/%d/id", i);
        if (!query_string(query_buf, id_buf, sizeof(id_buf))) {
            break; /* No more players */
        }
        
        /* Skip local player */
        if (local && strcmp(id_buf, local->id) == 0) {
            continue;
        }
        
        strncpy(other_players[count].id, id_buf, sizeof(other_players[count].id));
        parse_single_player(i, &other_players[count]);
        count++;
    }
    
    state_set_other_players(other_players, count);
    
    network_close(device_spec);
    return 1;
}

uint8_t kz_network_get_player_status(const char *player_id, player_state_t *player) {
    uint8_t err;
    uint32_t val;
    
    if (current_status != NET_CONNECTED) return 0;
    
    snprintf(path_buf, sizeof(path_buf), "/api/player/%s/status", player_id);
    build_device_spec(path_buf);
    
    err = network_open(device_spec, OPEN_MODE_HTTP_GET, OPEN_TRANS_NONE);
    if (err != FN_ERR_OK) return 0;
    
    err = network_json_parse(device_spec);
    if (err != FN_ERR_OK) {
        network_close(device_spec);
        return 0;
    }
    
    /* Extract fields */
    strncpy(player->id, player_id, sizeof(player->id));
    
    if (query_int("/x", &val)) player->x = (uint8_t)val;
    if (query_int("/y", &val)) player->y = (uint8_t)val;
    if (query_int("/health", &val)) player->health = (uint8_t)val;
    
    query_string("/status", player->status, sizeof(player->status));
    
    network_close(device_spec);
    return 1;
}

uint8_t kz_network_move_player(const char *player_id, const char *direction, move_result_t *result) {
    uint8_t err;
    uint32_t val;
    uint8_t bval;
    int i;
    
    if (current_status != NET_CONNECTED) return 0;
    
    snprintf(path_buf, sizeof(path_buf), "/api/player/%s/move", player_id);
    build_device_spec(path_buf);
    
    err = network_open(device_spec, OPEN_MODE_HTTP_POST, OPEN_TRANS_NONE);
    if (err != FN_ERR_OK) return 0;
    
    /* Headers */
    network_http_start_add_headers(device_spec);
    network_http_add_header(device_spec, "Content-Type: application/json");
    network_http_end_add_headers(device_spec);
    
    /* Body */
    snprintf(body_buf, sizeof(body_buf), "{\"direction\":\"%s\"}", direction);
    network_http_post(device_spec, body_buf);
    
    /* Parse response */
    err = network_json_parse(device_spec);
    if (err != FN_ERR_OK) {
        network_close(device_spec);
        return 0;
    }
    
    /* Check for error (e.g. player not found) */
    if (query_string("/error", value_buf, sizeof(value_buf))) {
        if (strcmp(value_buf, "Player not found") == 0) {
            /* Signal disconnect */
            state_set_connected(0);
        }
        network_close(device_spec);
        return 0;
    }
    
    /* Parse result */
    if (query_int("/x", &val)) result->x = (uint8_t)val;
    if (query_int("/y", &val)) result->y = (uint8_t)val;
    
    if (query_bool("/collision", &bval)) result->collision = bval;
    else result->collision = 0;
    
    result->message_count = 0;
    
    if (result->collision) {
        /* Parse messages */
        for (i = 0; i < 4; i++) {
            snprintf(query_buf, sizeof(query_buf), "/messages/%d", i);
            if (query_string(query_buf, result->messages[i], sizeof(result->messages[i]))) {
                result->message_count++;
            } else {
                break;
            }
        }
        
        /* Parse loser ID */
        if (!query_string("/finalLoserId", result->loser_id, sizeof(result->loser_id))) {
            result->loser_id[0] = '\0';
        }
    }
    
    network_close(device_spec);
    return 1;
}

uint8_t kz_network_leave_player(const char *player_id) {
    uint8_t err;
    
    if (current_status != NET_CONNECTED) return 0;
    
    build_device_spec("/api/player/leave");
    
    err = network_open(device_spec, OPEN_MODE_HTTP_POST, OPEN_TRANS_NONE);
    if (err != FN_ERR_OK) return 0;
    
    network_http_start_add_headers(device_spec);
    network_http_add_header(device_spec, "Content-Type: application/json");
    network_http_end_add_headers(device_spec);
    
    snprintf(body_buf, sizeof(body_buf), "{\"id\":\"%s\"}", player_id);
    network_http_post(device_spec, body_buf);
    
    /* We don't strictly need to parse the response for leave */
    
    network_close(device_spec);
    return 1;
}