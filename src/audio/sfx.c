#include "sfx.h"
#include "ctv_decoder.h"
#include <string.h>
#include <stdlib.h>

/* Sound Blaster CTV files contain the digitized sound effects.
 * SB20.CTV is the primary target (Sound Blaster 2.0 version).
 * The sound effects are stored as sequential entries in the CTV file;
 * this mapping is based on the order they appear in the original file. */
static const char *ctv_hashes[] = {
    "SB20.CTV", NULL,
};

bool sfx_init(SfxSystem *sfx, SoundSystem *snd) {
    if (!sfx) return false;
    memset(sfx, 0, sizeof(*sfx));
    sfx->sound = snd;

    for (int i = 0; i < SFX_COUNT; i++)
        sfx->sample_ids[i] = -1;

    return true;
}

bool sfx_load_ctv(SfxSystem *sfx, const uint8_t *data, uint32_t size) {
    if (!sfx || !sfx->sound || !data) return false;

    CtvFile ctv;
    if (!ctv_decode(&ctv, data, size)) return false;

    /* Map CTV entries to SFX types in the order they appear.
     * The exact mapping depends on the specific CTV file; this is a
     * sequential assignment that will be refined once the CTV contents
     * are verified against the original game's sound trigger points. */
    int count = ctv.count < SFX_COUNT ? ctv.count : SFX_COUNT;
    for (int i = 0; i < count; i++) {
        int id = sound_load_raw(sfx->sound,
                                ctv.entries[i].samples,
                                ctv.entries[i].length,
                                ctv.entries[i].sample_rate);
        if (id >= 0)
            sfx->sample_ids[i] = id;
    }

    ctv_free(&ctv);
    sfx->loaded = (count > 0);
    return sfx->loaded;
}

void sfx_play(SfxSystem *sfx, SfxType type) {
    if (!sfx || !sfx->loaded || !sfx->sound) return;
    if (type < 0 || type >= SFX_COUNT) return;
    if (sfx->sample_ids[type] < 0) return;

    sound_play(sfx->sound, sfx->sample_ids[type], 1.0f, 1.0f);
}
