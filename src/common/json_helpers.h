/**
 * KillZone JSON Helpers - Atari 8-bit
 * 
 * Helper functions for querying JSON data via FujiNet.
 */

#ifndef JSON_HELPERS_H
#define JSON_HELPERS_H

#include <stdint.h>
#include <stddef.h>

/**
 * Query an integer value from JSON
 * Returns 1 if found, 0 otherwise
 */
uint8_t json_query_int(const char *device_spec, const char *query, uint32_t *val, char *buffer);

/**
 * Query a string value from JSON
 * Returns 1 if found, 0 otherwise
 */
uint8_t json_query_string(const char *device_spec, const char *query, char *dest, size_t max_len, char *buffer);

/**
 * Query a boolean value from JSON
 * Returns 1 if found, 0 otherwise
 */
uint8_t json_query_bool(const char *device_spec, const char *query, uint8_t *val, char *buffer);

#endif /* JSON_HELPERS_H */
