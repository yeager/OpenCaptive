#include "anm_decoder.h"
#include <stdlib.h>
#include <string.h>

bool anm_decode(const uint8_t *data, size_t size, ANMAnimation *out) {
    if (!data || size < 16 || !out) return false;
    memset(out, 0, sizeof(*out));
    // TODO: reverse-engineer ANM format from DOS data files
    return false;
}

void anm_free(ANMAnimation *anim) {
    if (!anim) return;
    if (anim->frames) {
        for (uint16_t i = 0; i < anim->frame_count; i++) {
            free(anim->frames[i]);
        }
        free(anim->frames);
        anim->frames = NULL;
    }
}
