/**
 * KillZone Display Module - Atari 8-bit
 * 
 * Text-based display rendering for Atari.
 * Shows player position, world state, and game status.
 */

#ifndef KILLZONE_DISPLAY_H
#define KILLZONE_DISPLAY_H

#include <stdint.h>
#include "state.h"

/* Display dimensions */
#define DISPLAY_WIDTH 40
#define DISPLAY_HEIGHT 20
#define STATUS_BAR_HEIGHT 4
#define STATUS_BAR_START (DISPLAY_HEIGHT)

/* Display characters */
#define CHAR_EMPTY '.'
#define CHAR_PLAYER '@'
#define CHAR_ENEMY '*'
/* ATASCII box drawing characters for walls */
#define CHAR_WALL 1  /* DEC 1 - vertical bar */
#define CHAR_WALL_H 2  /* DEC 2 - horizontal bar */
#define CHAR_WALL_TL 3  /* DEC 3 - top-left corner */
#define CHAR_WALL_TR 4  /* DEC 4 - top-right corner */
#define CHAR_WALL_BL 5  /* DEC 5 - bottom-left corner */
#define CHAR_WALL_BR 18  /* DEC 18 - bottom-right corner */
#define CHAR_WALL_CROSS 19  /* DEC 19 - cross/intersection */

/* Initialization and lifecycle */
void display_init(void);
void display_close(void);
void display_show_welcome(const char *server_name);

/* Rendering */
void display_clear(void);
void display_draw_world(const world_state_t *world);
void display_draw_status(const player_state_t *player);
void display_draw_message(const char *message);
void display_update(void);

/* Status bar */
void display_draw_status_bar(const char *player_name, uint8_t player_count, 
                             const char *connection_status, uint16_t world_ticks);
void display_draw_command_help(void);
void display_draw_combat_message(const char *message);

/* Direct drawing */
void display_draw_char(uint8_t x, uint8_t y, char c);

#endif /* KILLZONE_DISPLAY_H */
