/**
 * KillZone Input Module Implementation - Atari 8-bit
 */

#include "input.h"
#ifdef _CMOC_VERSION_
#include <cmoc.h>
#include <coco.h>
#include "conio_wrapper.h"
#else
#include <conio.h>
#include <stdio.h>
#endif

#include "keydefs.h"

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
    
#ifndef _CMOC_VERSION_    
    if (!kbhit()) {
        return CMD_NONE;
    }
#endif
    
    c = cgetc();
    
    switch (c) {
        case 'w':
        case 'W':
        case 'k':  /* vi-style up */
        case KEY_UP_ARROW:   
            return CMD_UP;
            
        case 's':
        case 'S':
        case 'j':  /* vi-style down */
        case KEY_DOWN_ARROW:
            return CMD_DOWN;
            
        case 'a':
        case 'A':
        case 'h':  /* vi-style left */
        case KEY_LEFT_ARROW:
            return CMD_LEFT;

        case 'c':
        case 'C':
            return CMD_COLOR;
            
        case 'd':
        case 'D':
        case 'l':  /* vi-style right */
        case KEY_RIGHT_ARROW:   /* Atari right arrow */
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
#ifdef _CMOC_VERSION_
    return waitkey(0);
#else
    return cgetc();
#endif
}
