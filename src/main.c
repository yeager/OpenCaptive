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
#include "texture_atlas.h"
#include "music.h"
#include "puzzle.h"
#include "sound.h"
#include "shop.h"
#include "inventory.h"
#include "droid_ui.h"
#include "terminal.h"
#include "sfx.h"
#include "liberation.h"
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
static PuzzleList puzzles;
static SoundSystem sound_sys;
static MusicSystem music_sys;
static ItemDatabase item_db;
static ShopState shop;
static DroidUIState droid_ui;
static TerminalState terminal;
static SfxSystem sfx;
static LibState lib_state;

static const uint8_t simple_font[][5] = {
    ['A'] = {0x7C,0x12,0x12,0x12,0x7C}, ['B'] = {0x7E,0x4A,0x4A,0x4A,0x34},
    ['C'] = {0x3C,0x42,0x42,0x42,0x24}, ['D'] = {0x7E,0x42,0x42,0x42,0x3C},
    ['E'] = {0x7E,0x4A,0x4A,0x4A,0x42}, ['F'] = {0x7E,0x0A,0x0A,0x0A,0x02},
    ['G'] = {0x3C,0x42,0x52,0x52,0x34}, ['H'] = {0x7E,0x08,0x08,0x08,0x7E},
    ['I'] = {0x42,0x42,0x7E,0x42,0x42}, ['K'] = {0x7E,0x08,0x14,0x22,0x40},
    ['L'] = {0x7E,0x40,0x40,0x40,0x40}, ['M'] = {0x7E,0x04,0x08,0x04,0x7E},
    ['N'] = {0x7E,0x04,0x08,0x10,0x7E}, ['O'] = {0x3C,0x42,0x42,0x42,0x3C},
    ['P'] = {0x7E,0x12,0x12,0x12,0x0C}, ['R'] = {0x7E,0x12,0x32,0x52,0x0C},
    ['S'] = {0x24,0x4A,0x4A,0x4A,0x30}, ['T'] = {0x02,0x02,0x7E,0x02,0x02},
    ['U'] = {0x3E,0x40,0x40,0x40,0x3E}, ['V'] = {0x1E,0x20,0x40,0x20,0x1E},
    ['W'] = {0x3E,0x40,0x30,0x40,0x3E}, ['Y'] = {0x06,0x08,0x70,0x08,0x06},
    ['!'] = {0x00,0x00,0x5E,0x00,0x00}, [' '] = {0x00,0x00,0x00,0x00,0x00},
    ['.'] = {0x00,0x40,0x00,0x00,0x00},
};

static void draw_simple_text(uint32_t *fb, int pw, int ph,
                             int x, int y, const char *text, uint32_t color, int scale) {
    for (int i = 0; text[i]; i++) {
        unsigned char ch = (unsigned char)text[i];
        if (ch >= sizeof(simple_font)/sizeof(simple_font[0])) continue;
        const uint8_t *glyph = simple_font[ch];
        for (int col = 0; col < 5; col++) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 7; row++) {
                if (bits & (1 << row)) {
                    for (int sy = 0; sy < scale; sy++)
                        for (int sx = 0; sx < scale; sx++) {
                            int px = x + (i * 6 + col) * scale + sx;
                            int py = y + row * scale + sy;
                            if (px >= 0 && px < pw && py >= 0 && py < ph)
                                fb[py * pw + px] = color;
                        }
                }
            }
        }
    }
}

static void draw_centered(uint32_t *fb, int pw, int ph,
                          int y, const char *text, uint32_t color, int scale) {
    int len = 0;
    while (text[len]) len++;
    int tw = len * 6 * scale;
    draw_simple_text(fb, pw, ph, (pw - tw) / 2, y, text, color, scale);
}

static void spawn_level_content(GameState *gs_ptr) {
    combat_init(&creatures);
    puzzle_init(&puzzles);
    for (int i = 0; i < gs_ptr->num_levels; i++) {
        combat_spawn_for_level(&creatures, &gs_ptr->levels[i], i, gs_ptr->mission_seed);
        puzzle_generate(&puzzles, &gs_ptr->levels[i], i, gs_ptr->mission_seed);
    }
}

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
        case SDLK_I:
            droid_ui_init(&droid_ui, gs->selected_droid);
            gs->mode = STATE_INVENTORY;
            return;
        case SDLK_T:
            terminal_init(&terminal, gs->current_level);
            gs->mode = STATE_TERMINAL;
            return;
        case SDLK_1: gs->selected_droid = 0; return;
        case SDLK_2: gs->selected_droid = 1; return;
        case SDLK_3: gs->selected_droid = 2; return;
        case SDLK_4: gs->selected_droid = 3; return;
        case SDLK_SPACE:
            if (combat_droid_attack(gs, &creatures, gs->selected_droid))
                sfx_play(&sfx, SFX_SHOOT);
            return;
        case SDLK_F: {
            const DungeonLevel *cur = &gs->levels[gs->current_level];
            // Check if on shop cell
            if (cur->cells[gs->party_y][gs->party_x].type == CELL_SHOP) {
                shop_init(&shop, &item_db, gs->current_level, gs->mission_seed);
                shop.gold = gs->gold;
                gs->mode = STATE_SHOP;
                music_play(&music_sys, MUSIC_SHOP);
                sfx_play(&sfx, SFX_DOOR_OPEN);
                return;
            }
            // Try puzzle first, then general interact
            int fwd_x = (int[]){0,1,0,-1}[gs->party_dir];
            int fwd_y = (int[]){-1,0,1,0}[gs->party_dir];
            int tx = gs->party_x + fwd_x;
            int ty = gs->party_y + fwd_y;
            int face = (gs->party_dir + 2) % 4;
            if (!puzzle_interact(&puzzles, gs, gs->party_x, gs->party_y, gs->party_dir) &&
                !puzzle_interact(&puzzles, gs, tx, ty, face)) {
                combat_interact(gs);
            }
            return;
        }
        case SDLK_F5:
            save_game(gs, "opencaptive.sav");
            return;
        case SDLK_F9:
            if (load_game(gs, "opencaptive.sav")) {
                spawn_level_content(gs);
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
            sfx_play(&sfx, SFX_STEP);
        } else if (cell == CELL_DOOR_LOCKED) {
            sfx_play(&sfx, SFX_DOOR_LOCKED);
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

    // Audio
    sound_init(&sound_sys);
    music_init(&music_sys, &sound_sys, config.data_path);

    // Items and SFX
    item_db_init(&item_db);
    sfx_init(&sfx, &sound_sys);

    // Texture atlas
    TextureAtlas atlas = {0};
    bool textures_loaded = false;
    if (config.data_path) {
        textures_loaded = texture_atlas_load(&atlas, config.data_path);
        if (textures_loaded) printf("Loaded texture atlas\n");
    }

    // HUD background from GAMESCRN
    uint32_t *hud_bg = NULL;
    if (textures_loaded && atlas.gamescrn_sheet >= 0) {
        const Texture *gs_tex = gfx_get(&atlas.gfx, atlas.gamescrn_sheet);
        if (gs_tex) hud_bg = gs_tex->pixels;
    }

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
                                    spawn_level_content(&gs);
                                    music_play(&music_sys, MUSIC_BASE);
                                    gs.mode = STATE_GAME;
                                }
                            } else {
                                game_state_new_mission(&gs, 1);
                                spawn_level_content(&gs);
                                music_play(&music_sys, MUSIC_BASE);
                                gs.mode = STATE_GAME;
                            }
                            break;
                        case MENU_RESULT_START_LIBERATION:
                            gs.game_type = GAME_LIBERATION;
                            lib_init(&lib_state, 42);
                            game_state_new_mission(&gs, 1);
                            spawn_level_content(&gs);
                            music_play(&music_sys, MUSIC_BASE);
                            gs.mode = STATE_GAME;
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
                        spawn_level_content(&gs);
                        music_play(&music_sys, MUSIC_BASE);
                        gs.mode = STATE_GAME;
                    }
                    break;
                case STATE_GAME:
                    if (event.type == SDL_EVENT_KEY_DOWN &&
                        event.key.key == SDLK_ESCAPE) {
                        gs.mode = STATE_MENU;
                        start_menu_init(&menu);
                        music_stop(&music_sys);
                    } else {
                        game_handle_input(&gs, &event);
                    }
                    break;
                case STATE_TERMINAL:
                    if (event.type == SDL_EVENT_KEY_DOWN) {
                        switch (event.key.key) {
                            case SDLK_ESCAPE:
                                if (terminal.page != TERM_MAIN)
                                    terminal_handle_key(&terminal, 0x1B);
                                else
                                    gs.mode = STATE_GAME;
                                break;
                            case SDLK_UP:
                                terminal_handle_key(&terminal, 0x50);
                                break;
                            case SDLK_DOWN:
                                terminal_handle_key(&terminal, 0x51);
                                break;
                            case SDLK_RETURN:
                                terminal_handle_key(&terminal, 0x0D);
                                if (!terminal.active) gs.mode = STATE_GAME;
                                break;
                            default: break;
                        }
                    }
                    break;
                case STATE_INVENTORY:
                    if (event.type == SDL_EVENT_KEY_DOWN) {
                        switch (event.key.key) {
                            case SDLK_ESCAPE:
                                gs.mode = STATE_GAME;
                                break;
                            case SDLK_UP:
                                droid_ui_handle_key(&droid_ui, &gs, 0x50);
                                break;
                            case SDLK_DOWN:
                                droid_ui_handle_key(&droid_ui, &gs, 0x51);
                                break;
                            case SDLK_TAB:
                                droid_ui_handle_key(&droid_ui, &gs, 0x09);
                                break;
                            case SDLK_RETURN:
                                droid_ui_handle_key(&droid_ui, &gs, 0x0D);
                                break;
                            default: break;
                        }
                    }
                    break;
                case STATE_SHOP:
                    if (event.type == SDL_EVENT_KEY_DOWN) {
                        switch (event.key.key) {
                            case SDLK_ESCAPE:
                                gs.gold = shop.gold;
                                gs.mode = STATE_GAME;
                                music_play(&music_sys, MUSIC_BASE);
                                break;
                            case SDLK_UP:
                                if (shop.selected > 0) shop.selected--;
                                break;
                            case SDLK_DOWN:
                                if (shop.selected < shop.num_items - 1) shop.selected++;
                                break;
                            case SDLK_RETURN:
                                shop_buy(&shop, &item_db, &gs);
                                break;
                            default: break;
                        }
                    }
                    break;
                case STATE_GAMEOVER:
                case STATE_VICTORY:
                    if (event.type == SDL_EVENT_KEY_DOWN &&
                        event.key.key == SDLK_ESCAPE) {
                        gs.mode = STATE_MENU;
                        start_menu_init(&menu);
                        music_stop(&music_sys);
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
                            spawn_level_content(&gs);
                            music_play(&music_sys, MUSIC_BASE);
                            gs.mode = STATE_GAME;
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

            case STATE_GAME: {
                gs.tick++;
                if (gs.tick % 4 == 0) combat_tick(&creatures, &gs);

                // Check for game over (all droids destroyed)
                bool all_dead = true;
                for (int d = 0; d < 4; d++) {
                    if (gs.droids[d].hp > 0) { all_dead = false; break; }
                }
                if (all_dead) {
                    gs.mode = STATE_GAMEOVER;
                    music_play(&music_sys, MUSIC_TRAPPED);
                    break;
                }

                // Check for mission complete (generator destroyed = no active creatures)
                bool mission_done = true;
                for (int ci = 0; ci < creatures.num_creatures; ci++) {
                    if (creatures.creatures[ci].active &&
                        creatures.creatures[ci].level == gs.current_level) {
                        mission_done = false; break;
                    }
                }
                // Only on last level and after tick 200 (grace period)
                if (mission_done && gs.current_level == gs.num_levels - 1 && gs.tick > 200) {
                    if (gs.mission >= 10) {
                        gs.mode = STATE_VICTORY;
                        music_play(&music_sys, MUSIC_ESCAPE);
                    } else {
                        gs.mission++;
                        game_state_new_mission(&gs, gs.mission);
                        spawn_level_content(&gs);
                    }
                    break;
                }

                if (gs.game_type == GAME_LIBERATION) {
                    // Liberation: city or building view
                    if (lib_state.mode == LIB_MODE_CITY) {
                        lib_render_city(&lib_state, framebuffer,
                                        CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
                    } else {
                        lib_render_building(&lib_state,
                            &framebuffer[CAPTIVE_VIEWPORT_Y * CAPTIVE_ORIGINAL_WIDTH + CAPTIVE_VIEWPORT_X],
                            CAPTIVE_ORIGINAL_WIDTH);
                        hud_render(&gs, framebuffer,
                                   CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
                    }
                } else {
                    // Captive: dungeon crawling
                    if (hud_bg) {
                        memcpy(framebuffer, hud_bg,
                               CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT * sizeof(uint32_t));
                    }
                    viewport_render_full(&gs,
                        &framebuffer[CAPTIVE_VIEWPORT_Y * CAPTIVE_ORIGINAL_WIDTH + CAPTIVE_VIEWPORT_X],
                        CAPTIVE_ORIGINAL_WIDTH,
                        textures_loaded ? &atlas : NULL, &creatures);
                    hud_render(&gs, framebuffer,
                               CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
                }
                break;
            }

            case STATE_TERMINAL:
                terminal_render(&terminal, &gs, framebuffer,
                                CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
                break;

            case STATE_INVENTORY:
                droid_ui_render(&droid_ui, &gs, &item_db, framebuffer,
                                CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
                break;

            case STATE_SHOP:
                shop_render(&shop, &item_db, framebuffer,
                            CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
                break;

            case STATE_GAMEOVER:
                memset(framebuffer, 0, sizeof(framebuffer));
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              60, "GAME OVER", 0xFFFF2222, 3);
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              100, "ALL DROIDS DESTROYED", 0xFFAAAAAA, 1);
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              130, "PRESS ESCAPE", 0xFF888888, 1);
                break;

            case STATE_VICTORY:
                memset(framebuffer, 0, sizeof(framebuffer));
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              40, "VICTORY!", 0xFF44FF44, 3);
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              80, "ALL MISSIONS COMPLETE", 0xFFFFFF44, 2);
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              120, "YOU HAVE ESCAPED!", 0xFFAAAAFF, 1);
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              150, "PRESS ESCAPE", 0xFF888888, 1);
                break;

            default: break;
        }

        music_update(&music_sys);
        sound_mix(&sound_sys);
        renderer_present(&renderer, framebuffer);
        SDL_Delay(16);
    }

    music_shutdown(&music_sys);
    sound_shutdown(&sound_sys);
    if (intro_loaded) anm_free(&intro_anim);
    if (textures_loaded) texture_atlas_free(&atlas);
    renderer_shutdown(&renderer);
    SDL_Quit();
    return 0;
}
