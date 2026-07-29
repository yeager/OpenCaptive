#include "renderer.h"
#include <stdio.h>

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
    r->window_width = w;
    r->window_height = h;
    r->mode = config->render_mode;
    return true;
}

void renderer_present(OpenCaptiveRenderer *r, const uint32_t *pixels) {
    if (pixels) {
        SDL_UpdateTexture(r->framebuffer, NULL, pixels,
                          CAPTIVE_ORIGINAL_WIDTH * sizeof(uint32_t));
    }

    SDL_SetRenderDrawColor(r->renderer, 0, 0, 0, 255);
    SDL_RenderClear(r->renderer);
    SDL_RenderTexture(r->renderer, r->framebuffer, NULL, NULL);
    SDL_RenderPresent(r->renderer);
}

void renderer_shutdown(OpenCaptiveRenderer *r) {
    if (r->framebuffer) SDL_DestroyTexture(r->framebuffer);
    if (r->renderer) SDL_DestroyRenderer(r->renderer);
    if (r->window) SDL_DestroyWindow(r->window);
}
