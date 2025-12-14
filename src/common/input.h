/**
 * KillZone Input Module - Atari 8-bit
 * 
 * Handles keyboard and joystick input.
 */

#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

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
    CMD_ATTACK
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
