/**
 * KillZone Network Module - Atari 8-bit
 * Real FujiNet HTTP communication
 */

#include "network.h"
#include <stdio.h>
#include <string.h>
#include "fujinet-network.h"

static network_status_t current_status = NET_DISCONNECTED;
static char device_spec[DEVICE_SPEC_SIZE];
static char path_buf[256];
static char body_buf[256];

static void build_device_spec(const char *path) {
    snprintf(device_spec, DEVICE_SPEC_SIZE, "N:HTTP://%s:%d%s", SERVER_HOST, SERVER_PORT, path);
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

int16_t kz_network_http_get(const char *path, uint8_t *response, uint16_t response_len) {
    uint8_t err;
    int16_t bytes_read;
    int retry;
    
    if (current_status != NET_CONNECTED) {
        return -1;
    }
    
    build_device_spec(path);
    
    err = network_open(device_spec, OPEN_MODE_HTTP_GET, OPEN_TRANS_NONE);
    if (err != FN_ERR_OK) {
        return -1;
    }
    
    bytes_read = network_read_nb(device_spec, response, response_len - 1);
    
    retry = 0;
    while (bytes_read == 0 && retry < 10) {
        bytes_read = network_read_nb(device_spec, response, response_len - 1);
        retry++;
    }
    
    if (bytes_read > 0) {
        response[bytes_read] = 0;
    }
    
    network_close(device_spec);
    return bytes_read;
}

int16_t kz_network_http_post(const char *path, const char *body, uint8_t *response, uint16_t response_len) {
    uint8_t err;
    int16_t bytes_read;
    int retry;
    
    if (current_status != NET_CONNECTED) {
        return -1;
    }
    
    build_device_spec(path);
    
    err = network_open(device_spec, OPEN_MODE_HTTP_POST, OPEN_TRANS_NONE);
    if (err != FN_ERR_OK) {
        return -1;
    }
    
    /* Add Content-Type header */
    err = network_http_start_add_headers(device_spec);
    if (err != FN_ERR_OK) {
        network_close(device_spec);
        return -1;
    }
    
    err = network_http_add_header(device_spec, "Content-Type: application/json");
    if (err != FN_ERR_OK) {
        network_close(device_spec);
        return -1;
    }
    
    err = network_http_end_add_headers(device_spec);
    if (err != FN_ERR_OK) {
        network_close(device_spec);
        return -1;
    }
    
    err = network_http_post(device_spec, (char *)body);
    if (err != FN_ERR_OK) {
        network_close(device_spec);
        return -1;
    }
    
    bytes_read = network_read_nb(device_spec, response, response_len - 1);
    
    retry = 0;
    while (bytes_read == 0 && retry < 10) {
        bytes_read = network_read_nb(device_spec, response, response_len - 1);
        retry++;
    }
    
    if (bytes_read > 0) {
        response[bytes_read] = 0;
    }
    
    network_close(device_spec);
    return bytes_read;
}

int16_t kz_network_health_check(uint8_t *response, uint16_t response_len) {
    return kz_network_http_get("/api/health", response, response_len);
}

int16_t kz_network_join_player(const char *name, uint8_t *response, uint16_t response_len) {
    snprintf(body_buf, sizeof(body_buf), "{\"name\":\"%s\"}", name);
    return kz_network_http_post("/api/player/join", body_buf, response, response_len);
}

int16_t kz_network_get_world_state(uint8_t *response, uint16_t response_len) {
    return kz_network_http_get("/api/world/state", response, response_len);
}

int16_t kz_network_get_player_status(const char *player_id, uint8_t *response, uint16_t response_len) {
    snprintf(path_buf, sizeof(path_buf), "/api/player/%s/status", player_id);
    return kz_network_http_get(path_buf, response, response_len);
}

int16_t kz_network_move_player(const char *player_id, const char *direction, uint8_t *response, uint16_t response_len) {
    snprintf(path_buf, sizeof(path_buf), "/api/player/%s/move", player_id);
    snprintf(body_buf, sizeof(body_buf), "{\"direction\":\"%s\"}", direction);
    return kz_network_http_post(path_buf, body_buf, response, response_len);
}

int16_t kz_network_leave_player(const char *player_id, uint8_t *response, uint16_t response_len) {
    snprintf(body_buf, sizeof(body_buf), "{\"id\":\"%s\"}", player_id);
    return kz_network_http_post("/api/player/leave", body_buf, response, response_len);
}
