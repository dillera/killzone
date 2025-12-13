/**
 * KillZone 8-bit Client - Main Game Loop
 * 
 * Phase 3: Movement & Real-Time Synchronization
 * - Keyboard input handling
 * - Player movement submission
 * - World state polling
 * - Combat handling
 */

#ifdef _CMOC_VERSION_
#include <cmoc.h>
#include <coco.h>
#include "conio_wrapper.h"
#include "snprintf.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#endif

#include "network.h"
#include "state.h"
#include "display.h"

/* Game constants */
#define GAME_TITLE "KillZone"
#define PLAYER_NAME_MAX 32
#define RESPONSE_BUFFER_SIZE 1024

/* Function declarations */
void game_init(void);
void game_loop(void);
void game_close(void);
void handle_state_init(void);
void handle_state_connecting(void);
void handle_state_joining(void);
void handle_state_playing(void);
void handle_state_dead(void);
void handle_state_error(void);

/* Global response buffer */
static uint8_t response_buffer[RESPONSE_BUFFER_SIZE];

/* Flag to track if welcome screen has been shown */
static int welcome_shown = 0;

/**
 * Main entry point
 */
int main(void) {
    game_init();
    game_loop();
    game_close();
    return 0;
}

/**
 * Initialize game systems
 */
void game_init(void) {
    state_init();
    display_init();

    if (kz_network_init() != 0) {
        state_set_error("Network initialization failed");
        state_set_current(STATE_ERROR);
    } else {
        state_set_current(STATE_CONNECTING);
    }
}

/**
 * Close game systems
 */
void game_close(void) {
    display_close();
    kz_network_close();
    state_close();
#ifdef _CMOC_VERSION_
    hirestxt_close();
#endif
}

/**
 * Main game loop
 */
void game_loop(void) {
    int running = 1;
    int frame_count = 0;
    client_state_t current;
    
    while (running && frame_count < 100000) {
        current = state_get_current();
        frame_count++;
        
        switch (current) {
            case STATE_INIT:
                handle_state_init();
                break;
            case STATE_CONNECTING:
                handle_state_connecting();
                break;
            case STATE_JOINING:
                handle_state_joining();
                break;
            case STATE_PLAYING:
                handle_state_playing();
                break;
            case STATE_DEAD:
                handle_state_dead();
                break;
            case STATE_ERROR:
                handle_state_error();
                running = 0;
                break;
            default:
                running = 0;
                break;
        }
    }
    
}

/**
 * Handle STATE_INIT
 */
void handle_state_init(void) {
    state_set_current(STATE_CONNECTING);
}

/**
 * Handle STATE_CONNECTING
 * 
 * Attempt to connect to server and verify it's running
 */
void handle_state_connecting(void) {
    static int attempt = 0;
    
    /* Show welcome screen once */
    if (!welcome_shown) {
        display_show_welcome(SERVER_HOST);
        welcome_shown = 1;
    }
    
    if (kz_network_health_check()) {
        state_set_current(STATE_JOINING);
    } else if (++attempt > 10) {
        /* Give up after 10 attempts */
        state_set_error("Server not responding");
        state_set_current(STATE_ERROR);
    }
}

/**
 * Handle STATE_JOINING
 * 
 * Join the game world with a player name
 */
void handle_state_joining(void) {
    char player_name[PLAYER_NAME_MAX];
    size_t len;
    int i;
    const player_state_t *existing_player;
    player_state_t new_player;
    
    /* Check if we're rejoining with existing player name */
    if (state_is_rejoining()) {
        existing_player = state_get_local_player();
        if (existing_player && existing_player->name[0] != '\0') {
            /* Use existing player name for automatic rejoin */
            strncpy(player_name, existing_player->name, sizeof(player_name) - 1);
            player_name[sizeof(player_name) - 1] = '\0';
            
            /* Show rejoining message */
            clrscr();
            gotoxy(0, 8);
            printf("  Rejoining as: %s\n", player_name);
            gotoxy(0, 10);
            printf("  Please wait...\n");
            
            /* Clear rejoining flag */
            state_set_rejoining(0);
            
            /* Send join request immediately */
            if (kz_network_join_player(player_name, &new_player)) {
                state_set_current(STATE_PLAYING);
            } else {
                state_set_error("Rejoin failed");
                state_set_current(STATE_ERROR);
            }
            return;
        }
    }
    
    /* Normal join flow - prompt for name */
    clrscr();
    gotoxy(0, 5);
    printf("Enter player name:\n");
    gotoxy(0, 7);
    
#ifdef _CMOC_VERSION_
    get_line(player_name, sizeof(player_name));
#else
    fflush(stdout);
    fgets(player_name, sizeof(player_name), stdin);
#endif
    /* Remove newline and carriage return */
    len = strlen(player_name);
    if (len > 0 && player_name[len - 1] == '\n') {
        player_name[len - 1] = '\0';
        len--;
    }
    if (len > 0 && player_name[len - 1] == '\r') {
        player_name[len - 1] = '\0';
        len--;
    }
    
    /* Trim whitespace */
    for (i = 0; i < len; i++) {
        if (player_name[i] == ' ' || player_name[i] == '\t') {
            player_name[i] = '\0';
            break;
        }
    }
    
    if (strlen(player_name) == 0) {
        strcpy(player_name, "Player");
    }
    
    printf("Joining as: %s\n", player_name);
    if (kz_network_join_player(player_name, &new_player)) {
        state_set_current(STATE_PLAYING);
    } else {
        state_set_error("Server rejected join");
        state_set_current(STATE_ERROR);
    }
}

/**
 * Handle STATE_PLAYING
 * 
 * Main gameplay loop with input and movement
 */
/* Module-level flag for screen refresh */
static int force_screen_refresh = 0;

void handle_state_playing(void) {
    static int frame_count = 0;
    static uint8_t last_player_x = 255;
    static uint8_t last_player_y = 255;
    static uint8_t last_other_positions[MAX_OTHER_PLAYERS * 2];  /* x,y pairs */
    static uint8_t last_other_count = 255;  /* Track previous player count for rejoin detection */
    int c;
    const char *direction = NULL;
    player_state_t *player;
    uint8_t player_count;
    uint8_t x, y, i;
    const player_state_t *others;
    char entity_char;
    const char *status;
    move_result_t move_res;
    
    /* Get world state periodically (every 5 frames) */
    if (frame_count++ % 5 == 0) {
        if (!kz_network_get_world_state()) {
            /* Optional: handle network error during update */
        }
    }
    
    /* Render game world */
    player = (player_state_t *)state_get_local_player();
    if (player && player->x < 255 && player->y < 255) {
        /* Full redraw on first render or when player count changes (rejoin detection) or when refresh requested */
        static int world_rendered = 0;
        others = state_get_other_players(&player_count);
        
        /* Check if refresh was requested */
        if (force_screen_refresh) {
            world_rendered = 0;
            force_screen_refresh = 0;
        }
        
        if (!world_rendered || (last_other_count != 255 && last_other_count != player_count)) {
            clrscr();
            
            /* Draw world line by line */
            for (y = 0; y < DISPLAY_HEIGHT; y++) {
                gotoxy(0, y);
                for (x = 0; x < DISPLAY_WIDTH; x++) {
                    printf(".");
                }
            }
            
            /* Draw other entities - # for players, ^ for hunter mobs, * for regular mobs */
            for (i = 0; i < player_count; i++) {
                if (others[i].x < DISPLAY_WIDTH && others[i].y < DISPLAY_HEIGHT) {
                    gotoxy(others[i].x, others[i].y);
                    /* Use # for real players, ^ for hunter mobs, * for regular mobs */
                    if (strcmp(others[i].type, "player") == 0) {
                        entity_char = '#';
                    } else if (others[i].isHunter) {
                        entity_char = '^';
                    } else {
                        entity_char = '*';
                    }
                    printf("%c", entity_char);
                }
            }
            
            /* Draw local player as @ */
            if (player->x < DISPLAY_WIDTH && player->y < DISPLAY_HEIGHT) {
                gotoxy(player->x, player->y);
                printf("@");
            }
            
            last_player_x = player->x;
            last_player_y = player->y;
            last_other_count = player_count;
            world_rendered = 1;
        } else if (player->x != last_player_x || player->y != last_player_y) {
            /* Incremental update: only redraw changed positions */
            
            /* Erase old player position */
            gotoxy(last_player_x, last_player_y);
            printf(".");
            
            /* Draw new player position */
            gotoxy(player->x, player->y);
            printf("@");
            
            last_player_x = player->x;
            last_player_y = player->y;
        }
        
        /* Update other players incrementally */
        for (i = 0; i < player_count; i++) {
            uint8_t old_x = last_other_positions[i * 2];
            uint8_t old_y = last_other_positions[i * 2 + 1];
            uint8_t new_x_other = others[i].x;
            uint8_t new_y_other = others[i].y;
            
            /* If position changed, update it */
            if (old_x != new_x_other || old_y != new_y_other) {
                /* Erase old position (if it was valid) */
                if (old_x < DISPLAY_WIDTH && old_y < DISPLAY_HEIGHT && (old_x != 0 || old_y != 0)) {
                    gotoxy(old_x, old_y);
                    printf(".");
                }
                
                /* Draw new position - # for players, ^ for hunter mobs, * for regular mobs */
                if (new_x_other < DISPLAY_WIDTH && new_y_other < DISPLAY_HEIGHT) {
                    gotoxy(new_x_other, new_y_other);
                    if (strcmp(others[i].type, "player") == 0) {
                        entity_char = '#';
                    } else if (others[i].isHunter) {
                        entity_char = '^';
                    } else {
                        entity_char = '*';
                    }
                    printf("%c", entity_char);
                }
                
                /* Update tracked position */
                last_other_positions[i * 2] = new_x_other;
                last_other_positions[i * 2 + 1] = new_y_other;
            }
        }
    }
    
    /* Display status bar periodically (every 10 frames) */
    if (frame_count % 10 == 0) {
        player = (player_state_t *)state_get_local_player();
        if (player) {
            others = state_get_other_players(&player_count);
            player_count++;  /* Include self */
            status = state_is_connected() ? "CONNECTED" : "DISCONNECTED";
            display_draw_status_bar(player->name, player_count, status, state_get_world_ticks());
        }
    }
    
    /* Check for keyboard input (non-blocking) */
#ifdef _CMOC_VERSION_
    c = cgetc();
    if (c) {
#else
    if (kbhit()) {
        c = cgetc();
#endif
        player = (player_state_t *)state_get_local_player();
        
        if (!player) {
            return;
        }
        
        /* Parse movement commands */
        switch (c) {
            case 'w':
            case 'W':
            case 'k':  /* vi-style up */
            case 28:   /* Atari up arrow */
                direction = "up";
                break;
            case 's':
            case 'S':
            case 'j':  /* vi-style down */
            case 'J':
            case 29:   /* Atari down arrow */
                direction = "down";
                break;
            case 'a':
            case 'A':
            case 'h':  /* vi-style left */
            case 'H':
            case 30:   /* Atari left arrow */
                direction = "left";
                break;
            case 'd':
            case 'D':
            case 'l':  /* vi-style right */
            case 'L':
            case 31:   /* Atari right arrow */
                direction = "right";
                break;
            case 'r':
            case 'R':
                /* Trigger full screen redraw */
                force_screen_refresh = 1;
                return;
            case 'q':
            case 'Q':
                /* Show quit confirmation dialog */
                clrscr();
                gotoxy(0, 8);
                printf("  Are you sure you want to quit?\n");
                gotoxy(0, 10);
                printf("  Y=Quit  N=Continue Playing\n");
                gotoxy(0, 12);
                printf("  Press a key: ");
                
                /* Wait for confirmation */
#ifdef _CMOC_VERSION_
                c = waitkey(0);
#else
                c = cgetc();
#endif
                if (c == 'y' || c == 'Y') {
                    /* Really quit - leave player and go to init */
                    kz_network_leave_player(player->id);
                    state_clear_local_player();
                    state_set_rejoining(0);
                    state_set_current(STATE_INIT);
                } else if (c == 'n' || c == 'N') {
                    /* Don't quit - rejoin with saved name */
                    state_set_rejoining(1);
                    state_clear_other_players();
                    state_set_current(STATE_JOINING);
                }
                return;
            default:
                break;
        }
        
        /* Send movement command if valid */
        if (direction) {
            if (!kz_network_move_player(player->id, direction, &move_res)) {
                /* If move failed (e.g. network error) */
                if (!state_is_connected()) {
                    /* Show disconnection dialog */
                    clrscr();
                    gotoxy(0, 8);
                    printf("  CONNECTION LOST\n");
                    gotoxy(0, 10);
                    printf("  You were disconnected from the server.\n");
                    gotoxy(0, 12);
                    printf("  Y=Quit  N=Rejoin\n");
                    gotoxy(0, 14);
                    printf("  Press a key: ");
                    
                    /* Wait for confirmation */
                    c = cgetc();
                    if (c == 'y' || c == 'Y') {
                        /* Really quit - go to init */
                        state_clear_local_player();
                        state_set_rejoining(0);
                        state_set_connected(0);
                        state_set_current(STATE_INIT);
                    } else if (c == 'n' || c == 'N') {
                        /* Rejoin with saved name */
                        state_set_rejoining(1);
                        state_set_connected(0);
                        state_clear_other_players();
                        state_set_current(STATE_JOINING);
                    }
                    return;
                }
            } else {
                /* Update local player position from response */
                player->x = move_res.x;
                player->y = move_res.y;
                
                /* Check if combat occurred */
                if (move_res.collision) {
                    int msg_num;
                    
                    /* Display combat messages */
                    for (msg_num = 0; msg_num < move_res.message_count; msg_num++) {
                        /* Display message on fixed line */
                        display_draw_combat_message(move_res.messages[msg_num]);
                        
                        /* Pause briefly so player can read it */
                        {
                            int delay;
                            for (delay = 0; delay < 15000; delay++) {
                                /* Busy wait */
                            }
                        }
                    }
                    
                    /* Check if we lost */
                    if (move_res.loser_id[0] != '\0') {
                        /* If we are the loser, transition to dead state */
                        if (strcmp(move_res.loser_id, player->id) == 0) {
                            state_set_current(STATE_DEAD);
                        }
                    }
                }
            }
        }
    }    
}

/**
 * Handle STATE_DEAD
 * 
 * Player has been eliminated, offer to rejoin
 */
void handle_state_dead(void) {
    static int shown = 0;
    int c;
    
    /* Show death message once */
    if (!shown) {
        clrscr();
        gotoxy(0, 8);
        printf("  *** YOU WERE KILLED! ***\n");
        gotoxy(0, 10);
        printf("  You have been eliminated in combat.\n");
        gotoxy(0, 12);
        printf("  Rejoin the game? (Y/N): ");
        shown = 1;
    }
    
    /* Check for input */
    if (kbhit()) {
        c = cgetc();
        if (c == 'y' || c == 'Y') {
            shown = 0;
            /* Set rejoining flag - this tells STATE_JOINING to use saved name */
            state_set_rejoining(1);
            /* Keep player name and clear other state for rejoin */
            state_clear_other_players();
            /* Server will restore same player ID using saved name */
            state_set_current(STATE_JOINING);
        } else if (c == 'n' || c == 'N') {
            shown = 0;
            state_clear_local_player();
            state_set_rejoining(0);
            state_set_current(STATE_INIT);
        }
    }
}

/**
 * Handle STATE_ERROR
 * 
 * Error state - game ends
 */
void handle_state_error(void) {
#ifdef _CMOC_VERSION_
    hirestxt_close();
#endif
    clrscr();
    gotoxy(0, 10);
    printf("ERROR: %s\n", state_get_error());
}
