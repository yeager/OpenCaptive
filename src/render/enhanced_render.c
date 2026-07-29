#include "enhanced_render.h"
#include "opencaptive.h"
#include "viewport.h"
#include <string.h>
#include <math.h>

void enhanced_init(EnhancedRenderer *er) {
    memset(er, 0, sizeof(*er));
    er->enabled = true;
}

static uint32_t blend_colors(uint32_t c1, uint32_t c2, float t) {
    uint8_t r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    uint8_t r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;
    uint8_t r = (uint8_t)(r1 + (r2 - r1) * t);
    uint8_t g = (uint8_t)(g1 + (g2 - g1) * t);
    uint8_t b = (uint8_t)(b1 + (b2 - b1) * t);
    return 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static uint32_t smooth_darken(uint32_t color, float distance) {
    float factor = 1.0f / (1.0f + distance * 0.3f);
    uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * factor);
    uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * factor);
    uint8_t b = (uint8_t)((color & 0xFF) * factor);
    return 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void enhanced_render(EnhancedRenderer *er, const GameState *gs,
                     uint32_t *output, int stride,
                     const TextureAtlas *atlas, const CreatureList *creatures) {
    // First render at standard resolution
    uint32_t temp[CAPTIVE_VIEWPORT_WIDTH * CAPTIVE_VIEWPORT_HEIGHT];
    memset(temp, 0, sizeof(temp));

    viewport_render_full(gs, temp, CAPTIVE_VIEWPORT_WIDTH, atlas, creatures);

    // Upscale 2x with bilinear interpolation
    for (int y = 0; y < ENHANCED_VP_HEIGHT; y++) {
        float src_y = (float)y / 2.0f;
        int sy = (int)src_y;
        float fy = src_y - sy;
        int sy1 = (sy + 1 < CAPTIVE_VIEWPORT_HEIGHT) ? sy + 1 : sy;

        for (int x = 0; x < ENHANCED_VP_WIDTH; x++) {
            float src_x = (float)x / 2.0f;
            int sx = (int)src_x;
            float fx = src_x - sx;
            int sx1 = (sx + 1 < CAPTIVE_VIEWPORT_WIDTH) ? sx + 1 : sx;

            uint32_t c00 = temp[sy * CAPTIVE_VIEWPORT_WIDTH + sx];
            uint32_t c10 = temp[sy * CAPTIVE_VIEWPORT_WIDTH + sx1];
            uint32_t c01 = temp[sy1 * CAPTIVE_VIEWPORT_WIDTH + sx];
            uint32_t c11 = temp[sy1 * CAPTIVE_VIEWPORT_WIDTH + sx1];

            // Bilinear
            uint32_t top = blend_colors(c00, c10, fx);
            uint32_t bot = blend_colors(c01, c11, fx);
            er->buffer[y * ENHANCED_VP_WIDTH + x] = blend_colors(top, bot, fy);
        }
    }

    // Downsample back to output resolution (the viewport area)
    for (int y = 0; y < CAPTIVE_VIEWPORT_HEIGHT; y++) {
        for (int x = 0; x < CAPTIVE_VIEWPORT_WIDTH; x++) {
            // Average 2x2 block
            uint32_t c00 = er->buffer[(y*2) * ENHANCED_VP_WIDTH + x*2];
            uint32_t c10 = er->buffer[(y*2) * ENHANCED_VP_WIDTH + x*2+1];
            uint32_t c01 = er->buffer[(y*2+1) * ENHANCED_VP_WIDTH + x*2];
            uint32_t c11 = er->buffer[(y*2+1) * ENHANCED_VP_WIDTH + x*2+1];

            uint8_t r = (uint8_t)(((c00>>16)&0xFF) + ((c10>>16)&0xFF) +
                                   ((c01>>16)&0xFF) + ((c11>>16)&0xFF)) / 4;
            uint8_t g = (uint8_t)(((c00>>8)&0xFF) + ((c10>>8)&0xFF) +
                                   ((c01>>8)&0xFF) + ((c11>>8)&0xFF)) / 4;
            uint8_t b = (uint8_t)((c00&0xFF) + (c10&0xFF) +
                                   (c01&0xFF) + (c11&0xFF)) / 4;

            output[y * stride + x] = 0xFF000000 | ((uint32_t)r << 16) |
                                     ((uint32_t)g << 8) | b;
        }
    }
}
