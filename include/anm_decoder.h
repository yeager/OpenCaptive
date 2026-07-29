#ifndef ANM_DECODER_H
#define ANM_DECODER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ANM is the Captive animation format (ANIMS/*.ANM)

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t frame_count;
    uint16_t frame_delay_ms;
    uint8_t **frames;
    uint32_t palette[256];
} ANMAnimation;

bool anm_decode(const uint8_t *data, size_t size, ANMAnimation *out);
void anm_free(ANMAnimation *anim);

#endif
