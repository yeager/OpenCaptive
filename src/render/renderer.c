#include "renderer.h"
#include "custom_features.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>

static bool renderer_bilinear;
static bool renderer_scanlines;
static bool renderer_crt_curvature;
static int renderer_brightness = 50;
static int renderer_contrast = 50;
static int renderer_gamma = 50;

static const char *renderer_backend_name(int backend) {
    if (backend == 1) return "gpu";
    if (backend == 2) return "software";
    return NULL;
}

static int normalize_renderer_backend(int backend) {
    return backend >= 1 && backend <= 2 ? backend : 0;
}

static bool renderer_switch_backend(OpenCaptiveRenderer *r, int backend) {
    backend = normalize_renderer_backend(backend);
    if (backend == r->backend) return true;
    SDL_Renderer *replacement = SDL_CreateRenderer(
        r->window, renderer_backend_name(backend));
    if (!replacement) return false;
    SDL_Texture *texture = SDL_CreateTexture(replacement,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        r->texture_width, r->texture_height);
    if (!texture) {
        SDL_DestroyRenderer(replacement);
        return false;
    }
    SDL_SetTextureScaleMode(texture,
        renderer_bilinear ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
    SDL_Texture *old_texture = r->framebuffer;
    SDL_Renderer *old_renderer = r->renderer;
    r->renderer = replacement;
    r->framebuffer = texture;
    r->backend = backend;
    if (old_texture) SDL_DestroyTexture(old_texture);
    if (old_renderer) SDL_DestroyRenderer(old_renderer);
    return true;
}

static bool renderer_create_framebuffer(OpenCaptiveRenderer *r, int width, int height) {
    SDL_Texture *texture = SDL_CreateTexture(r->renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture) return false;
    SDL_SetTextureScaleMode(texture,
        renderer_bilinear ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
    if (r->framebuffer) SDL_DestroyTexture(r->framebuffer);
    r->framebuffer = texture;
    r->texture_width = width;
    r->texture_height = height;
    return true;
}

static bool renderer_texture_size(int width, int height, bool widescreen,
                                  int widescreen_width, bool enabled, int factor,
                                  int *out_width, int *out_height) {
    if (!out_width || !out_height || width <= 0 || height <= 0) return false;
    if (!enabled) factor = 1;
    if (factor < 1 || factor > 4) return false;
    int base_width = width;
    if (widescreen) {
        if (widescreen_width > 0) {
            base_width = widescreen_width;
        } else {
            if (height > (INT_MAX - 8) / 16) return false;
            base_width = (height * 16 + 8) / 9;
        }
        if (base_width < width) base_width = width;
    }
    if (base_width > INT_MAX / factor || height > INT_MAX / factor)
        return false;
    *out_width = base_width * factor;
    *out_height = height * factor;
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
    if (!r || !config) return false;
    r->window = NULL;
    r->renderer = NULL;
    r->framebuffer = NULL;
    int w = config->window_width > 0 ? config->window_width : 1280;
    int h = config->window_height > 0 ? config->window_height : 800;

    r->window = SDL_CreateWindow("OpenCaptive", w, h, SDL_WINDOW_RESIZABLE);
    if (!r->window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetWindowSize(r->window, w, h);
    SDL_SetWindowPosition(r->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    int requested_backend = normalize_renderer_backend(config->renderer_backend);
    r->renderer = SDL_CreateRenderer(r->window,
                                     renderer_backend_name(requested_backend));
    if (!r->renderer && requested_backend != 0) {
        /* A requested backend may be unavailable on a platform or headless
         * driver. Keep startup usable by falling back to SDL's auto choice. */
        r->renderer = SDL_CreateRenderer(r->window, NULL);
        requested_backend = 0;
    }
    if (!r->renderer) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(r->window);
        r->window = NULL;
        return false;
    }

    r->canvas_width = CAPTIVE_ORIGINAL_WIDTH;
    r->canvas_height = CAPTIVE_ORIGINAL_HEIGHT;
    r->hd_upscale = false;
    r->upscale_factor = 1;
    r->widescreen = false;
    r->widescreen_width = 0;
    if (!renderer_create_framebuffer(r, r->canvas_width, r->canvas_height)) {
        fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(r->renderer);
        SDL_DestroyWindow(r->window);
        r->renderer = NULL;
        r->window = NULL;
        return false;
    }

    renderer_set_effects(r, config->bilinear, config->scanlines, config->crt_curvature,
                         config->brightness, config->contrast, config->gamma);
    r->window_width = w;
    r->window_height = h;
    r->backend = requested_backend;
    r->mode = config->render_mode;
    renderer_apply_display(r, config);
    return true;
}

bool renderer_set_canvas(OpenCaptiveRenderer *r, int width, int height,
                         const OpenCaptiveConfig *config) {
    if (!r || !r->renderer || !config || width <= 0 || height <= 0) return false;
    int texture_width, texture_height;
    if (!renderer_texture_size(width, height, r->widescreen, r->widescreen_width,
                               r->hd_upscale, r->upscale_factor,
                               &texture_width, &texture_height)) return false;
    if (r->canvas_width != width || r->canvas_height != height ||
        r->texture_width != texture_width || r->texture_height != texture_height) {
        if (!renderer_create_framebuffer(r, texture_width, texture_height)) return false;
    }
    r->canvas_width = width;
    r->canvas_height = height;
    return true;
}

void renderer_apply_display(OpenCaptiveRenderer *r, const OpenCaptiveConfig *config) {
    if (!r || !r->window || !r->renderer || !config) return;

    r->integer_scaling = config->integer_scaling;
    r->mode = config->render_mode;
    renderer_switch_backend(r, config->renderer_backend);
    if (config->window_width > 0 && config->window_height > 0 &&
        (r->window_width != config->window_width ||
         r->window_height != config->window_height)) {
        SDL_SetWindowSize(r->window, config->window_width, config->window_height);
        r->window_width = config->window_width;
        r->window_height = config->window_height;
    }
    SDL_SetRenderVSync(r->renderer, config->vsync ? 1 : 0);
    SDL_SetWindowFullscreen(r->window, config->fullscreen);
}

void renderer_set_effects(OpenCaptiveRenderer *r, bool bilinear, bool scanlines,
                          bool crt_curvature, int brightness, int contrast,
                          int gamma) {
    if (!r || !r->framebuffer) return;
    renderer_bilinear = bilinear;
    renderer_scanlines = scanlines;
    renderer_crt_curvature = crt_curvature;
    renderer_brightness = brightness < 0 ? 0 : (brightness > 100 ? 100 : brightness);
    renderer_contrast = contrast < 0 ? 0 : (contrast > 100 ? 100 : contrast);
    renderer_gamma = gamma < 0 ? 0 : (gamma > 100 ? 100 : gamma);
    SDL_SetTextureScaleMode(r->framebuffer,
        renderer_bilinear ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
}

static uint8_t apply_gamma(uint8_t value) {
    if (renderer_gamma == 50 || value == 0 || value == 255) return value;
    /* 50 is neutral. Higher values brighten mid-tones and lower values
     * darken them; keep the exponent finite at the slider's zero endpoint. */
    double level = renderer_gamma > 0 ? (double)renderer_gamma : 1.0;
    double exponent = 50.0 / level;
    double normalized = (double)value / 255.0;
    int adjusted = (int)lround(pow(normalized, exponent) * 255.0);
    if (adjusted < 0) return 0;
    if (adjusted > 255) return 255;
    return (uint8_t)adjusted;
}

void renderer_set_upscale(OpenCaptiveRenderer *r, bool enabled, int factor) {
    if (!r || !r->renderer) return;
    if (!enabled) factor = 1;
    if (factor < 1 || factor > 4) return;
    int texture_width, texture_height;
    if (!renderer_texture_size(r->canvas_width, r->canvas_height,
                               r->widescreen, r->widescreen_width,
                               enabled, factor,
                               &texture_width, &texture_height)) return;
    if (r->hd_upscale == enabled && r->upscale_factor == factor &&
        r->texture_width == texture_width && r->texture_height == texture_height)
        return;
    if (!renderer_create_framebuffer(r, texture_width, texture_height)) return;
    r->hd_upscale = enabled;
    r->upscale_factor = factor;
    SDL_SetTextureScaleMode(r->framebuffer,
        renderer_bilinear ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
}

void renderer_set_widescreen(OpenCaptiveRenderer *r, bool enabled, int width) {
    if (!r || !r->renderer) return;
    if (width < 0) width = 0;
    int texture_width, texture_height;
    if (!renderer_texture_size(r->canvas_width, r->canvas_height, enabled, width,
                               r->hd_upscale, r->upscale_factor,
                               &texture_width, &texture_height)) return;
    if (r->widescreen == enabled && r->widescreen_width == width &&
        r->texture_width == texture_width && r->texture_height == texture_height)
        return;
    if (!renderer_create_framebuffer(r, texture_width, texture_height)) return;
    r->widescreen = enabled;
    r->widescreen_width = width;
    SDL_SetTextureScaleMode(r->framebuffer,
        renderer_bilinear ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
}

void renderer_present(OpenCaptiveRenderer *r, const uint32_t *pixels) {
    if (!r || !r->renderer || !r->framebuffer) return;
    if (pixels) {
        uint32_t *upscaled = NULL;
        uint32_t *widened = NULL;
        uint32_t *processed = NULL;
        const uint32_t *source = pixels;
        int render_width = r->canvas_width;
        int render_height = r->canvas_height;
        if (r->hd_upscale && r->upscale_factor > 1) {
            int upscale_width = r->canvas_width * r->upscale_factor;
            int upscale_height = r->canvas_height * r->upscale_factor;
            size_t pixel_count = (size_t)upscale_width * (size_t)upscale_height;
            if (pixel_count <= SIZE_MAX / sizeof(*upscaled))
                upscaled = (uint32_t *)SDL_malloc(pixel_count * sizeof(*upscaled));
            if (upscaled) {
                switch (r->upscale_factor) {
                    case 2: upscale_xbrz_2x(pixels, r->canvas_width, r->canvas_height, upscaled); break;
                    case 3: upscale_xbrz_3x(pixels, r->canvas_width, r->canvas_height, upscaled); break;
                    case 4: upscale_xbrz_4x(pixels, r->canvas_width, r->canvas_height, upscaled); break;
                    default: break;
                }
                source = upscaled;
                render_width = upscale_width;
                render_height = upscale_height;
            }
        }
        if (r->widescreen && render_width != r->texture_width) {
            size_t pixel_count = (size_t)r->texture_width * (size_t)render_height;
            if (pixel_count <= SIZE_MAX / sizeof(*widened))
                widened = (uint32_t *)SDL_malloc(pixel_count * sizeof(*widened));
            if (widened) {
                for (int y = 0; y < render_height; ++y) {
                    for (int x = 0; x < r->texture_width; ++x) {
                        int source_x = (int)((int64_t)x * render_width /
                                             r->texture_width);
                        if (source_x >= render_width) source_x = render_width - 1;
                        widened[y * r->texture_width + x] =
                            source[y * render_width + source_x];
                    }
                }
                source = widened;
                render_width = r->texture_width;
            }
        }
        bool apply_curvature = renderer_crt_curvature &&
                               render_width > 1 && render_height > 1;
        if (renderer_scanlines || apply_curvature ||
            renderer_brightness != 50 || renderer_contrast != 50 ||
            renderer_gamma != 50) {
            size_t pixel_count = (size_t)render_width * (size_t)render_height;
            if (pixel_count <= SIZE_MAX / sizeof(*processed))
                processed = (uint32_t *)SDL_malloc(pixel_count * sizeof(*processed));
            if (processed) for (int y = 0; y < render_height; y++) {
                for (int x = 0; x < render_width; x++) {
                    int sx = x, sy = y;
                    if (apply_curvature) {
                        float nx = (2.0f * x) / (render_width - 1) - 1.0f;
                        float ny = (2.0f * y) / (render_height - 1) - 1.0f;
                        float curved_x = nx * (1.0f + 0.12f * ny * ny);
                        float curved_y = ny * (1.0f + 0.12f * nx * nx);
                        sx = (int)((curved_x + 1.0f) * (render_width - 1) / 2.0f);
                        sy = (int)((curved_y + 1.0f) * (render_height - 1) / 2.0f);
                    }
                    uint32_t color = (sx < 0 || sx >= render_width ||
                                      sy < 0 || sy >= render_height)
                        ? 0xFF000000 : source[sy * render_width + sx];
                    uint8_t red = apply_gamma(apply_channel((color >> 16) & 0xFF));
                    uint8_t green = apply_gamma(apply_channel((color >> 8) & 0xFF));
                    uint8_t blue = apply_gamma(apply_channel(color & 0xFF));
                    if (renderer_scanlines && (y & 1)) {
                        red = (uint8_t)(red * 3 / 5);
                        green = (uint8_t)(green * 3 / 5);
                        blue = (uint8_t)(blue * 3 / 5);
                    }
                    processed[y * render_width + x] = 0xFF000000 |
                        ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
                }
            }
            if (processed) source = processed;
        }
        SDL_UpdateTexture(r->framebuffer, NULL, source,
                          render_width * sizeof(uint32_t));
        if (processed && source == processed) SDL_free(processed);
        if (widened) SDL_free(widened);
        if (upscaled) SDL_free(upscaled);
    }

    SDL_SetRenderDrawColor(r->renderer, 0, 0, 0, 255);
    SDL_RenderClear(r->renderer);
    int output_width = r->texture_width;
    int output_height = r->texture_height;
    SDL_GetRenderOutputSize(r->renderer, &output_width, &output_height);
    float scale_x = (float)output_width / r->texture_width;
    float scale_y = (float)output_height / r->texture_height;
    float scale = scale_x < scale_y ? scale_x : scale_y;
    if (r->integer_scaling && scale >= 1.0f) {
        scale = (float)(int)scale;
    }
    SDL_FRect destination = {
        (output_width - r->texture_width * scale) / 2.0f,
        (output_height - r->texture_height * scale) / 2.0f,
        r->texture_width * scale,
        r->texture_height * scale,
    };
    SDL_RenderTexture(r->renderer, r->framebuffer, NULL, &destination);
    SDL_RenderPresent(r->renderer);
}

void renderer_shutdown(OpenCaptiveRenderer *r) {
    if (!r) return;
    if (r->framebuffer) SDL_DestroyTexture(r->framebuffer);
    if (r->renderer) SDL_DestroyRenderer(r->renderer);
    if (r->window) SDL_DestroyWindow(r->window);
    r->framebuffer = NULL;
    r->renderer = NULL;
    r->window = NULL;
}
