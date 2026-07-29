#ifndef HUD_H
#define HUD_H

#include "game_state.h"

// Render the game HUD (droid stats, minimap, compass) into the 320x200 framebuffer
void hud_render(const GameState *gs, uint32_t *pixels, int width, int height);

#endif
