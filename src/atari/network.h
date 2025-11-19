/**
 * KillZone Network Module - Atari 8-bit
 * 
 * FujiNet HTTP wrapper for server communication.
 * Uses fujinet-network.h library for HTTP operations.
 */

#ifndef KILLZONE_NETWORK_H
#define KILLZONE_NETWORK_H

#include <stdint.h>
#include <stddef.h>

/* Client version */
#define CLIENT_VERSION "1.0.0"

/* Server configuration */
/* For local testing with netsio/fujinet-pc */
#define SERVER_HOST "localhost"
#define SERVER_PORT 3000
#define SERVER_PROTO "HTTP"

/* Network device specification format: N:PROTO://HOST:PORT/PATH */
#define DEVICE_SPEC_SIZE 256
#define RESPONSE_BUFFER_SIZE 1024

/* Network status */
typedef enum {
    NET_DISCONNECTED = 0,
    NET_CONNECTING = 1,
    NET_CONNECTED = 2,
    NET_ERROR = 3
} network_status_t;

/* Initialization and lifecycle */
uint8_t kz_network_init(void);
uint8_t kz_network_close(void);
network_status_t kz_network_get_status(void);

/* HTTP operations */
int16_t kz_network_http_get(const char *path, uint8_t *response, uint16_t response_len);
int16_t kz_network_http_post(const char *path, const char *body, 
                             uint8_t *response, uint16_t response_len);

/* Server communication */
int16_t kz_network_health_check(uint8_t *response, uint16_t response_len);
int16_t kz_network_join_player(const char *name, uint8_t *response, uint16_t response_len);
int16_t kz_network_get_world_state(uint8_t *response, uint16_t response_len);
int16_t kz_network_get_player_status(const char *player_id, uint8_t *response, uint16_t response_len);
int16_t kz_network_move_player(const char *player_id, const char *direction,
                               uint8_t *response, uint16_t response_len);
int16_t kz_network_leave_player(const char *player_id, uint8_t *response, uint16_t response_len);

#endif /* KILLZONE_NETWORK_H */
