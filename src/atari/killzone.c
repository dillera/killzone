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
            
            /* Draw walls using ATASCII characters */
            {
                uint16_t wall_count;
                const wall_t *walls = state_get_walls(&wall_count);
                for (i = 0; i < wall_count; i++) {
                    if (walls[i].x < DISPLAY_WIDTH && walls[i].y < DISPLAY_HEIGHT) {
                        gotoxy(walls[i].x, walls[i].y);
                        printf("%c", CHAR_WALL);
                    }
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
 * Parse walls from JSON response
 * Extracts wall positions from walls array
 */
void parse_walls_from_response(const uint8_t *response, uint16_t len) {
    const char *json = (const char *)response;
    const char *array_pos;
    const char *x_start;
    const char *y_start;
    const char *next_obj;
    uint32_t x_val, y_val;
    uint16_t count = 0;
    static wall_t temp_walls[MAX_WALLS];
    
    if (!response || len == 0) {
        return;
    }
    
    /* Find walls array */
    array_pos = strstr(json, "\"walls\":[");
    if (!array_pos) {
        state_clear_walls();  /* No walls in response */
        return;
    }
    
    array_pos += 9;  /* Skip "walls":[ */
    
    /* Parse each wall object */
    while (array_pos && count < MAX_WALLS) {
        x_start = strstr(array_pos, "\"x\":");
        y_start = strstr(array_pos, "\"y\":");
        next_obj = strchr(array_pos, '}');
        
        if (!x_start || !y_start || !next_obj) break;
        if (x_start > next_obj || y_start > next_obj) break;
        
        /* Extract x and y */
        x_val = (uint32_t)strtol(x_start + 4, NULL, 10);
        y_val = (uint32_t)strtol(y_start + 4, NULL, 10);
        
        /* Store wall */
        temp_walls[count].x = (uint8_t)x_val;
        temp_walls[count].y = (uint8_t)y_val;
        count++;
        
        array_pos = next_obj + 1;
    }
    
    /* Update state with walls */
    state_set_walls(temp_walls, count);
}

/**
 * Parse entities from players array in JSON
 * Extracts x,y coordinates for each entity
 */
void parse_entities_from_response(const uint8_t *response, uint16_t len) {
    const char *json = (const char *)response;
    const char *array_pos;
    const char *id_start;
    const char *x_start;
    const char *y_start;
    const char *type_start;
    const char *hunter_start;
    const char *next_obj;
    const char *id_val;
    const char *type_val;
    uint8_t count = 0;
    player_state_t *local = (player_state_t *)state_get_local_player();
    char id_buf[32];
    char type_buf[8];
    uint32_t x_val, y_val;
    int i;
    int is_hunter;
    static player_state_t other_players[MAX_OTHER_PLAYERS];
    
    /* Find players array */
    array_pos = strstr(json, "\"players\":[");
    if (!array_pos) return;
    
    array_pos += 11; /* Skip "players":[ */
    
    /* Parse each entity - look for {"id":"...", "x":N, "y":N, "type":"..."} patterns */
    while (count < MAX_OTHER_PLAYERS && *array_pos && *array_pos != ']') {
        id_start = strstr(array_pos, "\"id\":\"");
        x_start = strstr(array_pos, "\"x\":");
        y_start = strstr(array_pos, "\"y\":");
        type_start = strstr(array_pos, "\"type\":\"");
        next_obj = strchr(array_pos, '}');
        
        if (!id_start || !x_start || !y_start || !next_obj) break;
        if (id_start > next_obj) break; /* id not in this object */
        
        /* Extract id */
        id_val = id_start + 6;
        i = 0;
        while (i < 31 && id_val[i] && id_val[i] != '"') {
            id_buf[i] = id_val[i];
            i++;
        }
        id_buf[i] = '\0';
        
        /* Skip local player */
        if (local && strcmp(id_buf, local->id) == 0) {
            array_pos = next_obj + 1;
            continue;
        }
        
        /* Extract x */
        x_val = (uint32_t)strtol(x_start + 4, NULL, 10);
        
        /* Extract y */
        y_val = (uint32_t)strtol(y_start + 4, NULL, 10);
        
        /* Extract type (default to "mob" if not found) */
        strcpy(type_buf, "mob");
        if (type_start && type_start < next_obj) {
            type_val = type_start + 8;
            i = 0;
            while (i < 7 && type_val[i] && type_val[i] != '"') {
                type_buf[i] = type_val[i];
                i++;
            }
            type_buf[i] = '\0';
        }
        
        /* Extract isHunter flag (default to 0 if not found) */
        is_hunter = 0;
        hunter_start = strstr(array_pos, "\"isHunter\":");
        if (hunter_start && hunter_start < next_obj) {
            hunter_start += 11;  /* Skip "isHunter": */
            if (strstr(hunter_start, "true") && strstr(hunter_start, "true") < next_obj) {
                is_hunter = 1;
            }
        }
        
        /* Store entity */
        strncpy(other_players[count].id, id_buf, sizeof(other_players[count].id) - 1);
        other_players[count].x = (uint8_t)x_val;
        other_players[count].y = (uint8_t)y_val;
        other_players[count].health = 100;
        strcpy(other_players[count].status, "alive");
        strcpy(other_players[count].type, type_buf);
        other_players[count].isHunter = is_hunter;
        count++;
        
        array_pos = next_obj + 1;
    }
    
    /* Update state with other players */
    state_set_other_players(other_players, count);
}

/**
 * Parse world state JSON - extract entities from players array
 */
void parse_world_state(const uint8_t *response, uint16_t len) {
    uint32_t width, height, ticks;
    const char *json;
    static char kill_msg[41];
    const player_state_t *local_player;
    uint8_t other_count;
    int found_self = 0;
    int c;
    char search_str[64];
    const char *our_id_search;
    
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
    
    /* Extract and display lastKillMessage if present */
    if (json_get_string(json, "lastKillMessage", kill_msg, sizeof(kill_msg)) > 0) {
        display_draw_combat_message(kill_msg);
    }
    
    /* Parse walls from response */
    parse_walls_from_response(response, len);
    
    /* Parse entities from players array */
    parse_entities_from_response(response, len);
    
    /* Check if local player is still alive (present in world state) */
    local_player = state_get_local_player();
    if (local_player && local_player->id[0] != '\0') {
        /* Check if we're in the other_players list (we exclude ourselves during parsing) */
        const player_state_t *others = state_get_other_players(&other_count);
        
        /* If we're a player type, we should NOT be in other_players (we skip ourselves)
           But if we're dead, we won't be in the world at all.
           Check by looking at the raw JSON for our player ID */
        our_id_search = strstr(json, "\"players\":[");
        if (our_id_search) {
            snprintf(search_str, sizeof(search_str), "\"id\":\"%s\"", local_player->id);
            if (strstr(our_id_search, search_str) != NULL) {
                found_self = 1;  /* We're still in the world */
            }
        }
        
        /* If we're not found in the world state, we're dead */
        if (!found_self && state_get_current() == STATE_PLAYING) {
            /* Player is dead - show death dialog */
            clrscr();
            gotoxy(0, 8);
            printf("  YOU DIED!\n");
            gotoxy(0, 10);
            printf("  Play again?\n");
            gotoxy(0, 12);
            printf("  Y=Rejoin  N=Quit\n");
            gotoxy(0, 14);
            printf("  Press a key: ");
            
            c = cgetc();
            if (c == 'y' || c == 'Y') {
                /* Rejoin with saved name */
                state_set_rejoining(1);
                state_clear_other_players();
                state_set_current(STATE_JOINING);
            } else {
                /* Quit to main menu */
                state_clear_local_player();
                state_set_rejoining(0);
                state_set_current(STATE_INIT);
            }
        }
    }
}
