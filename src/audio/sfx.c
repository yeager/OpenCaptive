#include "sfx.h"
#include <string.h>

/* SFX type to CAP_A.BIN sequence index mapping.
 * Recovered from CAPPO.EXE disassembly: game calls INT 61h AH=0x40 AL=index.
 * The driver remaps: game index < 4 = silent, 4-14 = direct, 15+ = index-11.
 * Call sites found via disassembly of unpacked binary (SHA-256 fa7d5ca7...):
 *   0x67D0: AX=37 (step), 0x92FE: AX=33 (weapon), 0x8285: AX=35 (door),
 *   0x82CC: AX=34 (panel), 0xA48C: AL=29 (UI), 0x50B5: AX=31 (spawn),
 *   0x5763: DX=24 (creature hit), 0x578E: DX=28 (creature death),
 *   0x56AC: DX=26/27 (combat variants), 0x56B6: DX=19 (combat event).
 * Variable: weapon_type+3 at 0x55EF, val+38 at 0x734A. */
static const int sfx_sequence_map[SFX_COUNT] = {
    [SFX_HIT]         = 13,  // game idx 24 -> 24-11=13 (creature hit, at 0x5763)
    [SFX_SHOOT]       = 22,  // game idx 33 -> 33-11=22 (weapon fire, at 0x92FE)
    [SFX_DOOR_OPEN]   = 24,  // game idx 35 -> 35-11=24 (door/button, at 0x8285)
    [SFX_DOOR_LOCKED] = 23,  // game idx 34 -> 34-11=23 (panel/locked, at 0x82CC)
    [SFX_STEP]        = 26,  // game idx 37 -> 37-11=26 (movement, at 0x67D0)
    [SFX_BUTTON]      = 18,  // game idx 29 -> 29-11=18 (UI/ambient, at 0xA48C)
    [SFX_PICKUP]      = 20,  // game idx 31 -> 31-11=20 (spawn/pickup, at 0x50B5)
    [SFX_DEATH]       = 17,  // game idx 28 -> 28-11=17 (creature death, at 0x578E)
    [SFX_LEVEL_UP]    = 15,  // game idx 26 -> 26-11=15 (combat variant A, at 0x56AC)
    [SFX_GENERATOR]   = 8,   // game idx 19 -> 19-11=8 (combat event, at 0x56B6)
    [SFX_AMBIENT_DRIP] = 18, // reuse UI/ambient sequence for drip
    [SFX_AMBIENT_HUM]  = 8,  // low rumble reuse
    [SFX_AMBIENT_WIND] = 15, // reuse combat variant for wind
};

bool sfx_init(SfxSystem *sfx, SoundSystem *snd) {
    if (!sfx) return false;
    memset(sfx, 0, sizeof(*sfx));
    sfx->sound = snd;
    sfx->enabled = true;

    int rate = (snd && snd->sample_rate > 0) ? (int)snd->sample_rate : 22050;
    adlib_sfx_init(&sfx->adlib, rate);

    for (int i = 0; i < SFX_COUNT; i++)
        sfx->sfx_map[i] = sfx_sequence_map[i];

    if (snd) sound_register_aux(snd, sfx->mix_buf, &sfx->mix_pending);
    return true;
}

void sfx_play(SfxSystem *sfx, SfxType type) {
    if (!sfx || !sfx->enabled) return;
    if (type < 0 || type >= SFX_COUNT) return;

    adlib_sfx_play(&sfx->adlib, sfx->sfx_map[type]);
}

void sfx_update(SfxSystem *sfx) {
    if (!sfx || !sfx->enabled || !sfx->sound) return;
    if (!adlib_sfx_is_playing(&sfx->adlib)) { sfx->mix_pending = false; return; }

    adlib_sfx_render(&sfx->adlib, sfx->mix_buf, 1024);
    sfx->mix_pending = true;
}

void sfx_set_enabled(SfxSystem *sfx, bool enabled) {
    if (!sfx) return;
    sfx->enabled = enabled;
    if (!enabled) adlib_sfx_stop_all(&sfx->adlib);
}
