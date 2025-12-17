/**
 * KillZone Display Module - Atari 8-bit
 * 
 * Text-based display rendering for Atari.
 * Shows player position, world state, and game status.
 */

#ifndef KILLZONE_DISPLAY_H
#define KILLZONE_DISPLAY_H

#ifdef _CMOC_VERSION_
#include <cmoc.h>
#include <coco.h>
#else
#include <stdint.h>
#endif
#include "state.h"

#include "constants.h"

/* Initialization and lifecycle */
void display_init(void);
void display_close(void);
void display_show_welcome(const char *server_name);

/* Rendering */
/* Rendering */
/* display_render_game is the main rendering function now */

/* Status bar */
void display_draw_status_bar(const char *player_name, uint8_t player_count, 
                             const char *connection_status, uint16_t world_ticks);
void display_draw_command_help(void);
void display_draw_combat_message(const char *message);

/* Game Rendering */
void display_render_game(const player_state_t *local, const player_state_t *others, uint8_t count, int force_refresh);

/* Dialogs and Prompts */
void display_show_join_prompt(void);
void display_show_rejoining(const char *name);
void display_show_quit_confirmation(void);
void display_show_connection_lost(void);
void display_show_death_message(void);
void display_show_error(const char *error);
void display_toggle_color_scheme(void);

/* Direct drawing */


#endif /* KILLZONE_DISPLAY_H */
