#ifndef KILLZONE_ATARI_VISUALS_H
#define KILLZONE_ATARI_VISUALS_H

void atari_visuals_init(void);
void atari_visuals_shutdown(void);

/* Select the font/palette for the current screen. Cheap and idempotent:
 * use_text() for menu/prose screens (stock ROM font, readable '.'/'@'),
 * use_game() for the world map (custom grass/player/monster tiles). */
void atari_visuals_use_text(void);
void atari_visuals_use_game(void);

#endif /* KILLZONE_ATARI_VISUALS_H */
