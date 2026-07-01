/**
 * KillZone Client - Main Game Loop
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
#include <time.h>
#endif

#include "network.h"
#include "state.h"
#include "display.h"
#include "input.h"
#ifdef __ATARI__
#include "atari_sound.h"
#endif
/* json.h is no longer needed as parsing is done in network.c */

#include "constants.h"

/* Function declarations */
void game_init(void);
void game_loop(void);
void game_close(void);
void handle_state_init(void);
void handle_state_splash(void);
void handle_state_connecting(void);
void handle_state_joining(void);
void handle_state_playing(void);
void handle_state_dead(void);
void handle_state_error(void);

/* Set by any successful (re)join so the next gameplay frame does a full
 * clrscr() + redraw, instead of display_render_game's incremental-update
 * path diffing against stale tracked positions left over from before the
 * death/rejoin or connection-lost screens overwrote the display. */
static int force_screen_refresh = 0;

/**
 * Pause for roughly the given number of seconds so on-screen status
 * text is actually visible. A connect attempt can fail faster than a
 * single video frame, so the state machine needs an explicit pause or
 * diagnostic text never gets rendered before being overwritten.
 */
static void kz_pause_seconds(uint8_t seconds) {
#ifdef _CMOC_VERSION_
    volatile unsigned long i;
    unsigned long iterations = (unsigned long)seconds * 200000UL;
    for (i = 0; i < iterations; i++) {
        /* busy wait */
    }
#else
    clock_t start = clock();
    clock_t target = (clock_t)seconds * CLOCKS_PER_SEC;
    while ((clock() - start) < target) {
        /* busy wait */
    }
#endif
}

/**
 * Main entry point
 */
int main(void) {
    printf("Booting KillZone...\n");
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
    input_init();
    
    if (kz_network_init() != 0) {
        state_set_error("Network initialization failed");
        state_set_current(STATE_ERROR);
    } else {
        /* Enter through the title screen so startup and quit-to-title
         * share one logical flow: SPLASH -> CONNECTING -> JOINING. */
        state_set_current(STATE_SPLASH);
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
            case STATE_SPLASH:
                handle_state_splash();
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

#ifdef __ATARI__
        atari_sound_tick();
#endif
    }

}

/**
 * Handle STATE_INIT
 */
void handle_state_init(void) {
    state_set_current(STATE_SPLASH);
}

/**
 * Handle STATE_SPLASH
 *
 * The single title screen, reached both at startup and on quit-to-title.
 * Show the KILLZONE banner and target server, play the jingle, then wait
 * for the player to press a key before attempting to connect.
 */
void handle_state_splash(void) {
    display_show_splash(SERVER_HOST, SERVER_TCP_PORT);
#ifdef __ATARI__
    /* Play the title jingle before blocking for a keypress. The melody
     * routine is self-contained and blocks via RTCLOK, and there is no
     * SIO activity yet at this point, so nothing competes for POKEY. */
    atari_sound_play_melody();
#endif
    input_wait_key();
    state_set_current(STATE_CONNECTING);
}

/**
 * Handle STATE_CONNECTING
 * 
 * Attempt to connect to server and verify it's running
 */
void handle_state_connecting(void) {
    static int attempt = 0;
    static int welcome_shown = 0;
    static char status_buf[40];

    /* Show the connect-progress screen once. The title banner and jingle
     * already played in STATE_SPLASH; this screen just reports connection
     * status while the health-check retries. */
    if (!welcome_shown) {
        display_show_welcome(SERVER_HOST);
        welcome_shown = 1;
    }

    attempt++;
    display_show_connect_status(SERVER_HOST, SERVER_TCP_PORT, attempt, "connecting...");
    kz_pause_seconds(1);

    if (kz_network_health_check()) {
        display_show_connect_status(SERVER_HOST, SERVER_TCP_PORT, attempt, "connected!");
        kz_pause_seconds(1);
        state_set_current(STATE_JOINING);
    } else if (attempt > 10) {
        /* Give up after 10 attempts */
        snprintf(status_buf, sizeof(status_buf), "timed out (err %u)", kz_network_get_last_error());
        display_show_connect_status(SERVER_HOST, SERVER_TCP_PORT, attempt, status_buf);
        kz_pause_seconds(2);
        state_set_error("Server not responding");
        state_set_current(STATE_ERROR);
    } else {
        snprintf(status_buf, sizeof(status_buf), "failed (err %u), retrying...", kz_network_get_last_error());
        display_show_connect_status(SERVER_HOST, SERVER_TCP_PORT, attempt, status_buf);
        kz_pause_seconds(1);
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
            display_show_rejoining(player_name);
            
            /* Clear rejoining flag */
            state_set_rejoining(0);
            
            /* Send join request immediately */
            if (kz_network_join_player(player_name, &new_player)) {
#ifdef __ATARI__
                atari_sound_play_join();
#endif
                force_screen_refresh = 1;
                state_set_current(STATE_PLAYING);
            } else {
                state_set_error("Rejoin failed");
                state_set_current(STATE_ERROR);
            }
            return;
        }
    }
    
    /* Normal join flow - prompt for name */
    display_show_join_prompt();
    
#ifdef _CMOC_VERSION_
    get_line(player_name, sizeof(player_name) - 1);
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
    
    if (kz_network_join_player(player_name, &new_player)) {
#ifdef __ATARI__
        atari_sound_play_join();
#endif
        force_screen_refresh = 1;
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

void handle_state_playing(void) {
    static int frame_count = 0;
    int c;
    const char *direction = NULL;
    player_state_t *player;
    uint8_t player_count;
    const player_state_t *others;
    const char *status;
    move_result_t move_res;
    input_cmd_t cmd; /* Moved declaration to top */
    
    /* Get world state periodically. Interval is a tunable constant
     * (WORLD_POLL_INTERVAL_FRAMES) since each poll is a blocking TCP
     * round-trip and the right cadence depends on real-world latency. */
    if (frame_count++ % WORLD_POLL_INTERVAL_FRAMES == 0) {
        if (!kz_network_get_world_state()) {
            /* Optional: handle network error during update */
        }
    }
    
    /* Render game world */
    player = (player_state_t *)state_get_local_player();
    if (player) {
        int do_refresh;
        others = state_get_other_players(&player_count);
        
        /* Check if refresh was requested - save and reset flag */
        do_refresh = force_screen_refresh;
        force_screen_refresh = 0;
        
        display_render_game(player, others, player_count, do_refresh);
    }
    
    /* Display status bar and combat message periodically (every 10 frames) */
    if (frame_count % 10 == 0) {
        const char *combat_msg;
        player = (player_state_t *)state_get_local_player();
        if (player) {
            others = state_get_other_players(&player_count);
            player_count++;  /* Include self */
            status = state_is_connected() ? "CONNECTED" : "DISCONNECTED";
            display_draw_status_bar(player->name, player_count, status, state_get_world_ticks());
            
            /* Display combat message, or clear the line once it expires */
            combat_msg = state_get_combat_message();
            display_draw_combat_message(combat_msg);
        }
    }
    
    /* Tick combat message counter each frame */
    state_tick_combat_message();
    
    /* Check for input */
    cmd = input_check();
    
    switch (cmd) {
        case CMD_UP:
            direction = "up";
            break;
        case CMD_DOWN:
            direction = "down";
            break;
        case CMD_LEFT:
            direction = "left";
            break;
        case CMD_RIGHT:
            direction = "right";
            break;
        case CMD_REFRESH:
            /* Trigger full screen redraw */
            force_screen_refresh = 1;
            return;
        case CMD_COLOR:
            /* change color */
            display_toggle_color_scheme();
            break;
        case CMD_QUIT:
            /* Show quit confirmation dialog */
            display_show_quit_confirmation();
            
            /* Wait for confirmation */
            c = input_wait_key();
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
    if (direction)
    {
        if (!kz_network_move_player(player->id, direction, &move_res))
        {
            /* If move failed (e.g. network error) */
            if (!state_is_connected())
            {
                /* Show disconnection dialog */
                display_show_connection_lost();

                /* Wait for confirmation */
                c = input_wait_key();
                if (c == 'y' || c == 'Y')
                {
                    /* Really quit - go to init */
                    state_clear_local_player();
                    state_set_rejoining(0);
                    state_set_connected(0);
                    state_set_current(STATE_INIT);
                }
                else if (c == 'n' || c == 'N')
                {
                    /* Rejoin with saved name */
                    state_set_rejoining(1);
                    state_set_connected(0);
                    state_clear_other_players();
                    state_set_current(STATE_JOINING);
                }
                return;
            }
        }
        else
        {
            /* Update local player position from response */
            player->x = move_res.x;
            player->y = move_res.y;

            /* Check if combat occurred - message is auto-displayed via state */
            if (move_res.collision)
            {
                /* Message already stored in state by network code */
                /* No blocking delay - continues gameplay */

                /* Check if we lost */
                if (move_res.loser_id[0] != '\0')
                {
                    /* If we are the loser, transition to dead state */
                    if (strcmp(move_res.loser_id, player->id) == 0)
                    {
#ifdef __ATARI__
                        atari_sound_play_death();
#endif
                        state_set_current(STATE_DEAD);
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
    input_cmd_t cmd; /* Moved declaration to top */
    
    /* Show death message once */
    if (!shown) {
        display_show_death_message();
        shown = 1;
    }
    
    /* Check for input */
    cmd = input_check();
    if (cmd == CMD_YES) {
        shown = 0;
        /* Set rejoining flag - this tells STATE_JOINING to use saved name */
        state_set_rejoining(1);
        /* Keep player name and clear other state for rejoin */
        state_clear_other_players();
        /* Server will restore same player ID using saved name */
        state_set_current(STATE_JOINING);
    } else if (cmd == CMD_NO) {
        shown = 0;
        state_clear_local_player();
        state_set_rejoining(0);
        state_set_current(STATE_INIT);
    }
}

/**
 * Handle STATE_ERROR
 * 
 * Error state - game ends
 */
void handle_state_error(void) {
    display_show_error(state_get_error());
}
