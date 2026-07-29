#include "opencaptive.h"
#include "renderer.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    printf("OpenCaptive v%d.%d.%d\n",
           OPENCAPTIVE_VERSION_MAJOR,
           OPENCAPTIVE_VERSION_MINOR,
           OPENCAPTIVE_VERSION_PATCH);

    OpenCaptiveConfig config = {
        .platform = CAPTIVE_PLATFORM_DOS,
        .render_mode = CAPTIVE_RENDER_ORIGINAL,
        .data_path = NULL,
        .scale_factor = 3,
    };

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) {
            config.data_path = argv[++i];
        } else if (strcmp(argv[i], "--enhanced") == 0) {
            config.render_mode = CAPTIVE_RENDER_ENHANCED;
        } else if (strcmp(argv[i], "--platform") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "dos") == 0) config.platform = CAPTIVE_PLATFORM_DOS;
            else if (strcmp(argv[i], "atari") == 0) config.platform = CAPTIVE_PLATFORM_ATARI_ST;
            else if (strcmp(argv[i], "amiga") == 0) config.platform = CAPTIVE_PLATFORM_AMIGA;
        } else if (strcmp(argv[i], "--scale") == 0 && i + 1 < argc) {
            config.scale_factor = atoi(argv[++i]);
        }
    }

    if (!config.data_path) {
        fprintf(stderr, "Usage: opencaptive --data <path-to-captive-data> [--enhanced] [--platform dos|atari|amiga] [--scale N]\n");
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    OpenCaptiveRenderer renderer = {0};
    if (!renderer_init(&renderer, &config)) {
        fprintf(stderr, "Failed to initialize renderer\n");
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }

        renderer_present(&renderer, NULL);
        SDL_Delay(16);
    }

    renderer_shutdown(&renderer);
    SDL_Quit();
    return 0;
}
