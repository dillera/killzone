/**
 * KillZone JSON Helpers Implementation - Atari 8-bit
 */

#include "json_helpers.h"
#include "fujinet-network.h"
#include <stdlib.h>
#include <string.h>

uint8_t json_query_int(const char *device_spec, const char *query, uint32_t *val, char *buffer) {
    if (network_json_query((char *)device_spec, (char *)query, buffer) == FN_ERR_OK) {
        *val = (uint32_t)strtoul(buffer, NULL, 10);
        return 1;
    }
    return 0;
}

uint8_t json_query_string(const char *device_spec, const char *query, char *dest, size_t max_len, char *buffer) {
    if (network_json_query((char *)device_spec, (char *)query, buffer) == FN_ERR_OK) {
        strncpy(dest, buffer, max_len);
        dest[max_len - 1] = '\0';
        return 1;
    }
    return 0;
}

uint8_t json_query_bool(const char *device_spec, const char *query, uint8_t *val, char *buffer) {
    if (network_json_query((char *)device_spec, (char *)query, buffer) == FN_ERR_OK) {
        if (strcmp(buffer, "true") == 0) {
            *val = 1;
        } else {
            *val = 0;
        }
        return 1;
    }
    return 0;
}
