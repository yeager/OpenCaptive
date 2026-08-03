#include "renderer.h"
#include <stdio.h>

static bool renderer_bilinear;
static bool renderer_scanlines;
static bool renderer_crt_curvature;
static int renderer_brightness = 50;
static int renderer_contrast = 50;

static bool renderer_create_framebuffer(OpenCaptiveRenderer *r, int width, int height) {
    SDL_Texture *texture = SDL_CreateTexture(r->renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture) return false;
    SDL_SetTextureScaleMode(texture,
        renderer_bilinear ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
    if (r->framebuffer) SDL_DestroyTexture(r->framebuffer);
    r->framebuffer = texture;
    r->canvas_width = width;
    r->canvas_height = height;
    return true;
}

static uint8_t apply_channel(uint8_t value) {
    int adjusted = ((int)value - 128) * renderer_contrast / 50 + 128;
    adjusted += (renderer_brightness - 50) * 255 / 100;
    if (adjusted < 0) return 0;
    if (adjusted > 255) return 255;
    return (uint8_t)adjusted;
}

bool renderer_init(OpenCaptiveRenderer *r, const OpenCaptiveConfig *config) {
    int w = 1280;
    int h = 800;

    r->window = SDL_CreateWindow("OpenCaptive", w, h, SDL_WINDOW_RESIZABLE);
    if (!r->window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetWindowSize(r->window, w, h);
    SDL_SetWindowPosition(r->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    r->renderer = SDL_CreateRenderer(r->window, NULL);
    if (!r->renderer) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        return false;
    }

    r->canvas_width = CAPTIVE_ORIGINAL_WIDTH;
    r->canvas_height = CAPTIVE_ORIGINAL_HEIGHT;
    if (!renderer_create_framebuffer(r, r->canvas_width, r->canvas_height)) {
        fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
        return false;
    }

    renderer_set_effects(r, config->bilinear, config->scanlines, config->crt_curvature,
                         config->brightness, config->contrast);
    r->window_width = w;
    r->window_height = h;
    r->mode = config->render_mode;
    renderer_apply_display(r, config);
    return true;
}

bool renderer_set_canvas(OpenCaptiveRenderer *r, int width, int height,
                         const OpenCaptiveConfig *config) {
    if (!r || !r->renderer || !config || width <= 0 || height <= 0) return false;
    if (r->canvas_width != width || r->canvas_height != height) {
        if (!renderer_create_framebuffer(r, width, height)) return false;
    }
    return true;
}

void renderer_apply_display(OpenCaptiveRenderer *r, const OpenCaptiveConfig *config) {
    if (!r || !r->window || !r->renderer || !config) return;

    r->integer_scaling = config->integer_scaling;
    r->mode = config->render_mode;
    SDL_SetRenderVSync(r->renderer, config->vsync ? 1 : 0);
    SDL_SetWindowFullscreen(r->window, config->fullscreen);
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
        uint32_t *processed = (uint32_t *)SDL_malloc((size_t)r->canvas_width * r->canvas_height * sizeof(uint32_t));
        const uint32_t *source = pixels;
        if (renderer_scanlines || renderer_crt_curvature ||
            renderer_brightness != 50 || renderer_contrast != 50) {
            for (int y = 0; y < r->canvas_height; y++) {
                for (int x = 0; x < r->canvas_width; x++) {
                    int sx = x, sy = y;
                    if (renderer_crt_curvature) {
                        float nx = (2.0f * x) / (r->canvas_width - 1) - 1.0f;
                        float ny = (2.0f * y) / (r->canvas_height - 1) - 1.0f;
                        float curved_x = nx * (1.0f + 0.12f * ny * ny);
                        float curved_y = ny * (1.0f + 0.12f * nx * nx);
                        sx = (int)((curved_x + 1.0f) * (r->canvas_width - 1) / 2.0f);
                        sy = (int)((curved_y + 1.0f) * (r->canvas_height - 1) / 2.0f);
                    }
                    uint32_t color = (sx < 0 || sx >= r->canvas_width ||
                                      sy < 0 || sy >= r->canvas_height)
                        ? 0xFF000000 : pixels[sy * r->canvas_width + sx];
                    uint8_t red = apply_channel((color >> 16) & 0xFF);
                    uint8_t green = apply_channel((color >> 8) & 0xFF);
                    uint8_t blue = apply_channel(color & 0xFF);
                    if (renderer_scanlines && (y & 1)) {
                        red = (uint8_t)(red * 3 / 5);
                        green = (uint8_t)(green * 3 / 5);
                        blue = (uint8_t)(blue * 3 / 5);
                    }
                    processed[y * r->canvas_width + x] = 0xFF000000 |
                        ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
                }
            }
            source = processed;
        }
        SDL_UpdateTexture(r->framebuffer, NULL, source,
                          r->canvas_width * sizeof(uint32_t));
        if (processed && source == processed) SDL_free(processed);
    }

    SDL_SetRenderDrawColor(r->renderer, 0, 0, 0, 255);
    SDL_RenderClear(r->renderer);
    int output_width = r->canvas_width;
    int output_height = r->canvas_height;
    SDL_GetRenderOutputSize(r->renderer, &output_width, &output_height);
    float scale_x = (float)output_width / r->canvas_width;
    float scale_y = (float)output_height / r->canvas_height;
    float scale = scale_x < scale_y ? scale_x : scale_y;
    if (r->integer_scaling && scale >= 1.0f) {
        scale = (float)(int)scale;
    }
    SDL_FRect destination = {
        (output_width - r->canvas_width * scale) / 2.0f,
        (output_height - r->canvas_height * scale) / 2.0f,
        r->canvas_width * scale,
        r->canvas_height * scale,
    };
    SDL_RenderTexture(r->renderer, r->framebuffer, NULL, &destination);
    SDL_RenderPresent(r->renderer);
}

void renderer_shutdown(OpenCaptiveRenderer *r) {
    if (r->framebuffer) SDL_DestroyTexture(r->framebuffer);
    if (r->renderer) SDL_DestroyRenderer(r->renderer);
    if (r->window) SDL_DestroyWindow(r->window);
}
