#include "sfx.h"
#include <string.h>

bool sfx_init(SfxSystem *sfx, SoundSystem *snd) {
    if (!sfx) return false;
    memset(sfx, 0, sizeof(*sfx));
    sfx->sound = snd;

    /* Do not substitute generated tones and noise for the original effects.
       The game media is hash-verified, but its effect playback format has not
       been recovered yet.  Keeping this subsystem unloaded makes every call
       to sfx_play() an explicit no-op until decoded original samples can be
       registered here. */
    return true;
}

void sfx_play(SfxSystem *sfx, SfxType type) {
    (void)sfx;
    (void)type;
}
