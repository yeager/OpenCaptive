#ifndef ENHANCED_RENDER_H
#define ENHANCED_RENDER_H

#include "game_state.h"
#include "combat.h"
#include "texture_atlas.h"
#include <stdbool.h>
#include <stdint.h>

// Enhanced rendering: 2x resolution viewport with smooth lighting
// and bilinear texture sampling

#define ENHANCED_VP_WIDTH  (CAPTIVE_VIEWPORT_WIDTH * 2)
#define ENHANCED_VP_HEIGHT (CAPTIVE_VIEWPORT_HEIGHT * 2)

typedef struct {
    uint32_t buffer[ENHANCED_VP_WIDTH * ENHANCED_VP_HEIGHT];
    bool enabled;
} EnhancedRenderer;

void enhanced_init(EnhancedRenderer *er);
void enhanced_render(EnhancedRenderer *er, const GameState *gs,
                     uint32_t *output, int stride,
                     const TextureAtlas *atlas, const CreatureList *creatures);

#endif
