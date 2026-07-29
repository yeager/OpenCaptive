#ifndef ANM_DECODER_H
#define ANM_DECODER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define ANM_WIDTH       320
#define ANM_HEIGHT      200
#define ANM_PALETTE_SIZE 768
#define ANM_FRAME_PIXELS (ANM_WIDTH * ANM_HEIGHT)

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t frame_count;
    uint8_t **frames;          // array of frame_count pixel buffers (ANM_FRAME_PIXELS each)
    uint32_t palette[256];     // ARGB8888
    uint8_t vga_palette[ANM_PALETTE_SIZE]; // raw 6-bit VGA palette
} ANMAnimation;

bool anm_decode(const uint8_t *data, size_t size, ANMAnimation *out);
void anm_free(ANMAnimation *anim);

#endif
