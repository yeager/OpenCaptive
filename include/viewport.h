#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "game_state.h"
#include "combat.h"
#include "texture_atlas.h"

// Captive's original GAME SCRN viewport: 144x112 pixels at offset (32,55)
// in the 320x200 screen shell.

void viewport_render(const GameState *gs, uint32_t *pixels, int stride);

void viewport_render_textured(const GameState *gs, uint32_t *pixels, int stride,
                              const TextureAtlas *atlas);

void viewport_render_full(const GameState *gs, uint32_t *pixels, int stride,
                          const TextureAtlas *atlas, const CreatureList *creatures);

#endif
