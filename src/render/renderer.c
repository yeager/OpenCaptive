#include "renderer.h"
#include <stdio.h>

static bool renderer_bilinear;
static bool renderer_scanlines;
static bool renderer_crt_curvature;
static int renderer_brightness = 50;
static int renderer_contrast = 50;

static uint8_t apply_channel(uint8_t value) {
    int adjusted = ((int)value - 128) * renderer_contrast / 50 + 128;
    adjusted += (renderer_brightness - 50) * 255 / 100;
    if (adjusted < 0) return 0;
    if (adjusted > 255) return 255;
    return (uint8_t)adjusted;
}

bool renderer_init(OpenCaptiveRenderer *r, const OpenCaptiveConfig *config) {
    int w = CAPTIVE_ORIGINAL_WIDTH * config->scale_factor;
    int h = CAPTIVE_ORIGINAL_HEIGHT * config->scale_factor;

    r->window = SDL_CreateWindow("OpenCaptive", w, h, SDL_WINDOW_RESIZABLE);
    if (!r->window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return false;
    }

    r->renderer = SDL_CreateRenderer(r->window, NULL);
    if (!r->renderer) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        return false;
    }

    r->framebuffer = SDL_CreateTexture(r->renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    if (!r->framebuffer) {
        fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetTextureScaleMode(r->framebuffer, SDL_SCALEMODE_NEAREST);
    renderer_set_effects(r, config->bilinear, config->scanlines, config->crt_curvature,
                         config->brightness, config->contrast);
    r->window_width = w;
    r->window_height = h;
    r->mode = config->render_mode;
    renderer_apply_display(r, config);
    return true;
}

void renderer_apply_display(OpenCaptiveRenderer *r, const OpenCaptiveConfig *config) {
    if (!r || !r->window || !r->renderer || !config) return;

    r->integer_scaling = config->integer_scaling;
    r->mode = config->render_mode;
    SDL_SetRenderVSync(r->renderer, config->vsync ? 1 : 0);
    SDL_SetWindowFullscreen(r->window, config->fullscreen);
    if (!config->fullscreen) {
        r->window_width = CAPTIVE_ORIGINAL_WIDTH * config->scale_factor;
        r->window_height = CAPTIVE_ORIGINAL_HEIGHT * config->scale_factor;
        SDL_SetWindowSize(r->window, r->window_width, r->window_height);
    }
}

void renderer_set_effects(OpenCaptiveRenderer *r, bool bilinear, bool scanlines,
                          bool crt_curvature, int brightness, int contrast) {
    if (!r || !r->framebuffer) return;
    renderer_bilinear = bilinear;
    renderer_scanlines = scanlines;
    renderer_crt_curvature = crt_curvature;
    renderer_brightness = brightness < 0 ? 0 : (brightness > 100 ? 100 : brightness);
    renderer_contrast = contrast < 0 ? 0 : (contrast > 100 ? 100 : contrast);
    SDL_SetTextureScaleMode(r->framebuffer,
        renderer_bilinear ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
}

void renderer_present(OpenCaptiveRenderer *r, const uint32_t *pixels) {
    if (pixels) {
        uint32_t processed[CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT];
        const uint32_t *source = pixels;
        if (renderer_scanlines || renderer_crt_curvature ||
            renderer_brightness != 50 || renderer_contrast != 50) {
            for (int y = 0; y < CAPTIVE_ORIGINAL_HEIGHT; y++) {
                for (int x = 0; x < CAPTIVE_ORIGINAL_WIDTH; x++) {
                    int sx = x, sy = y;
                    if (renderer_crt_curvature) {
                        float nx = (2.0f * x) / (CAPTIVE_ORIGINAL_WIDTH - 1) - 1.0f;
                        float ny = (2.0f * y) / (CAPTIVE_ORIGINAL_HEIGHT - 1) - 1.0f;
                        float curved_x = nx * (1.0f + 0.12f * ny * ny);
                        float curved_y = ny * (1.0f + 0.12f * nx * nx);
                        sx = (int)((curved_x + 1.0f) * (CAPTIVE_ORIGINAL_WIDTH - 1) / 2.0f);
                        sy = (int)((curved_y + 1.0f) * (CAPTIVE_ORIGINAL_HEIGHT - 1) / 2.0f);
                    }
                    uint32_t color = (sx < 0 || sx >= CAPTIVE_ORIGINAL_WIDTH ||
                                      sy < 0 || sy >= CAPTIVE_ORIGINAL_HEIGHT)
                        ? 0xFF000000 : pixels[sy * CAPTIVE_ORIGINAL_WIDTH + sx];
                    uint8_t red = apply_channel((color >> 16) & 0xFF);
                    uint8_t green = apply_channel((color >> 8) & 0xFF);
                    uint8_t blue = apply_channel(color & 0xFF);
                    if (renderer_scanlines && (y & 1)) {
                        red = (uint8_t)(red * 3 / 5);
                        green = (uint8_t)(green * 3 / 5);
                        blue = (uint8_t)(blue * 3 / 5);
                    }
                    processed[y * CAPTIVE_ORIGINAL_WIDTH + x] = 0xFF000000 |
                        ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
                }
            }
            source = processed;
        }
        SDL_UpdateTexture(r->framebuffer, NULL, source,
                          CAPTIVE_ORIGINAL_WIDTH * sizeof(uint32_t));
    }

    SDL_SetRenderDrawColor(r->renderer, 0, 0, 0, 255);
    SDL_RenderClear(r->renderer);
    int output_width = CAPTIVE_ORIGINAL_WIDTH;
    int output_height = CAPTIVE_ORIGINAL_HEIGHT;
    SDL_GetRenderOutputSize(r->renderer, &output_width, &output_height);
    float scale_x = (float)output_width / CAPTIVE_ORIGINAL_WIDTH;
    float scale_y = (float)output_height / CAPTIVE_ORIGINAL_HEIGHT;
    float scale = scale_x < scale_y ? scale_x : scale_y;
    if (r->integer_scaling && scale >= 1.0f) {
        scale = (float)(int)scale;
    }
    SDL_FRect destination = {
        (output_width - CAPTIVE_ORIGINAL_WIDTH * scale) / 2.0f,
        (output_height - CAPTIVE_ORIGINAL_HEIGHT * scale) / 2.0f,
        CAPTIVE_ORIGINAL_WIDTH * scale,
        CAPTIVE_ORIGINAL_HEIGHT * scale,
    };
    SDL_RenderTexture(r->renderer, r->framebuffer, NULL, &destination);
    SDL_RenderPresent(r->renderer);
}

void renderer_shutdown(OpenCaptiveRenderer *r) {
    if (r->framebuffer) SDL_DestroyTexture(r->framebuffer);
    if (r->renderer) SDL_DestroyRenderer(r->renderer);
    if (r->window) SDL_DestroyWindow(r->window);
}
