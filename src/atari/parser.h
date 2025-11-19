/**
 * KillZone JSON Parser Module
 * 
 * Handles parsing of JSON responses from the game server.
 * Updates local game state based on parsed data.
 */

#ifndef KILLZONE_PARSER_H
#define KILLZONE_PARSER_H

#include <stdint.h>

/**
 * Parse join response JSON
 * Updates local player state
 */
void parse_join_response(const uint8_t *response, uint16_t len);

/**
 * Parse world state JSON
 * Updates world dimensions, ticks, walls, and entities
 */
void parse_world_state(const uint8_t *response, uint16_t len);

/**
 * Check if a player ID exists in the players array of the response
 */
int parser_is_player_in_world(const uint8_t *response, const char *player_id);

#endif /* KILLZONE_PARSER_H */
