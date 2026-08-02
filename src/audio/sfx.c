#include "sfx.h"
#include <string.h>

/* SFX type to CAP_A.BIN sequence index mapping.
 * Recovered from CAPPO.EXE disassembly: game calls INT 61h AH=0x40 AL=index.
 * The driver remaps: game index < 4 = silent, 4-14 = direct, 15+ = index-11.
 * Game indices found: 30(hit), 33(weapon), 34(panel), 35(door), 37(step).
 * Variable SFX: weapon fire = [weapon_type]+3, action = [val]+38. */
static const int sfx_sequence_map[SFX_COUNT] = {
    [SFX_HIT]         = 19,  // game idx 30 -> 30-11=19 (combat damage)
    [SFX_SHOOT]       = 22,  // game idx 33 -> 33-11=22 (weapon fire)
    [SFX_DOOR_OPEN]   = 24,  // game idx 35 -> 35-11=24 (door/button)
    [SFX_DOOR_LOCKED] = 23,  // game idx 34 -> 34-11=23 (panel/locked)
    [SFX_STEP]        = 26,  // game idx 37 -> 37-11=26 (movement)
    [SFX_BUTTON]      = 18,  // game idx 29 -> 29-11=18 (UI/ambient)
    [SFX_PICKUP]      = 20,  // game idx 31 -> 31-11=20 (droid/pickup)
    [SFX_DEATH]       = 6,   // provisional (no static call site found)
    [SFX_LEVEL_UP]    = 8,   // provisional (no static call site found)
    [SFX_GENERATOR]   = 3,   // provisional (no static call site found)
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
