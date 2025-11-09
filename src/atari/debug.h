/**
 * KillZone Debug Configuration
 * 
 * Feature flag to enable/disable debug output
 */

#ifndef KILLZONE_DEBUG_H
#define KILLZONE_DEBUG_H

/* Set to 1 to enable debug output, 0 to disable */
#define DEBUG_ENABLED 0

#if DEBUG_ENABLED
#define DEBUG_PRINT printf
#else
#define DEBUG_PRINT(...)
#endif

#endif /* KILLZONE_DEBUG_H */
