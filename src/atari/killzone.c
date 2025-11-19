/**
 * KillZone Atari 8-bit Client - Main Game Loop
 * 
 * Phase 3: Movement & Real-Time Synchronization
 * - Keyboard input handling
 * - Player movement submission
 * - World state polling
 * - Combat handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>

#include "network.h"
#include "state.h"
#include "display.h"
#include "json.h"
#include "parser.h"

/* Game constants */
#define GAME_TITLE "KillZone"
#define PLAYER_NAME_MAX 32

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
    int16_t bytes_read;
    static int attempt = 0;
    static int welcome_shown = 0;
    
    /* Show welcome screen once */
    if (!welcome_shown) {
        display_show_welcome(SERVER_HOST);
        welcome_shown = 1;
    }
    
    bytes_read = kz_network_health_check(response_buffer, RESPONSE_BUFFER_SIZE);
    
    if (bytes_read > 0) {
        /* Check for "status":"healthy" in health check response */
        if (strstr((const char *)response_buffer, "healthy") != NULL) {
            state_set_current(STATE_JOINING);
        } else {
            state_set_error("Server response invalid");
            state_set_current(STATE_ERROR);
        }
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
    int16_t bytes_read;
    size_t len;
    int i;
    const player_state_t *existing_player;
    
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
            bytes_read = kz_network_join_player(player_name, response_buffer, RESPONSE_BUFFER_SIZE);
            
            if (bytes_read > 0) {
                parse_join_response(response_buffer, (uint16_t)bytes_read);
                
                if (json_is_success((const char *)response_buffer)) {
                    state_set_current(STATE_PLAYING);
                } else {
                    state_set_error("Rejoin failed");
                    state_set_current(STATE_ERROR);
                }
            } else {
                state_set_error("Rejoin request failed");
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
    
    fflush(stdout);
    fgets(player_name, sizeof(player_name), stdin);
    
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
    
    bytes_read = kz_network_join_player(player_name, response_buffer, RESPONSE_BUFFER_SIZE);
    
    if (bytes_read > 0) {
        parse_join_response(response_buffer, (uint16_t)bytes_read);
        
        if (json_is_success((const char *)response_buffer)) {
            state_set_current(STATE_PLAYING);
        } else {
            state_set_error("Server rejected join");
            state_set_current(STATE_ERROR);
        }
    } else {
        /* bytes_read <= 0 means network error or timeout */
        state_set_error("Network timeout");
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
    int16_t bytes_read;
    int c;
    const char *direction = NULL;
    player_state_t *player;
    uint8_t player_count;
    uint8_t x, y, i;
    const player_state_t *others;
    char loser_id[32];
    const char *loser_start;
    uint32_t new_x, new_y;
    char entity_char;
    const char *status;
    
    /* Get world state periodically (every 5 frames) */
    if (frame_count++ % 5 == 0) {
        bytes_read = kz_network_get_world_state(response_buffer, RESPONSE_BUFFER_SIZE);
        
        if (bytes_read > 0) {
            parse_world_state(response_buffer, (uint16_t)bytes_read);
            
            /* Check if player is still alive */
            player = (player_state_t *)state_get_local_player();
            if (player && player->id[0] != '\0') {
                if (!parser_is_player_in_world(response_buffer, player->id)) {
                    /* Player is dead */
                    state_set_current(STATE_DEAD);
                }
            }
        }
    }
    
    /* Render game world */
    player = (player_state_t *)state_get_local_player();
    if (player && player->x < 255 && player->y < 255) {
        /* Full redraw on first render or when player count changes (rejoin detection) or when refresh requested */
        static int world_rendered = 0;
        static uint16_t last_wall_count = 0;
        uint16_t current_wall_count;
        others = state_get_other_players(&player_count);
        state_get_walls(&current_wall_count);
        
        /* Check if refresh was requested */
        if (force_screen_refresh) {
            world_rendered = 0;
            force_screen_refresh = 0;
        }
        
        /* Check for level change (wall count changed) */
        if (current_wall_count != last_wall_count) {
            world_rendered = 0;
            last_wall_count = current_wall_count;
        }
        
        if (!world_rendered || (last_other_count != 255 && last_other_count != player_count)) {
            /* Use display module for full redraw */
            display_draw_world();
            display_update();
            
            last_player_x = player->x;
            last_player_y = player->y;
            last_other_count = player_count;
            world_rendered = 1;
            
            /* Initialize last positions for incremental updates */
            for (i = 0; i < player_count; i++) {
                last_other_positions[i * 2] = others[i].x;
                last_other_positions[i * 2 + 1] = others[i].y;
            }
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
    if (kbhit()) {
        c = cgetc();
        player = (player_state_t *)state_get_local_player();
        
        if (!player) {
            return;
        }
        
        /* Parse movement commands */
        switch (c) {
            case 'w':
            case 'W':
            case 28:   /* Atari up arrow */
                direction = "up";
                break;
            case 's':
            case 'S':
            case 29:   /* Atari down arrow */
                direction = "down";
                break;
            case 'a':
            case 'A':
            case 30:   /* Atari left arrow */
                direction = "left";
                break;
            case 'd':
            case 'D':
            case 31:   /* Atari right arrow */
                direction = "right";
                break;
            case 'L':
                /* Next Level command */
                bytes_read = kz_network_next_level(response_buffer, RESPONSE_BUFFER_SIZE);
                if (bytes_read > 0) {
                    /* Parse new state (including new walls) */
                    parse_world_state(response_buffer, (uint16_t)bytes_read);
                    /* Force redraw to show new level */
                    force_screen_refresh = 1;
                }
                return;
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
                c = cgetc();
                if (c == 'y' || c == 'Y') {
                    /* Really quit - leave player and go to init */
                    kz_network_leave_player(player->id, response_buffer, RESPONSE_BUFFER_SIZE);
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
            bytes_read = kz_network_move_player(player->id, direction, response_buffer, RESPONSE_BUFFER_SIZE);
            
            if (bytes_read > 0) {
                /* Check if player was disconnected (404 error - player not found) */
                if (strstr((const char *)response_buffer, "\"error\":\"Player not found\"") != NULL) {
                    /* Player was cleaned up on server - mark as disconnected */
                    state_set_connected(0);
                    
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
                
                /* Parse new position from response */
                if (json_get_uint((const char *)response_buffer, "x", &new_x) && 
                    json_get_uint((const char *)response_buffer, "y", &new_y)) {
                    /* Update local player position */
                    player->x = (uint8_t)new_x;
                    player->y = (uint8_t)new_y;
                }
                
                /* Check if combat occurred */
                if (strstr((const char *)response_buffer, "\"collision\":true") != NULL) {
                    /* Combat occurred - parse combat result */
                    const char *msg_start;
                    const char *msg_end;
                    char message[41];
                    int msg_num;
                    
                    /* Display combat messages from combatResult.messages array */
                    msg_start = strstr((const char *)response_buffer, "\"messages\":[");
                    if (msg_start) {
                        msg_start += 12; /* Skip "messages":[ */
                        
                        /* Parse and display up to 4 messages (3 rounds + final) */
                        for (msg_num = 0; msg_num < 4 && *msg_start && *msg_start != ']'; msg_num++) {
                            /* Find opening quote */
                            msg_start = strchr(msg_start, '"');
                            if (!msg_start) break;
                            msg_start++;
                            
                            /* Find closing quote */
                            msg_end = strchr(msg_start, '"');
                            if (!msg_end) break;
                            
                            /* Extract message */
                            i = 0;
                            while (i < 40 && msg_start < msg_end) {
                                message[i] = *msg_start;
                                msg_start++;
                                i++;
                            }
                            message[i] = '\0';
                            
                            /* Display message on fixed line */
                            display_draw_combat_message(message);
                            
                            /* Pause briefly so player can read it */
                            {
                                int delay;
                                for (delay = 0; delay < 15000; delay++) {
                                    /* Busy wait */
                                }
                            }
                            
                            /* Move to next message */
                            msg_start = strchr(msg_end, ',');
                            if (!msg_start) {
                                msg_start = strchr(msg_end, ']');
                                if (!msg_start) break;
                            }
                        }
                    }
                    
                    /* Check if we lost */
                    loser_start = strstr((const char *)response_buffer, "\"finalLoserId\":\"");
                    if (loser_start) {
                        loser_start += 16; /* Skip past "\"finalLoserId\":\"" */
                        i = 0;
                        while (i < 31 && loser_start[i] && loser_start[i] != '"') {
                            loser_id[i] = loser_start[i];
                            i++;
                        }
                        loser_id[i] = '\0';
                        
                        /* If we are the loser, transition to dead state */
                        if (strcmp(loser_id, player->id) == 0) {
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
    clrscr();
    gotoxy(0, 10);
    printf("ERROR: %s\n", state_get_error());
    printf("Press any key to continue...\n");
    cgetc();
}
