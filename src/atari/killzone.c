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
void parse_join_response(const uint8_t *response, uint16_t len);
void parse_world_state(const uint8_t *response, uint16_t len);

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
    static int prompt_shown = 0;
    
    /* Show prompt once */
    if (!prompt_shown) {
        clrscr();
        gotoxy(0, 5);
        printf("Enter player name:\n");
        gotoxy(0, 7);
        prompt_shown = 1;
    }
    
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
            state_set_error("Join request failed");
            state_set_current(STATE_ERROR);
        }
    } else {
        state_set_error("Join request failed");
        state_set_current(STATE_ERROR);
    }
}

/**
 * Handle STATE_PLAYING
 * 
 * Main gameplay loop with input and movement
 */
void handle_state_playing(void) {
    static int frame_count = 0;
    static uint8_t last_player_x = 255;
    static uint8_t last_player_y = 255;
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
    
    /* Get world state periodically (every 5 frames) */
    if (frame_count++ % 5 == 0) {
        bytes_read = kz_network_get_world_state(response_buffer, RESPONSE_BUFFER_SIZE);
        
        if (bytes_read > 0) {
            parse_world_state(response_buffer, (uint16_t)bytes_read);
        }
    }
    
    /* Render game world only when player position changes (and we have a valid position) */
    player = (player_state_t *)state_get_local_player();
    if (player && player->x < 255 && player->y < 255 && (player->x != last_player_x || player->y != last_player_y)) {
        clrscr();
        
        /* Draw world line by line */
        for (y = 0; y < DISPLAY_HEIGHT; y++) {
            gotoxy(0, y);
            for (x = 0; x < DISPLAY_WIDTH; x++) {
                printf(".");
            }
        }
        
        /* Draw other players as * */
        others = state_get_other_players(&player_count);
        for (i = 0; i < player_count; i++) {
            if (others[i].x < DISPLAY_WIDTH && others[i].y < DISPLAY_HEIGHT) {
                gotoxy(others[i].x, others[i].y);
                printf("*");
            }
        }
        
        /* Draw local player as @ (last so it appears on top) */
        if (player->x < DISPLAY_WIDTH && player->y < DISPLAY_HEIGHT) {
            gotoxy(player->x, player->y);
            printf("@");
        }
        
        last_player_x = player->x;
        last_player_y = player->y;
    }
    
    /* Display status bar periodically (every 10 frames) */
    if (frame_count % 10 == 0) {
        player = (player_state_t *)state_get_local_player();
        if (player) {
            const player_state_t *others = state_get_other_players(&player_count);
            player_count++;  /* Include self */
            display_draw_status_bar(player->name, player_count, "CONNECTED", state_get_world_ticks());
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
            case 'k':  /* vi-style up */
            case 28:   /* Atari up arrow */
                direction = "up";
                break;
            case 's':
            case 'S':
            case 'j':  /* vi-style down */
            case 29:   /* Atari down arrow */
                direction = "down";
                break;
            case 'a':
            case 'A':
            case 'h':  /* vi-style left */
            case 30:   /* Atari left arrow */
                direction = "left";
                break;
            case 'd':
            case 'D':
            case 'l':  /* vi-style right */
            case 31:   /* Atari right arrow */
                direction = "right";
                break;
            case 'q':
            case 'Q':
                kz_network_leave_player(player->id, response_buffer, RESPONSE_BUFFER_SIZE);
                state_set_current(STATE_INIT);
                return;
            default:
                break;
        }
        
        /* Send movement command if valid */
        if (direction) {
            bytes_read = kz_network_move_player(player->id, direction, response_buffer, RESPONSE_BUFFER_SIZE);
            
            if (bytes_read > 0) {
                /* Parse new position from response */
                if (json_get_uint((const char *)response_buffer, "x", &new_x) && 
                    json_get_uint((const char *)response_buffer, "y", &new_y)) {
                    /* Update local player position */
                    player->x = (uint8_t)new_x;
                    player->y = (uint8_t)new_y;
                }
                
                /* Check if combat occurred */
                if (strstr((const char *)response_buffer, "\"collision\":true") != NULL) {
                    /* Combat occurred - check if we lost */
                    if (strstr((const char *)response_buffer, "\"loserId\":\"") != NULL) {
                        /* Extract loser ID from response */
                        loser_start = strstr((const char *)response_buffer, "\"loserId\":\"");
                        if (loser_start) {
                            loser_start += 11; /* Skip past "\"loserId\":\"" */
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
    
}

/**
 * Handle STATE_DEAD
 * 
 * Player has been eliminated, offer to rejoin
 */
void handle_state_dead(void) {
    int c;
    
    clrscr();
    gotoxy(0, 10);
    printf("You have been eliminated!\n");
    gotoxy(0, 12);
    printf("Rejoin? (y/n): ");
    
    c = getchar();
    if (c == 'y' || c == 'Y') {
        state_set_current(STATE_JOINING);
    } else {
        state_set_current(STATE_ERROR);
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
}

/**
 * Parse join response JSON
 */
void parse_join_response(const uint8_t *response, uint16_t len) {
    char player_id[32];
    char player_name[32];
    uint32_t x, y, health;
    player_state_t player;
    const char *json;
    
    if (!response || len == 0) {
        return;
    }
    
    json = (const char *)response;
    
    /* Extract player ID */
    if (json_get_string(json, "id", player_id, sizeof(player_id)) == 0) {
        return;
    }
    
    /* Extract player name if available, otherwise use ID */
    if (json_get_string(json, "name", player_name, sizeof(player_name)) == 0) {
        strncpy(player_name, player_id, sizeof(player_name) - 1);
    }
    
    /* Extract position */
    if (!json_get_uint(json, "x", &x) || !json_get_uint(json, "y", &y)) {
        return;
    }
    
    /* Extract health */
    if (!json_get_uint(json, "health", &health)) {
        health = 100;
    }
    
    /* Create player state */
    strncpy(player.id, player_id, sizeof(player.id) - 1);
    strncpy(player.name, player_name, sizeof(player.name) - 1);
    player.x = (uint8_t)x;
    player.y = (uint8_t)y;
    player.health = (uint8_t)health;
    strcpy(player.status, "alive");
    
    /* Store in state */
    state_set_local_player(&player);
}

/**
 * Parse world state JSON
 */
void parse_world_state(const uint8_t *response, uint16_t len) {
    uint32_t width, height, ticks;
    const char *json;
    
    if (!response || len == 0) {
        return;
    }
    
    json = (const char *)response;
    
    /* Extract world dimensions */
    if (json_get_uint(json, "width", &width) && json_get_uint(json, "height", &height)) {
        state_set_world_dimensions((uint8_t)width, (uint8_t)height);
    }
    
    /* Extract world ticks */
    if (json_get_uint(json, "ticks", &ticks)) {
        state_set_world_ticks((uint16_t)ticks);
    }
    
    /* TODO: Parse player array from JSON */
}
