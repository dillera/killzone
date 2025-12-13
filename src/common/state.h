/**
 * KillZone State Module - Atari 8-bit
 * 
 * Local client state management.
 * Tracks player position, health, and world view.
 */

#ifndef KILLZONE_STATE_H
#define KILLZONE_STATE_H

#ifdef _CMOC_VERSION_
#include <cmoc.h>
#include <coco.h>
#else
#include <stdint.h>
#endif

/* Maximum number of other players visible */
#define MAX_OTHER_PLAYERS 10

/* Player state */
typedef struct {
    char id[32];
    char name[32];
    uint8_t x;
    uint8_t y;
    uint8_t health;
    char status[16];
    char type[8];  /* "player" or "mob" */
    int isHunter;  /* 1 if this is a hunter mob, 0 otherwise */
} player_state_t;

/* World state */
typedef struct {
    player_state_t local_player;
    player_state_t other_players[MAX_OTHER_PLAYERS];
    uint8_t other_player_count;
    uint8_t world_width;
    uint8_t world_height;
    uint16_t world_ticks;
} world_state_t;

/* Client state machine */
typedef enum {
    STATE_INIT = 0,
    STATE_CONNECTING = 1,
    STATE_JOINING = 2,
    STATE_PLAYING = 3,
    STATE_DEAD = 4,
    STATE_ERROR = 5
} client_state_t;

/* Rejoin flag */
void state_set_rejoining(int rejoining);
int state_is_rejoining(void);

/* Connection state */
void state_set_connected(int connected);
int state_is_connected(void);

/* Initialization and lifecycle */
void state_init(void);
void state_close(void);

/* State management */
client_state_t state_get_current(void);
void state_set_current(client_state_t new_state);

/* Player state */
void state_set_local_player(const player_state_t *player);
const player_state_t *state_get_local_player(void);
void state_clear_local_player(void);
void state_update_local_position(uint8_t x, uint8_t y);
void state_update_local_health(uint8_t health);

/* World state */
void state_set_other_players(const player_state_t *players, uint8_t count);
const player_state_t *state_get_other_players(uint8_t *count);
void state_clear_other_players(void);

/* World dimensions */
void state_set_world_dimensions(uint8_t width, uint8_t height);
uint8_t state_get_world_width(void);
uint8_t state_get_world_height(void);

/* World ticks */
void state_set_world_ticks(uint16_t ticks);
uint16_t state_get_world_ticks(void);

/* Error handling */
void state_set_error(const char *message);
const char *state_get_error(void);

#endif /* KILLZONE_STATE_H */
