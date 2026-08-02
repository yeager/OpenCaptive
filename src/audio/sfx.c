#include "sfx.h"
#include <string.h>

/* SFX type to CAP_A.BIN sequence index mapping.
 * These assignments are provisional — the exact mapping between game events
 * and the 49 SFX sequences needs verification against the original game.
 * Indices chosen based on sequence characteristics (length, pitch patterns). */
static const int sfx_sequence_map[SFX_COUNT] = {
    [SFX_HIT]         = 5,   // short percussive hit
    [SFX_SHOOT]       = 13,  // multi-tone attack sequence
    [SFX_DOOR_OPEN]   = 4,   // sustained tone
    [SFX_DOOR_LOCKED] = 7,   // brief alert
    [SFX_STEP]        = 11,  // short click
    [SFX_BUTTON]      = 9,   // interface click
    [SFX_PICKUP]      = 10,  // pickup chime
    [SFX_DEATH]       = 6,   // descending tone
    [SFX_LEVEL_UP]    = 8,   // ascending tone
    [SFX_GENERATOR]   = 3,   // sustained rumble
};

bool sfx_init(SfxSystem *sfx, SoundSystem *snd) {
    if (!sfx) return false;
    memset(sfx, 0, sizeof(*sfx));
    sfx->sound = snd;
    sfx->enabled = true;

    int rate = 22050;
    adlib_sfx_init(&sfx->adlib, rate);

    for (int i = 0; i < SFX_COUNT; i++)
        sfx->sfx_map[i] = sfx_sequence_map[i];

    return true;
}

void sfx_play(SfxSystem *sfx, SfxType type) {
    if (!sfx || !sfx->enabled) return;
    if (type < 0 || type >= SFX_COUNT) return;

    adlib_sfx_play(&sfx->adlib, sfx->sfx_map[type]);
}

void sfx_update(SfxSystem *sfx) {
    if (!sfx || !sfx->enabled || !sfx->sound) return;
    if (!adlib_sfx_is_playing(&sfx->adlib)) return;

    int16_t buffer[1024];
    adlib_sfx_render(&sfx->adlib, buffer, 1024);

    if (sfx->sound->stream) {
        SDL_PutAudioStreamData(sfx->sound->stream, buffer, sizeof(buffer));
    }
}

void sfx_set_enabled(SfxSystem *sfx, bool enabled) {
    if (!sfx) return;
    sfx->enabled = enabled;
    if (!enabled) adlib_sfx_stop_all(&sfx->adlib);
}
