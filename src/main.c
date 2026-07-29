#include "opencaptive.h"
#include "renderer.h"
#include "game_state.h"
#include "start_menu.h"
#include "viewport.h"
#include "hud.h"
#include "anm_decoder.h"
#include "pl5_decoder.h"
#include "combat.h"
#include "save_load.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t framebuffer[CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT];

static bool load_intro_anm(const char *data_path, ANMAnimation *anim) {
    char path[512];
    snprintf(path, sizeof(path), "%s/ANIMS/TEST0.ANM", data_path);
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(size);
    if (!data) { fclose(f); return false; }
    fread(data, 1, size, f);
    fclose(f);
    bool ok = anm_decode(data, size, anim);
    free(data);
    return ok;
}

static CreatureList creatures;

static void game_handle_input(GameState *gs, const SDL_Event *event) {
    if (event->type != SDL_EVENT_KEY_DOWN) return;

    const DungeonLevel *lvl = &gs->levels[gs->current_level];
    int dx = 0, dy = 0;

    switch (event->key.key) {
        case SDLK_W:
        case SDLK_UP:
            dx = (int[]){0,1,0,-1}[gs->party_dir];
            dy = (int[]){-1,0,1,0}[gs->party_dir];
            break;
        case SDLK_S:
        case SDLK_DOWN:
            dx = -(int[]){0,1,0,-1}[gs->party_dir];
            dy = -(int[]){-1,0,1,0}[gs->party_dir];
            break;
        case SDLK_Q:
            dx = -(int[]){0,1,0,-1}[(gs->party_dir+1)%4];
            dy = -(int[]){-1,0,1,0}[(gs->party_dir+1)%4];
            break;
        case SDLK_E:
            dx = (int[]){0,1,0,-1}[(gs->party_dir+1)%4];
            dy = (int[]){-1,0,1,0}[(gs->party_dir+1)%4];
            break;
        case SDLK_A:
        case SDLK_LEFT:
            gs->party_dir = (gs->party_dir + 3) % 4;
            return;
        case SDLK_D:
        case SDLK_RIGHT:
            gs->party_dir = (gs->party_dir + 1) % 4;
            return;
        case SDLK_M:
            gs->map_overlay = !gs->map_overlay;
            return;
        case SDLK_1: gs->selected_droid = 0; return;
        case SDLK_2: gs->selected_droid = 1; return;
        case SDLK_3: gs->selected_droid = 2; return;
        case SDLK_4: gs->selected_droid = 3; return;
        case SDLK_SPACE:
            combat_droid_attack(gs, &creatures, gs->selected_droid);
            return;
        case SDLK_F:
            combat_interact(gs);
            return;
        case SDLK_F5:
            save_game(gs, "opencaptive.sav");
            return;
        case SDLK_F9:
            if (load_game(gs, "opencaptive.sav")) {
                combat_init(&creatures);
                for (int i = 0; i < gs->num_levels; i++)
                    combat_spawn_for_level(&creatures, &gs->levels[i], i, gs->mission_seed);
            }
            return;
        case SDLK_PERIOD: // > stairs down
            if (lvl->cells[gs->party_y][gs->party_x].type == CELL_STAIRS_DOWN &&
                gs->current_level + 1 < gs->num_levels) {
                gs->current_level++;
                // Find stairs up on new level
                for (int sy = 0; sy < MAP_HEIGHT; sy++)
                    for (int sx = 0; sx < MAP_WIDTH; sx++)
                        if (gs->levels[gs->current_level].cells[sy][sx].type == CELL_STAIRS_UP) {
                            gs->party_x = sx; gs->party_y = sy;
                            goto stairs_done;
                        }
                stairs_done: ;
            }
            return;
        case SDLK_COMMA: // < stairs up
            if (lvl->cells[gs->party_y][gs->party_x].type == CELL_STAIRS_UP &&
                gs->current_level > 0) {
                gs->current_level--;
                for (int sy = 0; sy < MAP_HEIGHT; sy++)
                    for (int sx = 0; sx < MAP_WIDTH; sx++)
                        if (gs->levels[gs->current_level].cells[sy][sx].type == CELL_STAIRS_DOWN) {
                            gs->party_x = sx; gs->party_y = sy;
                            goto stairs_done2;
                        }
                stairs_done2: ;
            }
            return;
        default: return;
    }

    // Try to move
    int nx = gs->party_x + dx;
    int ny = gs->party_y + dy;
    if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
        CellType cell = lvl->cells[ny][nx].type;
        if (cell != CELL_WALL && cell != CELL_DOOR_LOCKED) {
            gs->party_x = nx;
            gs->party_y = ny;
        }
    }
}

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
        } else if (strcmp(argv[i], "--game") == 0 && i + 1 < argc) {
            i++; // handled after menu
        }
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

    // State
    StartMenu menu;
    start_menu_init(&menu);
    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    gs.config = config;

    // Intro animation (loaded on demand)
    ANMAnimation intro_anim = {0};
    bool intro_loaded = false;
    int intro_frame = 0;
    uint32_t intro_last_tick = 0;

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                break;
            }

            switch (gs.mode) {
                case STATE_MENU: {
                    MenuResult result = start_menu_handle_event(&menu, &event);
                    switch (result) {
                        case MENU_RESULT_START_CAPTIVE:
                            gs.game_type = GAME_CAPTIVE;
                            if (config.data_path) {
                                gs.mode = STATE_INTRO;
                                if (!intro_loaded) {
                                    intro_loaded = load_intro_anm(config.data_path, &intro_anim);
                                    intro_frame = 0;
                                    intro_last_tick = SDL_GetTicks();
                                }
                                if (!intro_loaded) {
                                    game_state_new_mission(&gs, 1);
                                    combat_init(&creatures);
                                    for (int i = 0; i < gs.num_levels; i++)
                                        combat_spawn_for_level(&creatures, &gs.levels[i], i, gs.mission_seed);
                                }
                            } else {
                                game_state_new_mission(&gs, 1);
                                combat_init(&creatures);
                                for (int i = 0; i < gs.num_levels; i++)
                                    combat_spawn_for_level(&creatures, &gs.levels[i], i, gs.mission_seed);
                            }
                            break;
                        case MENU_RESULT_START_LIBERATION:
                            gs.game_type = GAME_LIBERATION;
                            game_state_new_mission(&gs, 1);
                            combat_init(&creatures);
                            for (int li = 0; li < gs.num_levels; li++)
                                combat_spawn_for_level(&creatures, &gs.levels[li], li, gs.mission_seed);
                            break;
                        case MENU_RESULT_QUIT:
                            running = false;
                            break;
                        default: break;
                    }
                    break;
                }
                case STATE_INTRO:
                    if (event.type == SDL_EVENT_KEY_DOWN) {
                        game_state_new_mission(&gs, 1);
                        combat_init(&creatures);
                        for (int li = 0; li < gs.num_levels; li++)
                            combat_spawn_for_level(&creatures, &gs.levels[li], li, gs.mission_seed);
                    }
                    break;
                case STATE_GAME:
                    if (event.type == SDL_EVENT_KEY_DOWN &&
                        event.key.key == SDLK_ESCAPE) {
                        gs.mode = STATE_MENU;
                        start_menu_init(&menu);
                    } else {
                        game_handle_input(&gs, &event);
                    }
                    break;
                default: break;
            }
        }

        // Render
        memset(framebuffer, 0, sizeof(framebuffer));

        switch (gs.mode) {
            case STATE_MENU:
                start_menu_render(&menu, framebuffer,
                                  CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
                break;

            case STATE_INTRO:
                if (intro_loaded && intro_anim.frame_count > 0) {
                    uint32_t now = SDL_GetTicks();
                    if (now - intro_last_tick > 100) {
                        intro_frame++;
                        intro_last_tick = now;
                        if (intro_frame >= intro_anim.frame_count) {
                            game_state_new_mission(&gs, 1);
                            combat_init(&creatures);
                            for (int li = 0; li < gs.num_levels; li++)
                                combat_spawn_for_level(&creatures, &gs.levels[li], li, gs.mission_seed);
                            break;
                        }
                    }
                    // Convert indexed frame to ARGB
                    const uint8_t *frame = intro_anim.frames[intro_frame];
                    for (int i = 0; i < CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT; i++) {
                        framebuffer[i] = intro_anim.palette[frame[i]];
                    }
                }
                break;

            case STATE_GAME:
                gs.tick++;
                if (gs.tick % 4 == 0) combat_tick(&creatures, &gs);
                // Viewport
                viewport_render(&gs,
                    &framebuffer[CAPTIVE_VIEWPORT_Y * CAPTIVE_ORIGINAL_WIDTH + CAPTIVE_VIEWPORT_X],
                    CAPTIVE_ORIGINAL_WIDTH);
                // HUD
                hud_render(&gs, framebuffer,
                           CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
                break;

            default: break;
        }

        renderer_present(&renderer, framebuffer);
        SDL_Delay(16);
    }

    if (intro_loaded) anm_free(&intro_anim);
    renderer_shutdown(&renderer);
    SDL_Quit();
    return 0;
}
