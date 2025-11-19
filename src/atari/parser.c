/**
 * KillZone JSON Parser Implementation
 * 
 * Handles parsing of JSON responses from the game server.
 */

#include "parser.h"
#include "state.h"
#include "json.h"
#include "display.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * Parse integer from string (simple positive integers)
 */
static uint32_t parse_fast_int(const char *s) {
    uint32_t res = 0;
    while (*s && (*s < '0' || *s > '9')) s++; /* Skip non-digits */
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res;
}

/**
 * Parse walls from JSON response
 * Extracts wall positions from walls array
 */
static void parse_walls_from_response(const uint8_t *response, uint16_t len) {
    const char *json = (const char *)response;
    const char *array_pos;
    const char *next_obj;
    uint32_t x_val = 0, y_val = 0;
    uint16_t count = 0;
    static wall_t temp_walls[MAX_WALLS];
    char obj_buf[64];
    uint16_t obj_len;
    
    if (!response || len == 0) {
        return;
    }
    
    /* Find walls array */
    array_pos = strstr(json, "\"walls\":[");
    if (!array_pos) {
        state_clear_walls();  /* No walls in response */
        return;
    }
    
    array_pos += 9;  /* Skip "walls":[ */
    
    /* Parse each wall object */
    while (array_pos && count < MAX_WALLS) {
        /* Find end of current object */
        next_obj = strchr(array_pos, '}');
        if (!next_obj) break;
        
        /* Extract object string to buffer */
        obj_len = (uint16_t)(next_obj - array_pos + 1);
        if (obj_len >= sizeof(obj_buf)) obj_len = sizeof(obj_buf) - 1;
        
        strncpy(obj_buf, array_pos, obj_len);
        obj_buf[obj_len] = '\0';
        
        /* Parse x and y from object buffer manually to be robust */
        {
            char *p = obj_buf;
            x_val = 0;
            y_val = 0;
            
            while (*p) {
                /* Look for "key" */
                if (*p == '"') {
                    char key = p[1];
                    if (p[2] == '"') {
                        /* Found "x" or "y" */
                        if (key == 'x') {
                            x_val = parse_fast_int(p + 3);
                        } else if (key == 'y') {
                            y_val = parse_fast_int(p + 3);
                        }
                    }
                }
                p++;
            }
        }
        
        /* Store wall */
        temp_walls[count].x = (uint8_t)x_val;
        temp_walls[count].y = (uint8_t)y_val;
        count++;
        
        /* Move to next object */
        array_pos = next_obj + 1;
        /* Skip comma if present */
        if (*array_pos == ',') array_pos++;
        /* Stop if array ends */
        if (*array_pos == ']') break;
    }
    
    /* Update state with walls */
    state_set_walls(temp_walls, count);
}

/**
 * Parse entities from players array in JSON
 * Extracts x,y coordinates for each entity
 */
static void parse_entities_from_response(const uint8_t *response, uint16_t len) {
    const char *json = (const char *)response;
    const char *array_pos;
    const char *id_start;
    const char *x_start;
    const char *y_start;
    const char *type_start;
    const char *hunter_start;
    const char *next_obj;
    const char *id_val;
    const char *type_val;
    uint8_t count = 0;
    player_state_t *local = (player_state_t *)state_get_local_player();
    char id_buf[32];
    char type_buf[8];
    uint32_t x_val, y_val;
    int i;
    int is_hunter;
    static player_state_t other_players[MAX_OTHER_PLAYERS];
    
    /* Find players array */
    array_pos = strstr(json, "\"players\":[");
    if (!array_pos) return;
    
    array_pos += 11; /* Skip "players":[ */
    
    /* Parse each entity - look for {"id":"...", "x":N, "y":N, "type":"..."} patterns */
    while (count < MAX_OTHER_PLAYERS && *array_pos && *array_pos != ']') {
        id_start = strstr(array_pos, "\"id\":\"");
        x_start = strstr(array_pos, "\"x\":");
        y_start = strstr(array_pos, "\"y\":");
        type_start = strstr(array_pos, "\"type\":\"");
        next_obj = strchr(array_pos, '}');
        
        if (!id_start || !x_start || !y_start || !next_obj) break;
        if (id_start > next_obj) break; /* id not in this object */
        
        /* Extract id */
        id_val = id_start + 6;
        i = 0;
        while (i < 31 && id_val[i] && id_val[i] != '"') {
            id_buf[i] = id_val[i];
            i++;
        }
        id_buf[i] = '\0';
        
        /* Skip local player */
        if (local && strcmp(id_buf, local->id) == 0) {
            array_pos = next_obj + 1;
            continue;
        }
        
        /* Extract x */
        x_val = (uint32_t)strtol(x_start + 4, NULL, 10);
        
        /* Extract y */
        y_val = (uint32_t)strtol(y_start + 4, NULL, 10);
        
        /* Extract type (default to "mob" if not found) */
        strcpy(type_buf, "mob");
        if (type_start && type_start < next_obj) {
            type_val = type_start + 8;
            i = 0;
            while (i < 7 && type_val[i] && type_val[i] != '"') {
                type_buf[i] = type_val[i];
                i++;
            }
            type_buf[i] = '\0';
        }
        
        /* Extract isHunter flag (default to 0 if not found) */
        is_hunter = 0;
        hunter_start = strstr(array_pos, "\"isHunter\":");
        if (hunter_start && hunter_start < next_obj) {
            hunter_start += 11;  /* Skip "isHunter": */
            if (strstr(hunter_start, "true") && strstr(hunter_start, "true") < next_obj) {
                is_hunter = 1;
            }
        }
        
        /* Store entity */
        strncpy(other_players[count].id, id_buf, sizeof(other_players[count].id) - 1);
        other_players[count].x = (uint8_t)x_val;
        other_players[count].y = (uint8_t)y_val;
        other_players[count].health = 100;
        strcpy(other_players[count].status, "alive");
        strcpy(other_players[count].type, type_buf);
        other_players[count].isHunter = is_hunter;
        count++;
        
        array_pos = next_obj + 1;
    }
    
    /* Update state with other players */
    state_set_other_players(other_players, count);
}

/**
 * Parse join response JSON
 */
void parse_join_response(const uint8_t *response, uint16_t len) {
    char player_id[32];
    char player_name[32];
    uint32_t x, y, health;
    player_state_t player;
    const char *json;
    
    if (!response || len == 0) {
        return;
    }
    
    json = (const char *)response;
    
    /* Extract player ID */
    if (json_get_string(json, "id", player_id, sizeof(player_id)) == 0) {
        return;
    }
    
    /* Extract player name if available, otherwise use ID */
    if (json_get_string(json, "name", player_name, sizeof(player_name)) == 0) {
        strncpy(player_name, player_id, sizeof(player_name) - 1);
    }
    
    /* Extract position */
    if (!json_get_uint(json, "x", &x) || !json_get_uint(json, "y", &y)) {
        return;
    }
    
    /* Extract health */
    if (!json_get_uint(json, "health", &health)) {
        health = 100;
    }
    
    /* Create player state */
    strncpy(player.id, player_id, sizeof(player.id) - 1);
    strncpy(player.name, player_name, sizeof(player.name) - 1);
    player.x = (uint8_t)x;
    player.y = (uint8_t)y;
    player.health = (uint8_t)health;
    strcpy(player.status, "alive");
    
    /* Store in state */
    state_set_local_player(&player);
}

/**
 * Parse world state JSON
 */
void parse_world_state(const uint8_t *response, uint16_t len) {
    uint32_t width, height, ticks;
    const char *json;
    static char kill_msg[41];
    
    if (!response || len == 0) {
        return;
    }
    
    json = (const char *)response;
    
    /* Extract world dimensions */
    if (json_get_uint(json, "width", &width) && json_get_uint(json, "height", &height)) {
        state_set_world_dimensions((uint8_t)width, (uint8_t)height);
    }
    
    /* Extract world ticks */
    if (json_get_uint(json, "ticks", &ticks)) {
        state_set_world_ticks((uint16_t)ticks);
    }
    
    /* Extract and display lastKillMessage if present */
    if (json_get_string(json, "lastKillMessage", kill_msg, sizeof(kill_msg)) > 0) {
        display_draw_combat_message(kill_msg);
    }
    
    /* Optimization: Only parse walls if level changed */
    {
        char level_name[32] = "";
        /* Try to get level name from response */
        if (json_get_string(json, "level", level_name, sizeof(level_name)) > 0) {
            /* Check if level changed or if we have no walls yet */
            uint16_t wc;
            state_get_walls(&wc);
            
            if (strcmp(level_name, state_get_level_name()) != 0 || wc == 0) {
                /* Level changed or init! Update name and parse walls */
                state_set_level_name(level_name);
                parse_walls_from_response(response, len);
            }
        } else {
            /* Fallback: Always parse if no level name provided */
            parse_walls_from_response(response, len);
        }
    }
    
    /* Parse entities from players array */
    parse_entities_from_response(response, len);
}

/**
 * Check if a player ID exists in the players array of the response
 */
int parser_is_player_in_world(const uint8_t *response, const char *player_id) {
    const char *json = (const char *)response;
    char search_str[64];
    const char *players_array;
    
    if (!response || !player_id) return 0;
    
    players_array = strstr(json, "\"players\":[");
    if (players_array) {
        snprintf(search_str, sizeof(search_str), "\"id\":\"%s\"", player_id);
        if (strstr(players_array, search_str) != NULL) {
            return 1;
        }
    }
    return 0;
}
