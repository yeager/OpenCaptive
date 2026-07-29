#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "game_state.h"
#include "combat.h"
#include "texture_atlas.h"

// Captive viewport: 256x136 pixels at offset (32,8) in 320x200 screen
// 19 visible cells, 4 deep, 5 wide at closest row

void viewport_render(const GameState *gs, uint32_t *pixels, int stride);

void viewport_render_textured(const GameState *gs, uint32_t *pixels, int stride,
                              const TextureAtlas *atlas);

void viewport_render_full(const GameState *gs, uint32_t *pixels, int stride,
                          const TextureAtlas *atlas, const CreatureList *creatures);

#endif
