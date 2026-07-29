#ifndef RENDERER_H
#define RENDERER_H

#include "opencaptive.h"
#include <SDL3/SDL.h>

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *framebuffer;
    int window_width;
    int window_height;
    CaptiveRenderMode mode;
} OpenCaptiveRenderer;

bool renderer_init(OpenCaptiveRenderer *r, const OpenCaptiveConfig *config);
void renderer_set_effects(OpenCaptiveRenderer *r, bool bilinear, bool scanlines,
                          int brightness, int contrast);
void renderer_present(OpenCaptiveRenderer *r, const uint32_t *pixels);
void renderer_shutdown(OpenCaptiveRenderer *r);

#endif
