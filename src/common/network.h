/**
 * KillZone Network Module - Atari 8-bit
 * 
 * FujiNet HTTP wrapper for server communication.
 * Uses fujinet-network.h library for HTTP operations.
 */

#ifndef KILLZONE_NETWORK_H
#define KILLZONE_NETWORK_H

#ifdef _CMOC_VERSION_
#include <cmoc.h>
#include <coco.h>
#else
#include <stdint.h>
#include <stddef.h>
#endif
#include "state.h"

/* Client version */
#define CLIENT_VERSION "1.0.0"

/* Server configuration */
#define SERVER_HOST "killzone.diller.org"
#define SERVER_PORT 80
#define SERVER_PROTO "HTTP"

/* Network device specification format: N:PROTO://HOST:PORT/PATH */
#define DEVICE_SPEC_SIZE 256

/* Network status */
typedef enum {
    NET_DISCONNECTED = 0,
    NET_CONNECTING = 1,
    NET_CONNECTED = 2,
    NET_ERROR = 3
} network_status_t;

/* Move result structure */
typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t collision;
    char messages[4][41];
    uint8_t message_count;
    char loser_id[32];
} move_result_t;

/* Initialization and lifecycle */
uint8_t kz_network_init(void);
uint8_t kz_network_close(void);
network_status_t kz_network_get_status(void);

/* Server communication */
/* Returns 1 if healthy, 0 if not */
uint8_t kz_network_health_check(void);

/* Returns 1 if success, 0 if failed. Populates player struct. */
uint8_t kz_network_join_player(const char *name, player_state_t *player);

/* Returns 1 if success, 0 if failed. Updates global state directly. */
uint8_t kz_network_get_world_state(void);

/* Returns 1 if success, 0 if failed. Populates player struct. */
uint8_t kz_network_get_player_status(const char *player_id, player_state_t *player);

/* Returns 1 if success, 0 if failed. Populates result struct. */
uint8_t kz_network_move_player(const char *player_id, const char *direction, move_result_t *result);

/* Returns 1 if success, 0 if failed. */
uint8_t kz_network_leave_player(const char *player_id);

#endif /* KILLZONE_NETWORK_H */