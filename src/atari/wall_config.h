/**
 * KillZone Wall Configuration
 *
 * Defines the ATASCII characters used for wall rendering.
 * Values are in Hexadecimal.
 */

#ifndef KILLZONE_WALL_CONFIG_H
#define KILLZONE_WALL_CONFIG_H

/* ATASCII box drawing characters for walls */
#define CHAR_WALL 0x01       /* Vertical bar */
#define CHAR_WALL_H 0x02     /* Horizontal bar */
#define CHAR_WALL_TL 0x03    /* Top-left corner */
#define CHAR_WALL_TR 0x04    /* Top-right corner */
#define CHAR_WALL_BL 0x05    /* Bottom-left corner */
#define CHAR_WALL_BR 0x12    /* Bottom-right corner */
#define CHAR_WALL_CROSS 0x13 /* Cross/intersection */

#endif /* KILLZONE_WALL_CONFIG_H */
