#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "captive_view_window.h"
#include "texture_atlas.h"
#include <stdint.h>

/* Render the first-person dungeon viewport into the framebuffer.
 * Uses the 19-cell view window and hash-verified PL5 panel sheets.
 *
 * The viewport area is CAPTIVE_VIEWPORT_WIDTH x CAPTIVE_VIEWPORT_HEIGHT
 * pixels at position (CAPTIVE_VIEWPORT_X, CAPTIVE_VIEWPORT_Y) in the
 * 320x200 framebuffer. */
void viewport_render(const CaptiveViewWindow *window,
                     const TextureAtlas *atlas,
                     uint32_t *framebuffer, int fb_width, int fb_height);

#endif
