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
    int texture_width;
    int texture_height;
    CaptiveRenderMode mode;
    int backend; // 0=auto, 1=gpu, 2=software
    bool integer_scaling;
    bool hd_upscale;
    int upscale_factor;
    bool widescreen;
    int widescreen_width;
} OpenCaptiveRenderer;

bool renderer_init(OpenCaptiveRenderer *r, const OpenCaptiveConfig *config);
void renderer_apply_display(OpenCaptiveRenderer *r, const OpenCaptiveConfig *config);
bool renderer_set_canvas(OpenCaptiveRenderer *r, int width, int height,
                         const OpenCaptiveConfig *config);
void renderer_set_effects(OpenCaptiveRenderer *r, bool bilinear, bool scanlines,
                          bool crt_curvature, int brightness, int contrast,
                          int gamma);
bool renderer_set_upscale(OpenCaptiveRenderer *r, bool enabled, int factor);
bool renderer_set_widescreen(OpenCaptiveRenderer *r, bool enabled, int width);
void renderer_present(OpenCaptiveRenderer *r, const uint32_t *pixels);
/* Map a window-relative mouse position (logical points) to a canvas pixel,
 * mirroring renderer_present()'s letterboxing and HiDPI points->pixels ratio.
 * Returns false when the click is outside the presented image. */
bool renderer_window_to_canvas(const OpenCaptiveRenderer *r,
                               float window_x, float window_y,
                               float *canvas_x, float *canvas_y);
/* SDL-free inverse of the presentation transform, exposed for testing.
 * window_(w,h) in points, output_(w,h) in pixels (HiDPI-aware). */
bool renderer_map_point(int window_w, int window_h, int output_w, int output_h,
                        int texture_w, int texture_h,
                        int canvas_w, int canvas_h, bool integer_scaling,
                        float window_x, float window_y,
                        float *canvas_x, float *canvas_y);
void renderer_shutdown(OpenCaptiveRenderer *r);

#endif
