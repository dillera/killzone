/**
 * KillZone Constants - Atari 8-bit
 * 
 * Centralized configuration and constants.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

/* Game Info */
#define GAME_TITLE "KillZone"
#define CLIENT_VERSION "1.3.0"

/* Display Dimensions */
#define DISPLAY_WIDTH 40
#define DISPLAY_HEIGHT 20
#define STATUS_BAR_HEIGHT 4
#define STATUS_BAR_START (DISPLAY_HEIGHT)

/* Display Characters */
#define CHAR_EMPTY '.'
#define CHAR_PLAYER '@'
#define CHAR_ENEMY '*'
#define CHAR_HUNTER '+'
#define CHAR_WALL '#'

/* Player Limits */
#define MAX_OTHER_PLAYERS 10
#define PLAYER_NAME_MAX 32

/* Server Configuration */
/* Override at build time with: make atari KZ_SERVER_HOST=localhost */
#ifndef SERVER_HOST
#define SERVER_HOST "fujinet.diller.org"
#endif
#define SERVER_PORT 3000
#define SERVER_PROTO "http"
#define SERVER_TCP_PORT 6809

/* Network Configuration */
#define DEVICE_SPEC_SIZE 256

/* How often (in game-loop frames) the client polls the server for world
 * state. Each poll is a BLOCKING TCP round-trip via FujiNet, so on a
 * high-latency real-world link a smaller value means the client spends
 * more time blocked and is less responsive (and more exposed to read
 * timeouts), while a larger value trades world freshness for smoothness.
 * The old value of 5 was raised to 20 to reduce the SIO buzz; that buzz is
 * now fixed in the emulator, so this is purely a latency/freshness knob.
 * Tune here rather than hunting for a magic number in the game loop. */
#ifndef WORLD_POLL_INTERVAL_FRAMES
#define WORLD_POLL_INTERVAL_FRAMES 20
#endif

#endif /* CONSTANTS_H */
