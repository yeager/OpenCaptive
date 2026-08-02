#ifndef ADLIB_SFX_H
#define ADLIB_SFX_H

#include "opl2_emu.h"
#include "adlib_data.h"
#include <stdbool.h>

/* AdLib SFX player: interprets the bytecode sequences from CAP_A.BIN
 * and drives the OPL2 emulator to render sound effects to PCM. */

#define ADLIB_SFX_MAX_ACTIVE 4

typedef struct {
    const uint8_t *data;
    size_t size;
    size_t pos;
    int delay;
    int channel;
    bool active;
} AdlibSfxVoice;

typedef struct {
    OPL2 opl;
    AdlibSfxVoice voices[ADLIB_SFX_MAX_ACTIVE];
    int tick_counter;
    int samples_per_tick;
} AdlibSfxPlayer;

void adlib_sfx_init(AdlibSfxPlayer *player, int sample_rate);
bool adlib_sfx_play(AdlibSfxPlayer *player, int sfx_id);
void adlib_sfx_stop_all(AdlibSfxPlayer *player);
void adlib_sfx_render(AdlibSfxPlayer *player, int16_t *buffer, int num_samples);
bool adlib_sfx_is_playing(const AdlibSfxPlayer *player);

#endif
