/**
 * KillZone Display Module Implementation - Atari 8-bit
 * 
 * Text-based display rendering.
 * Uses Atari's text mode with last 4 lines for status bar.
 */

#include "display.h"
#include "network.h"
#ifdef _CMOC_VERSION_
#include <cmoc.h>
#include <coco.h>
#include "conio_wrapper.h"
#include "snprintf.h"
#else
#include <stdio.h>
#include <string.h>
#include <conio.h>
#endif

/* Screen buffer */
static char screen_buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];

/**
 * Initialize display system
 */
void display_init(void) {
#ifdef _CMOC_VERSION_
    hirestxt_init();
#endif
    display_clear();
    clrscr();  /* Clear screen using conio */
}

/**
 * Close display system
 */
void display_close(void) {
    display_clear();
}

/**
 * Show welcome screen
 */
void display_show_welcome(const char *server_name) {
    static char url_buf[60];
    
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
    if (SERVER_PORT == 80) {
        snprintf(url_buf, sizeof(url_buf), "  %s://%s", 
                 SERVER_PROTO, SERVER_HOST);
    } else {
        snprintf(url_buf, sizeof(url_buf), "  %s://%s:%d", 
                 SERVER_PROTO, SERVER_HOST, SERVER_PORT);
    }
    printf("%s\n", url_buf);
    
    gotoxy(0, 11);
    printf("  Waiting for game world...\n");
}

/**
 * Clear screen buffer
 */
void display_clear(void) {
    uint8_t x, y;
    for (y = 0; y < DISPLAY_HEIGHT; y++) {
        for (x = 0; x < DISPLAY_WIDTH; x++) {
            screen_buffer[y][x] = CHAR_EMPTY;
        }
    }
}

/**
 * Draw a character at position
 */
void display_draw_char(uint8_t x, uint8_t y, char c) {
    if (x < DISPLAY_WIDTH && y < DISPLAY_HEIGHT) {
        screen_buffer[y][x] = c;
    }
}

/**
 * Draw the game world
 */
void display_draw_world(const world_state_t *world) {
    uint8_t i;
    
    if (!world) {
        return;
    }
    
    display_clear();
    
    /* Draw other players */
    for (i = 0; i < world->other_player_count; i++) {
        const player_state_t *player = &world->other_players[i];
        if (player->x < DISPLAY_WIDTH && player->y < DISPLAY_HEIGHT) {
            display_draw_char(player->x, player->y, CHAR_ENEMY);
        }
    }
    
    /* Draw local player (last so it appears on top) */
    if (world->local_player.x < DISPLAY_WIDTH && world->local_player.y < DISPLAY_HEIGHT) {
        display_draw_char(world->local_player.x, world->local_player.y, CHAR_PLAYER);
    }
}

/**
 * Draw player status line
 */
void display_draw_status(const player_state_t *player) {
    if (!player) {
        return;
    }
    
    printf("\nPlayer: %s | Pos: (%d,%d) | Health: %d | Status: %s\n",
           player->id, player->x, player->y, player->health, player->status);
}

/**
 * Draw message on screen
 */
void display_draw_message(const char *message) {
    if (message) {
        printf("\n%s\n", message);
    }
}

/**
 * Update screen display
 * 
 * Renders screen buffer to fixed positions using gotoxy
 */
void display_update(void) {
    uint8_t x, y;
    
    for (y = 0; y < DISPLAY_HEIGHT; y++) {
        gotoxy(0, y);
        for (x = 0; x < DISPLAY_WIDTH; x++) {
            cputc(screen_buffer[y][x]);
        }
    }
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
    
    /* Line 23: Command help */
    cputsxy(0, 23, "WASD=Move R=Refresh Q=Quit");
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
