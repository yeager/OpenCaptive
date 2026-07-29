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
    int canvas_width;
    int canvas_height;
    CaptiveRenderMode mode;
    bool integer_scaling;
} OpenCaptiveRenderer;

bool renderer_init(OpenCaptiveRenderer *r, const OpenCaptiveConfig *config);
void renderer_apply_display(OpenCaptiveRenderer *r, const OpenCaptiveConfig *config);
bool renderer_set_canvas(OpenCaptiveRenderer *r, int width, int height,
                         const OpenCaptiveConfig *config);
void renderer_set_effects(OpenCaptiveRenderer *r, bool bilinear, bool scanlines,
                          bool crt_curvature, int brightness, int contrast);
void renderer_present(OpenCaptiveRenderer *r, const uint32_t *pixels);
void renderer_shutdown(OpenCaptiveRenderer *r);

#endif
