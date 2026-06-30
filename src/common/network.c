/**
 * KillZone Network Module
 * FujiNet communication with TCP binary protocol
 */

#include "network.h"
#include "state.h"
#ifdef _CMOC_VERSION_
#include <cmoc.h>
#include <coco.h>
#include "conio_wrapper.h"
#include "snprintf.h"
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#endif
#include "fujinet-network.h"
#include "json_helpers.h"
#include "constants.h"

/* Toggle for TCP mode - in production this might be a runtime switch or compile-time */
/* For this step, let's try to prioritize TCP if available, or just have dedicated functions */
#define USE_TCP 1

static network_status_t current_status = NET_DISCONNECTED;
static char device_spec[DEVICE_SPEC_SIZE];
static char path_buf[256];
static char body_buf[256];
static char value_buf[256]; /* Buffer for JSON values */
static char query_buf[64];  /* Buffer for JSON keys/paths */

/* TCP connection handle */
static char tcp_device_spec[64];
static uint8_t tcp_connected = 0;

/* --- TCP Helper Functions --- */
static void mark_connected(void) {
    tcp_connected = 1;
    current_status = NET_CONNECTED;
    state_set_connected(1);
}

static void mark_disconnected(void) {
    tcp_connected = 0;
    current_status = NET_DISCONNECTED;
    state_set_connected(0);
}

static uint8_t tcp_connect(void) {
    uint8_t err;
    snprintf(tcp_device_spec, sizeof(tcp_device_spec), "N:TCP://%s:%d", SERVER_HOST, SERVER_TCP_PORT);
    
    printf("Connecting TCP: %s\n", tcp_device_spec);
    err = network_open(tcp_device_spec, 0x0C, 0); /* 0x0C = Read/Write? Check docs. Usually 12 for RW */
    if (err != FN_ERR_OK) {
        printf("TCP Connect Failed: %d\n", err);
        mark_disconnected();
        return 0;
    }
    mark_connected();
    return 1;
}

static void tcp_disconnect(void) {
    if (tcp_connected) {
        network_close(tcp_device_spec);
    }
    mark_disconnected();
}
/* --- Existing Functions Modified --- */

static void build_device_spec(const char *path) {
    snprintf(device_spec, DEVICE_SPEC_SIZE, "N:HTTP://%s:%d%s", SERVER_HOST, SERVER_PORT, path);
}

/* Helper functions moved to json_helpers.c */

uint8_t kz_network_init(void) {
    printf("Network init...\n");
    current_status = NET_CONNECTING;
    state_set_connected(0);
    return 0;
}

uint8_t kz_network_close(void) {
    tcp_disconnect();
    return 0;
}

network_status_t kz_network_get_status(void) {
    return current_status;
}

uint8_t kz_network_health_check(void) {
    if (USE_TCP) {
        /* Simple TCP connect check */
        if (tcp_connected) {
            mark_connected();
            return 1;
        }
        return tcp_connect();
    }
    
    mark_disconnected();
    return 0;
}

/* TCP Join Implementation */
static uint8_t kz_network_join_player_tcp(const char *name, player_state_t *player) {
    /* Large buffer must be static to avoid stack overflow in cc65 */
    static uint8_t buf[256];
    int len;
    int idLen;
    size_t maxNameLen;
    
    if (!tcp_connected) {
        if (!tcp_connect()) {
            mark_disconnected();
            return 0;
        }
    }
    
    /* Packet: 0x01 [NameLen] [Name] */
    len = strlen(name);
    maxNameLen = sizeof(buf) - 2;
    if (len <= 0 || (size_t)len > maxNameLen) {
        mark_disconnected();
        return 0;
    }
    buf[0] = 0x01;
    buf[1] = (uint8_t)len;
    memcpy(&buf[2], name, len);
    
    if (network_write(tcp_device_spec, buf, 2 + len) != FN_ERR_OK) {
        mark_disconnected();
        return 0;
    }
    
    /* Read Response: 0x01 [IDLen] [ID] [X] [Y] [Health] */
    /* Read header first */
    len = network_read(tcp_device_spec, buf, 2); 
    if (len < 2 || buf[0] != 0x01) {
        mark_disconnected();
        return 0;
    }
    
    idLen = buf[1];
    if (idLen <= 0 || idLen >= (int)sizeof(player->id) || idLen >= (int)sizeof(buf)) {
        mark_disconnected();
        return 0;
    }
    /* Read ID */
    len = network_read(tcp_device_spec, buf, idLen);
    if (len != idLen) {
        mark_disconnected();
        return 0;
    }
    memcpy(player->id, buf, idLen);
    player->id[idLen] = '\0';
    strncpy(player->name, name, sizeof(player->name) - 1);
    player->name[sizeof(player->name) - 1] = '\0';
    
    /* Read data: X, Y, Health */
    len = network_read(tcp_device_spec, buf, 3);
    if (len != 3) {
        mark_disconnected();
        return 0;
    }
    
    player->x = buf[0];
    player->y = buf[1];
    player->health = buf[2];
    
    /* Read server version: [VerLen] [Version] */
    len = network_read(tcp_device_spec, buf, 1);
    if (len == 1 && buf[0] > 0 && buf[0] < 16) {
        int verLen = buf[0];
        if (verLen >= (int)sizeof(buf)) {
            mark_disconnected();
            return 0;
        }
        len = network_read(tcp_device_spec, buf, verLen);
        if (len == verLen) {
            buf[verLen] = '\0';
            state_set_server_version((char*)buf);
        } else {
            mark_disconnected();
            return 0;
        }
    }
    
    strcpy(player->status, "alive");
    strcpy(player->type, "player");
    player->isHunter = 0;
    
    state_set_local_player(player);
    mark_connected();
    return 1;
}

uint8_t kz_network_join_player(const char *name, player_state_t *player) {
    if (USE_TCP) return kz_network_join_player_tcp(name, player);
    return 0; 
}

/* TCP Move Implementation */
static uint8_t kz_network_move_player_tcp(const char *player_id, const char *direction, move_result_t *result) {
    uint8_t buf[64];
    int len;
    const player_state_t *local;
    char dirChar = 'x';
    if (strcmp(direction, "up") == 0) dirChar = 'u';
    if (strcmp(direction, "down") == 0) dirChar = 'd';
    if (strcmp(direction, "left") == 0) dirChar = 'l';
    if (strcmp(direction, "right") == 0) dirChar = 'r';
    
    if (!tcp_connected || !result) {
        mark_disconnected();
        return 0;
    }

    result->collision = 0;
    result->message_count = 0;
    result->messages[0][0] = '\0';
    result->loser_id[0] = '\0';
    
    /* Packet: 0x02 [DirChar] */
    buf[0] = 0x02;
    buf[1] = (uint8_t)dirChar;
    
    if (network_write(tcp_device_spec, buf, 2) != FN_ERR_OK) {
        mark_disconnected();
        return 0;
    }
    
    /* Resp: 0x02 [X] [Y] [Health] [Collision] [MsgLen] [Msg...] [LoserIdLen] [LoserId...] */
    len = network_read(tcp_device_spec, buf, 6);
    if (len < 6 || buf[0] != 0x02) {
        mark_disconnected();
        return 0;
    }
    
    result->x = buf[1];
    result->y = buf[2];
    /* Update local player health too? */
    local = state_get_local_player();
    if (local) {
        ((player_state_t*)local)->health = buf[3];
        ((player_state_t*)local)->x = result->x;
        ((player_state_t*)local)->y = result->y;
    }
    
    result->collision = buf[4];
    
    /* Read battle message if present */
    {
        uint8_t msgLen = buf[5];
        if (msgLen > 0) {
            if (msgLen >= sizeof(buf)) {
                mark_disconnected();
                return 0;
            }
            len = network_read(tcp_device_spec, buf, msgLen);
            if (len == msgLen) {
                uint8_t copyLen = msgLen;
                if (copyLen > 40) copyLen = 40;
                memcpy(result->messages[0], buf, copyLen);
                result->messages[0][copyLen] = '\0';
                if (copyLen > 0) {
                    result->message_count = 1;
                    /* Store in state for non-blocking display */
                    state_set_combat_message(result->messages[0]);
                }
            } else {
                mark_disconnected();
                return 0;
            }
        }
    }

    /* Read loser ID for death handling (optional for backward compatibility) */
    len = network_read(tcp_device_spec, buf, 1);
    if (len == 1) {
        uint8_t loserLen = buf[0];
        if (loserLen > 0) {
            if (loserLen >= sizeof(buf)) {
                mark_disconnected();
                return 0;
            }
            len = network_read(tcp_device_spec, buf, loserLen);
            if (len != loserLen) {
                mark_disconnected();
                return 0;
            }
            if (loserLen >= sizeof(result->loser_id)) loserLen = sizeof(result->loser_id) - 1;
            memcpy(result->loser_id, buf, loserLen);
            result->loser_id[loserLen] = '\0';
        }
    } else {
        mark_disconnected();
        return 0;
    }
    
    mark_connected();
    return 1;
}

uint8_t kz_network_move_player(const char *player_id, const char *direction, move_result_t *result) {
    if (USE_TCP) return kz_network_move_player_tcp(player_id, direction, result);
    return 0;
}

uint8_t kz_network_leave_player(const char *player_id) {
    tcp_disconnect();
    return 1;
}

static player_state_t other_players[MAX_OTHER_PLAYERS];

uint8_t kz_network_get_world_state(void) {
    if (USE_TCP) {
        static uint8_t buf[256]; /* Large buffer static */
        int len;
        uint8_t count;
        uint8_t i;
        uint8_t actual_count = 0;
        char typeChar;
        uint8_t x, y;
        const player_state_t *local;
        uint8_t msgLen;

        if (!tcp_connected) {
            mark_disconnected();
            return 0;
        }
        
        buf[0] = 0x03;
        if (network_write(tcp_device_spec, buf, 1) != FN_ERR_OK) {
            mark_disconnected();
            return 0;
        }
        
        /* Resp: 0x03 [Count] [TicksLow] [TicksHigh] [MsgLen] [Msg...] [Entities...] */
        len = network_read(tcp_device_spec, buf, 5);
        if (len < 5 || buf[0] != 0x03) {
            mark_disconnected();
            return 0;
        }
        
        count = buf[1];
        {         
            uint16_t ticks = (uint16_t)buf[2] | ((uint16_t)buf[3] << 8);
            state_set_world_ticks(ticks);
        }
        /* Read message if present */
        msgLen = buf[4];
        if (msgLen > 0 && msgLen < 40) {
            len = network_read(tcp_device_spec, buf, msgLen);
            if (len == msgLen) {
                buf[msgLen] = '\0';
                state_set_combat_message((char*)buf);
            } else {
                mark_disconnected();
                return 0;
            }
        } else if (msgLen >= 40) {
            mark_disconnected();
            return 0;
        }
        
        local = state_get_local_player();
        
        for (i = 0; i < count; i++) {
            /* Read 3 bytes: Type, X, Y */
            len = network_read(tcp_device_spec, buf, 3);
            if (len < 3) {
                mark_disconnected();
                return 0;
            }
            
            typeChar = (char)buf[0];
            x = buf[1];
            y = buf[2];
            
            if (typeChar == 'M') {
                /* Me / Local Player - update if moved externally? */
                if (local) {
                     ((player_state_t*)local)->x = x;
                     ((player_state_t*)local)->y = y;
                }
                continue;
            }
            
            if (actual_count < MAX_OTHER_PLAYERS) {
                player_state_t *p = &other_players[actual_count];
                /* We don't have ID or Name in simplified packet, just position/type */
                /* This is a limitation of the simplified protocol, we just render them blindly */
                /* For full feature parity we need IDs */
                /* But for "bouncy" style, it's just visual. */
                p->x = x;
                p->y = y;
                p->isHunter = (typeChar == 'H');
                
                if (typeChar == 'P') strcpy(p->type, "player");
                else strcpy(p->type, "mob");
                
                actual_count++;
            }
        }
        
        state_set_other_players(other_players, actual_count);
        mark_connected();
        return 1;
    }
    mark_disconnected();
    return 0;
}

uint8_t kz_network_get_player_status(const char *player_id, player_state_t *player) {
    return 0; // Not used in TCP loop currently
}
