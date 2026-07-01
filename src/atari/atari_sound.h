/**
 * KillZone Atari Sound Module
 *
 * Small POKEY sound-effect player using channels 1 & 2 only.
 * Channels 3 & 4 are left untouched since FujiNet's NetSIO transport
 * uses them for the SIO serial clock.
 */

#ifndef ATARI_SOUND_H
#define ATARI_SOUND_H

void atari_sound_init(void);
void atari_sound_shutdown(void);

/* Call once per game loop iteration to advance any playing effect. */
void atari_sound_tick(void);

void atari_sound_play_hit(void);
void atari_sound_play_kill(void);
void atari_sound_play_join(void);
void atari_sound_play_death(void);
void atari_sound_play_melody(void);

#endif
