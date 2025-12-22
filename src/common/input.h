/**
 * KillZone Input Module
 * 
 * Handles keyboard and joystick input.
 */

#ifndef INPUT_H
#define INPUT_H

#ifdef _CMOC_VERSION_
#include <cmoc.h>
#else
#include <stdint.h>
#endif

/* Input commands */
typedef enum {
    CMD_NONE = 0,
    CMD_UP,
    CMD_DOWN,
    CMD_LEFT,
    CMD_RIGHT,
    CMD_REFRESH,
    CMD_QUIT,
    CMD_YES,
    CMD_NO,
    CMD_ATTACK,
    CMD_COLOR
} input_cmd_t;

/**
 * Initialize input system
 */
void input_init(void);

/**
 * Check for input and return command
 * Non-blocking
 */
input_cmd_t input_check(void);

/**
 * Wait for a specific key press (blocking)
 * Returns the character pressed
 */
char input_wait_key(void);

#endif /* INPUT_H */
