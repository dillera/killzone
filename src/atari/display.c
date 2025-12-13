/**
 * KillZone Display Module Implementation - Atari 8-bit
 * 
 * Text-based display rendering.
 * Uses Atari's text mode with last 4 lines for status bar.
 */

#include "display.h"
#include "network.h"
#include <stdio.h>
#include <string.h>
#include <conio.h>

/* Direct drawing to screen, no buffer needed */

/**
 * Initialize display system
 */
void display_init(void) {
    clrscr();  /* Clear screen using conio */
}

/**
 * Close display system
 */
void display_close(void) {
    clrscr();
}

/**
 * Show welcome screen
 */
void display_show_welcome(const char *server_name) {
    static char url_buf[60];
    (void)server_name; /* Suppress unused warning */
    
    clrscr();
    gotoxy(0, 4);
    printf("  *** KILLZONE ***\n");
    gotoxy(0, 5);
    printf("  Version %s\n", CLIENT_VERSION);
    gotoxy(0, 6);
    printf("  @2025 DillerNet Studios\n");
    gotoxy(0, 8);
    printf("  Connecting to server:\n");
    gotoxy(0, 9);
    
    /* Build full URL */
    snprintf(url_buf, sizeof(url_buf), "  %s://%s:%d", 
             SERVER_PROTO, SERVER_HOST, SERVER_PORT);

    printf("%s\n", url_buf);
    
    gotoxy(0, 11);
    printf("  Waiting for game world...\n");
}



/**
 * Draw status bar (last 4 lines of screen)
 * 
 * Shows: player name, player count, connection status, world ticks
 * Uses cputsxy for direct character placement without scrolling
 */
void display_draw_status_bar(const char *player_name, uint8_t player_count, 
                             const char *connection_status, uint16_t world_ticks) {
    static char line_buf[41];
    static char ticks_buf[11];
    static char ver_buf[16];
    const char *server_ver;
    
    if (!player_name || !connection_status) {
        return;
    }
    
    /* Line 20: Player info on left, ticks on right (starting at char 30) */
    snprintf(line_buf, sizeof(line_buf), "%s P:%d %s", player_name, player_count, connection_status);
    cputsxy(0, 20, line_buf);
    
    snprintf(ticks_buf, sizeof(ticks_buf), "T:%d", world_ticks);
    cputsxy(30, 20, ticks_buf);
    
    /* Line 21: Clear for server messages (combat, kills, etc) */
    /* Messages written by display_draw_combat_message() */
    
    /* Line 22: Separator */
    cputsxy(0, 22, "----------------------------------------");
    
    /* Line 23: Command help on left, version on right */
    cputsxy(0, 23, "WASD=Move R=Refresh Q=Quit");
    
    /* Version display at far right: C1.1.0|S1.1.0 */
    server_ver = state_get_server_version();
    snprintf(ver_buf, sizeof(ver_buf), "C%s|S%s", CLIENT_VERSION, server_ver);
    cputsxy(40 - strlen(ver_buf), 23, ver_buf);
}

/**
 * Draw command help line
 */
void display_draw_command_help(void) {
    printf("WASD/Arrows=Move | Q=Quit | A=Attack\n");
}

/**
 * Draw combat message on line 21 (fixed position, no scrolling)
 */
void display_draw_combat_message(const char *message) {
    static char line_buf[41];
    
    if (!message) {
        return;
    }
    
    /* Clear line first */
    memset(line_buf, ' ', 40);
    line_buf[40] = '\0';
    cputsxy(0, 21, line_buf);
    
    /* Draw message */
    snprintf(line_buf, sizeof(line_buf), "%s", message);
    cputsxy(0, 21, line_buf);
}

/* Game Rendering */
void display_render_game(const player_state_t *local, const player_state_t *others, uint8_t count, int force_refresh) {
    static uint8_t last_player_x = 255;
    static uint8_t last_player_y = 255;
    static uint8_t last_other_positions[MAX_OTHER_PLAYERS * 2];  /* x,y pairs */
    static uint8_t last_other_count = 255;
    static int world_rendered = 0; /* Moved declaration to top */
    static int positions_initialized = 0;
    uint8_t x, y, i;
    char entity_char;
    
    /* Initialize position tracking to invalid values on first call */
    if (!positions_initialized) {
        for (i = 0; i < MAX_OTHER_PLAYERS * 2; i++) {
            last_other_positions[i] = 255;
        }
        positions_initialized = 1;
    }
    
    if (!local || local->x >= 255 || local->y >= 255) {
        return;
    }
    
    /* Full redraw on first render or when player count changes or refresh requested */
    
    if (force_refresh) {
        world_rendered = 0;
    }
    
    if (!world_rendered || (last_other_count != 255 && last_other_count != count)) {
        /* Only clear the play field (lines 0-19), NOT the status area (lines 20-23) */
        /* Draw world line by line - this fills the play area without clrscr */
        for (y = 0; y < DISPLAY_HEIGHT; y++) {
            gotoxy(0, y);
            for (x = 0; x < DISPLAY_WIDTH; x++) {
                printf("%c", CHAR_EMPTY);
            }
        }
        
        /* Draw other entities */
        for (i = 0; i < count; i++) {
            if (others[i].x < DISPLAY_WIDTH && others[i].y < DISPLAY_HEIGHT) {
                gotoxy(others[i].x, others[i].y);
                if (strcmp(others[i].type, "player") == 0) {
                    entity_char = CHAR_WALL; /* Using # for other players as per original code */
                } else if (others[i].isHunter) {
                    entity_char = CHAR_HUNTER;
                } else {
                    entity_char = CHAR_ENEMY;
                }
                printf("%c", entity_char);
            }
        }
        
        /* Draw local player */
        if (local->x < DISPLAY_WIDTH && local->y < DISPLAY_HEIGHT) {
            gotoxy(local->x, local->y);
            printf("%c", CHAR_PLAYER);
        }
        
        last_player_x = local->x;
        last_player_y = local->y;
        last_other_count = count;
        world_rendered = 1;
        
        /* Initialize tracked positions */
        for (i = 0; i < count; i++) {
            last_other_positions[i * 2] = others[i].x;
            last_other_positions[i * 2 + 1] = others[i].y;
        }
        
    } else if (local->x != last_player_x || local->y != last_player_y) {
        /* Incremental update: only redraw changed positions */
        
        /* Erase old player position */
        gotoxy(last_player_x, last_player_y);
        printf("%c", CHAR_EMPTY);
        
        /* Draw new player position */
        gotoxy(local->x, local->y);
        printf("%c", CHAR_PLAYER);
        
        last_player_x = local->x;
        last_player_y = local->y;
    }
    
    /* Update other players incrementally */
    /* Only if we didn't do a full redraw */
    if (world_rendered) {
        for (i = 0; i < count; i++) {
            uint8_t old_x = last_other_positions[i * 2];
            uint8_t old_y = last_other_positions[i * 2 + 1];
            uint8_t new_x_other = others[i].x;
            uint8_t new_y_other = others[i].y;
            
            /* If position changed, update it */
            if (old_x != new_x_other || old_y != new_y_other) {
                /* Erase old position (if it was valid, i.e. not 255) */
                if (old_x < DISPLAY_WIDTH && old_y < DISPLAY_HEIGHT) {
                    gotoxy(old_x, old_y);
                    printf("%c", CHAR_EMPTY);
                }
                
                /* Draw new position */
                if (new_x_other < DISPLAY_WIDTH && new_y_other < DISPLAY_HEIGHT) {
                    gotoxy(new_x_other, new_y_other);
                    if (strcmp(others[i].type, "player") == 0) {
                        entity_char = CHAR_WALL;
                    } else if (others[i].isHunter) {
                        entity_char = CHAR_HUNTER;
                    } else {
                        entity_char = CHAR_ENEMY;
                    }
                    printf("%c", entity_char);
                }
                
                /* Update tracked position */
                last_other_positions[i * 2] = new_x_other;
                last_other_positions[i * 2 + 1] = new_y_other;
            }
        }
    }
}

/* Dialogs and Prompts */

void display_show_join_prompt(void) {
    clrscr();
    gotoxy(0, 5);
    printf("Enter player name:\n");
    gotoxy(0, 7);
}

void display_show_rejoining(const char *name) {
    clrscr();
    gotoxy(0, 8);
    printf("  Rejoining as: %s\n", name);
    gotoxy(0, 10);
    printf("  Please wait...\n");
}

void display_show_quit_confirmation(void) {
    clrscr();
    gotoxy(0, 8);
    printf("  Are you sure you want to quit?\n");
    gotoxy(0, 10);
    printf("  Y=Quit  N=Continue Playing\n");
    gotoxy(0, 12);
    printf("  Press a key: ");
}

void display_show_connection_lost(void) {
    clrscr();
    gotoxy(0, 8);
    printf("  CONNECTION LOST\n");
    gotoxy(0, 10);
    printf("  You were disconnected from the server.\n");
    gotoxy(0, 12);
    printf("  Y=Quit  N=Rejoin\n");
    gotoxy(0, 14);
    printf("  Press a key: ");
}

void display_show_death_message(void) {
    clrscr();
    gotoxy(0, 8);
    printf("  *** YOU WERE KILLED! ***\n");
    gotoxy(0, 10);
    printf("  You have been eliminated in combat.\n");
    gotoxy(0, 12);
    printf("  Rejoin the game? (Y/N): ");
}

void display_show_error(const char *error) {
    clrscr();
    gotoxy(0, 10);
    printf("ERROR: %s\n", error);
}
