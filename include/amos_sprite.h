#ifndef AMOS_SPRITE_H
#define AMOS_SPRITE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t width, height, depth;
    const uint8_t *planes; /* colour planes followed by a transparency mask */
    uint32_t palette[32];
} AmosSprite;

bool amos_sprite_get(const uint8_t *data, size_t size, unsigned index, AmosSprite *out);
bool amos_sprite_decode_argb(const AmosSprite *sprite, uint32_t *pixels, size_t count);

#endif
