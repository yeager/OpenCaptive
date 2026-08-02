#ifndef SFX_H
#define SFX_H

#include "sound.h"

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
    int sample_ids[SFX_COUNT];
    SoundSystem *sound;
    bool loaded;
} SfxSystem;

bool sfx_init(SfxSystem *sfx, SoundSystem *snd);
bool sfx_load_ctv(SfxSystem *sfx, const uint8_t *data, uint32_t size);
void sfx_play(SfxSystem *sfx, SfxType type);

#endif
