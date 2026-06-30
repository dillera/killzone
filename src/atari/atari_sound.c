#include <peekpoke.h>

#include "atari_sound.h"

/* POKEY hardware registers. Only channels 1 & 2 play our effects; channels
 * 3 & 4 carry NetSIO's serial clock and must keep their frequency/distortion
 * bits untouched - we only ever mask their volume nibble (bits 0-3), which
 * is purely an audio output level and has no effect on the timer/divider
 * the OS uses to clock SIO transfers. */
#define AUDF1  ((unsigned int)53760U)
#define AUDC1  ((unsigned int)53761U)
#define AUDF2  ((unsigned int)53762U)
#define AUDC2  ((unsigned int)53763U)
#define AUDC3  ((unsigned int)53765U)
#define AUDC4  ((unsigned int)53767U)
#define AUDCTL ((unsigned int)53768U)
#define SKCTL  ((unsigned int)53775U)

/* Distortion bits for a clean "pure tone" square wave, the standard
 * non-buzzy beep used by AUDCx on real hardware (POKE 53761,160+vol). */
#define AUDC_PURE_TONE 0xA0

typedef struct {
    unsigned char freq;
    unsigned char vol;
    unsigned char frames;
} sound_step_t;

static const sound_step_t hit_steps[] = {
    { 144, 6, 3 }
};

static const sound_step_t kill_steps[] = {
    { 72, 8, 3 },
    { 47, 10, 4 }
};

static const sound_step_t join_steps[] = {
    { 81, 5, 3 },
    { 60, 7, 4 }
};

static const sound_step_t death_steps[] = {
    { 106, 6, 4 },
    { 150, 5, 4 },
    { 212, 4, 5 }
};

static const sound_step_t *active_steps;
static unsigned char active_step_count;
static unsigned char active_step_index;
static unsigned char active_step_frames;
static unsigned char initialized;

static void start_step(unsigned char index) {
    const sound_step_t *step = &active_steps[index];

    /* The game polls the server constantly over FujiNet/NetSIO. SIO
     * transactions can leave SKCTL outside its normal run state, and
     * with bits 0-1 both clear POKEY's 64KHz audio clock freezes
     * entirely - so reassert it immediately before every note. */
    POKE(SKCTL, 0x03);

    POKE(AUDF1, step->freq);
    POKE(AUDC1, AUDC_PURE_TONE | step->vol);
    active_step_index = index;
    active_step_frames = step->frames;
}

static void play_sequence(const sound_step_t *steps, unsigned char count) {
    if (!initialized) {
        return;
    }
    active_steps = steps;
    active_step_count = count;
    start_step(0);
}

void atari_sound_init(void) {
    if (initialized) {
        return;
    }

    POKE(SKCTL, 0x03);
    POKE(AUDCTL, 0);
    POKE(AUDC1, 0);
    POKE(AUDC2, 0);

    active_steps = 0;
    active_step_count = 0;
    active_step_index = 0;
    active_step_frames = 0;

    initialized = 1;
}

void atari_sound_shutdown(void) {
    if (!initialized) {
        return;
    }

    POKE(AUDC1, 0);
    POKE(AUDC2, 0);

    initialized = 0;
}

void atari_sound_tick(void) {
    if (!initialized) {
        return;
    }

    /* The OS continuously reasserts POKEY channel state while NetSIO is
     * clocking transfers (same mechanism that was clobbering our own
     * channel 1 volume mid-note). Mask channels 3 & 4's volume nibble
     * every tick to silence that audible clock without ever writing
     * their frequency or distortion bits, which NetSIO still owns. */
    POKE(AUDC3, PEEK(AUDC3) & 0xF0);
    POKE(AUDC4, PEEK(AUDC4) & 0xF0);

    if (active_step_count == 0) {
        return;
    }

    /* Reassert our own note every tick too, for the same reason - one
     * write at note-start isn't enough to survive the OS clobbering it
     * mid-note. */
    POKE(AUDC1, AUDC_PURE_TONE | active_steps[active_step_index].vol);

    if (active_step_frames > 0) {
        active_step_frames--;
        if (active_step_frames == 0) {
            unsigned char next = (unsigned char)(active_step_index + 1);
            if (next < active_step_count) {
                start_step(next);
            } else {
                POKE(AUDC1, 0);
                active_step_count = 0;
            }
        }
    }
}

void atari_sound_play_hit(void) {
    play_sequence(hit_steps, sizeof(hit_steps) / sizeof(hit_steps[0]));
}

void atari_sound_play_kill(void) {
    play_sequence(kill_steps, sizeof(kill_steps) / sizeof(kill_steps[0]));
}

void atari_sound_play_join(void) {
    play_sequence(join_steps, sizeof(join_steps) / sizeof(join_steps[0]));
}

void atari_sound_play_death(void) {
    play_sequence(death_steps, sizeof(death_steps) / sizeof(death_steps[0]));
}
