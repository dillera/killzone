/**
 * KillZone State Module Implementation - Atari 8-bit
 * 
 * Local client state management.
 */

#include "state.h"
#ifdef _CMOC_VERSION_
#include <cmoc.h>
#include <coco.h>
#else
#include <string.h>
#endif

/* Global state */
static client_state_t current_state = STATE_INIT;
static player_state_t local_player;
static player_state_t other_players[MAX_OTHER_PLAYERS];
static uint8_t other_player_count = 0;
static uint8_t world_width = 40;
static uint8_t world_height = 20;
static uint16_t world_ticks = 0;
char error_message[128];
static int is_rejoining = 0;
static int is_connected = 1;  /* Track connection state (1=connected, 0=disconnected) */

/**
 * Initialize state system
 */
void state_init(void) {
    current_state = STATE_INIT;
    memset(&local_player, 0, sizeof(local_player));
    memset(other_players, 0, sizeof(other_players));
    other_player_count = 0;
    world_width = 40;
    world_height = 20;
    memset(error_message, 0, sizeof(error_message));
}

/**
 * Close state system
 */
void state_close(void) {
    current_state = STATE_INIT;
}

/**
 * Get current client state
 */
client_state_t state_get_current(void) {
    return current_state;
}

/**
 * Set current client state
 */
void state_set_current(client_state_t new_state) {
    current_state = new_state;
}

/**
 * Set rejoining flag
 */
void state_set_rejoining(int rejoining) {
    is_rejoining = rejoining;
}

/**
 * Get rejoining flag
 */
int state_is_rejoining(void) {
    return is_rejoining;
}

/**
 * Set connection state
 */
void state_set_connected(int connected) {
    is_connected = connected;
}

/**
 * Get connection state
 */
int state_is_connected(void) {
    return is_connected;
}

/**
 * Set local player state
 */
void state_set_local_player(const player_state_t *player) {
    if (player) {
        memcpy(&local_player, player, sizeof(player_state_t));
    }
}

/**
 * Get local player state
 */
const player_state_t *state_get_local_player(void) {
    return &local_player;
}

/**
 * Clear local player state (for respawn/rejoin)
 */
void state_clear_local_player(void) {
    memset(&local_player, 0, sizeof(local_player));
}

/**
 * Update local player position
 */
void state_update_local_position(uint8_t x, uint8_t y) {
    local_player.x = x;
    local_player.y = y;
}

/**
 * Update local player health
 */
void state_update_local_health(uint8_t health) {
    local_player.health = health;
}

/**
 * Set other players in world
 */
void state_set_other_players(const player_state_t *players, uint8_t count) {
    if (count > MAX_OTHER_PLAYERS) {
        count = MAX_OTHER_PLAYERS;
    }
    
    if (players && count > 0) {
        memcpy(other_players, players, count * sizeof(player_state_t));
    }
    
    other_player_count = count;
}

/**
 * Get other players in world
 */
const player_state_t *state_get_other_players(uint8_t *count) {
    if (count) {
        *count = other_player_count;
    }
    return other_players;
}

/**
 * Clear other players
 */
void state_clear_other_players(void) {
    memset(other_players, 0, sizeof(other_players));
    other_player_count = 0;
}

/**
 * Set world dimensions
 */
void state_set_world_dimensions(uint8_t width, uint8_t height) {
    world_width = width;
    world_height = height;
}

/**
 * Get world width
 */
uint8_t state_get_world_width(void) {
    return world_width;
}

/**
 * Get world height
 */
uint8_t state_get_world_height(void) {
    return world_height;
}

/**
 * Set error message
 */
void state_set_error(const char *message) {
    if (message) {
        memset(error_message, 0, sizeof(error_message));
        strncpy(error_message, message, sizeof(error_message) - 1);
    }
}

/**
 * Get error message
 */
const char *state_get_error(void) {
    return error_message;
}

/**
 * Set world ticks
 */
void state_set_world_ticks(uint16_t ticks) {
    world_ticks = ticks;
}

/**
 * Get world ticks
 */
uint16_t state_get_world_ticks(void) {
    return world_ticks;
}
