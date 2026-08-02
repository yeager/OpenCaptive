#ifndef SFX_H
#define SFX_H

#include "sound.h"
#include "adlib_sfx.h"

typedef enum {
    SFX_HIT,
    SFX_SHOOT,
    SFX_DOOR_OPEN,
    SFX_DOOR_LOCKED,
    SFX_STEP,
    SFX_BUTTON,
    SFX_PICKUP,
    SFX_DEATH,
    SFX_LEVEL_UP,
    SFX_GENERATOR,
    SFX_COUNT,
} SfxType;

typedef struct {
    AdlibSfxPlayer adlib;
    SoundSystem *sound;
    int sfx_map[SFX_COUNT];
    bool enabled;
} SfxSystem;

bool sfx_init(SfxSystem *sfx, SoundSystem *snd);
void sfx_play(SfxSystem *sfx, SfxType type);
void sfx_update(SfxSystem *sfx);
void sfx_set_enabled(SfxSystem *sfx, bool enabled);

#endif
