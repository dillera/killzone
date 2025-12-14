/**
 * KillZone Input Module Implementation - Atari 8-bit
 */

#include "input.h"
#include <conio.h>
#include <stdio.h>

/**
 * Initialize input system
 */
void input_init(void) {
    /* Nothing to initialize for standard conio input yet */
}

/**
 * Check for input and return command
 * Non-blocking
 */
input_cmd_t input_check(void) {
    int c;
    
    if (!kbhit()) {
        return CMD_NONE;
    }
    
    c = cgetc();
    
    switch (c) {
        case 'w':
        case 'W':
        case 'k':  /* vi-style up */
        case 28:   /* Atari up arrow */
            return CMD_UP;
            
        case 's':
        case 'S':
        case 'j':  /* vi-style down */
        case 29:   /* Atari down arrow */
            return CMD_DOWN;
            
        case 'a':
        case 'A':
        case 'h':  /* vi-style left */
        case 30:   /* Atari left arrow */
            return CMD_LEFT;
            
        case 'd':
        case 'D':
        case 'l':  /* vi-style right */
        case 31:   /* Atari right arrow */
            return CMD_RIGHT;
            
        case 'r':
        case 'R':
            return CMD_REFRESH;
            
        case 'q':
        case 'Q':
            return CMD_QUIT;
            
        case 'y':
        case 'Y':
            return CMD_YES;
            
        case 'n':
        case 'N':
            return CMD_NO;
            
        case ' ':
        case 'f':
        case 'F':
            return CMD_ATTACK;
            
        default:
            return CMD_NONE;
    }
}

/**
 * Wait for a specific key press (blocking)
 */
char input_wait_key(void) {
    return cgetc();
}
