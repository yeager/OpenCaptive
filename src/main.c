#include "opencaptive.h"
#include "renderer.h"
#include "game_state.h"
#include "start_menu.h"
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
#include "data_vfs.h"
#include "sha256.h"
#include "liberation_data.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

static uint32_t framebuffer[LIBERATION_SCREEN_WIDTH * LIBERATION_SCREEN_HEIGHT];

static bool write_frame_ppm(const char *path, const uint32_t *pixels,
                            int width, int height) {
    FILE *file = fopen(path, "wb");
    if (!file) return false;
    bool ok = fprintf(file, "P6\n%d %d\n255\n", width, height) > 0;
    for (int y = 0; ok && y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint32_t pixel = pixels[y * width + x];
            uint8_t rgb[3] = {(uint8_t)(pixel >> 16),
                              (uint8_t)(pixel >> 8), (uint8_t)pixel};
            if (fwrite(rgb, 1, sizeof(rgb), file) != sizeof(rgb)) {
                ok = false;
                break;
            }
        }
    }
    if (fclose(file) != 0) ok = false;
    return ok;
}

static bool parse_int_option(const char *text, int minimum, int maximum, int *value) {
    if (!text || !value) return false;
    char *end = NULL;
    errno = 0;
    long parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        parsed < minimum || parsed > maximum || parsed > INT_MAX)
        return false;
    *value = (int)parsed;
    return true;
}

static void get_default_data_path(char *buf, size_t bufsize) {
#ifdef _WIN32
    // Windows: <exe_dir>\data
    char exe_path[512] = {0};
    GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    char *last_sep = strrchr(exe_path, '\\');
    if (last_sep) *last_sep = '\0';
    snprintf(buf, bufsize, "%s\\data", exe_path);
#else
    // Linux/macOS: ~/.opencaptive
    const char *home = getenv("HOME");
    if (home) {
        snprintf(buf, bufsize, "%s/.opencaptive", home);
    } else {
        snprintf(buf, bufsize, ".opencaptive");
    }
#endif
}

static bool validate_data_path(const DataVFS *vfs) {
    static const char *required[] = {
        "71bcf404103f1ac2920800a8bc166939bb49a1204cf51bebce8aca7dd5faafde",
        "1ec1f90adbcfcb3b99b64a56cf1c669b409b7d3a76bc09cedb056f503bfb1959",
        "47ad15b4a593c37880d0306b6a0f51b7a9f20615cf6a188f23716d5b48315524",
        "43833e4a8df622f84d53698a76c6d18f910c1cca79c6b89cbfacc563f695356c",
        "8b7301fc6c302fd673a81d23e7a99d715aa02d5b404c1e1edea19ceccccc9681",
        "519d3ef4494f0e868479a90c8a47249b840598e382c7ba3272f417ce3daf5936",
        "7edb8ee856a91e835ea86dda00af49fda3dae730d694bd7234b7fa96d711e296",
        "978d18857d5ffcf6fb7b91fb22c02b85079db0171caeac3d290a69b276cf098f",
        "dec7143f063c98459ab2f267ed135204cdee1b521eda9810b219e8c10e05c7e8",
        "ce00ba2bc78f160b934486fe101a90264163356e02e7acbea2a41cf5d125b017",
        "21db7daf64cff3b0cae19c3e7eb2057762df9110055e7253175024ecb146fb6b",
        "dfca77f0e219962242226f11f9697f580f92e8ad24786296a5b2571b20c2b707",
    };
    if (!vfs || !vfs->initialized) return false;
    for (size_t i = 0; i < sizeof(required) / sizeof(required[0]); i++) {
        size_t size = 0; uint8_t *data = vfs_find_sha256(vfs, required[i], &size);
        if (!data) return false;
        free(data);
    }
    return true;
}

static void show_missing_liberation_data_dialog(const char *data_path) {
    char msg[768];
    snprintf(msg, sizeof(msg),
        "Verified Liberation: Captive II CD32 data was not found in:\n  %s\n\n"
        "OpenCaptive only accepts a known original data track and verifies it with SHA-256.",
        data_path);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
        "OpenCaptive - Liberation data not found", msg, NULL);
}

static void show_missing_data_dialog(const char *data_path) {
    char msg[1024];
    snprintf(msg, sizeof(msg),
        "Game data files not found!\n\n"
        "OpenCaptive requires the original Captive game data to run.\n\n"
        "Expected location:\n  %s\n\n"
        "Place your Captive game files there, or use:\n"
        "  --data <path>  to specify a different location.\n\n"
        "Required content is identified by SHA-256 manifests, not filenames.\n\n"
        "You can also change the data path in Settings.",
        data_path);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
        "OpenCaptive - Missing Game Data", msg, NULL);
}

static bool load_intro_anm(const DataVFS *vfs, ANMAnimation *anim) {
    size_t size;
    uint8_t *data = vfs_find_sha256(vfs,
        "1ec1f90adbcfcb3b99b64a56cf1c669b409b7d3a76bc09cedb056f503bfb1959", &size);
    if (!data) return false;
    bool ok = anm_decode(data, size, anim);
    free(data);
    return ok;
}

static bool reload_captive_assets(TextureAtlas *atlas, const DataVFS *vfs,
                                  uint32_t **hud_bg) {
    texture_atlas_free(atlas);
    *hud_bg = NULL;
    if (!texture_atlas_load(atlas, vfs)) return false;

    const Texture *screen = gfx_get(&atlas->gfx, atlas->gamescrn_sheet);
    if (screen) *hud_bg = screen->pixels;
    printf("Loaded texture atlas\n");
    return true;
}

static void apply_menu_config(OpenCaptiveConfig *config, const StartMenu *menu) {
    config->data_path = menu->data_path;
    config->render_mode = menu->enhanced_mode
        ? CAPTIVE_RENDER_ENHANCED : CAPTIVE_RENDER_ORIGINAL;
    config->scale_factor = menu->scale_factor;
    config->fullscreen = menu->fullscreen;
    config->vsync = menu->vsync;
    config->scanlines = menu->scanlines;
    config->crt_curvature = menu->crt_curvature;
    config->bilinear = menu->bilinear;
    config->integer_scaling = menu->integer_scaling;
    config->fps_limit = menu->fps_limit;
    config->brightness = menu->brightness;
    config->contrast = menu->contrast;
}

static void sync_menu_from_config(StartMenu *menu, const OpenCaptiveConfig *config,
                                  bool music_enabled, bool sfx_enabled) {
    start_menu_init(menu);
    strncpy(menu->data_path, config->data_path, sizeof(menu->data_path) - 1);
    menu->data_path[sizeof(menu->data_path) - 1] = '\0';
    menu->data_path_cursor = (int)strlen(menu->data_path);
    menu->enhanced_mode = config->render_mode == CAPTIVE_RENDER_ENHANCED;
    menu->music_enabled = music_enabled;
    menu->sfx_enabled = sfx_enabled;
    menu->fullscreen = config->fullscreen;
    menu->vsync = config->vsync;
    menu->scanlines = config->scanlines;
    menu->crt_curvature = config->crt_curvature;
    menu->bilinear = config->bilinear;
    menu->integer_scaling = config->integer_scaling;
    menu->fps_limit = config->fps_limit;
    menu->brightness = config->brightness;
    menu->contrast = config->contrast;
    menu->scale_factor = config->scale_factor;
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
static LiberationData liberation_data;
static bool liberation_intro_active;
static bool skip_liberation_intro_requested;


typedef struct {
    bool open;
    int selected;
    bool invulnerable;
    bool infinite_energy;
} RuntimePopup;

static RuntimePopup runtime_popup;

enum {
    POPUP_ENHANCED,
    POPUP_SCANLINES,
    POPUP_CRT,
    POPUP_BILINEAR,
    POPUP_BRIGHTNESS,
    POPUP_MUSIC,
    POPUP_SFX,
    POPUP_INVULNERABLE,
    POPUP_INFINITE_ENERGY,
    POPUP_COMPLETE_OBJECTIVE,
    POPUP_CLOSE,
    POPUP_ITEMS,
};

static const uint8_t simple_font[][5] = {
    ['A'] = {0x7C,0x12,0x12,0x12,0x7C}, ['B'] = {0x7E,0x4A,0x4A,0x4A,0x34},
    ['C'] = {0x3C,0x42,0x42,0x42,0x24}, ['D'] = {0x7E,0x42,0x42,0x42,0x3C},
    ['E'] = {0x7E,0x4A,0x4A,0x4A,0x42}, ['F'] = {0x7E,0x0A,0x0A,0x0A,0x02},
    ['G'] = {0x3C,0x42,0x52,0x52,0x34}, ['H'] = {0x7E,0x08,0x08,0x08,0x7E},
    ['I'] = {0x42,0x42,0x7E,0x42,0x42}, ['J'] = {0x20,0x40,0x42,0x3E,0x02},
    ['K'] = {0x7E,0x08,0x14,0x22,0x40}, ['L'] = {0x7E,0x40,0x40,0x40,0x40},
    ['M'] = {0x7E,0x04,0x08,0x04,0x7E}, ['N'] = {0x7E,0x04,0x08,0x10,0x7E},
    ['O'] = {0x3C,0x42,0x42,0x42,0x3C}, ['P'] = {0x7E,0x12,0x12,0x12,0x0C},
    ['Q'] = {0x3C,0x42,0x52,0x22,0x5C}, ['R'] = {0x7E,0x12,0x32,0x52,0x0C},
    ['S'] = {0x24,0x4A,0x4A,0x4A,0x30}, ['T'] = {0x02,0x02,0x7E,0x02,0x02},
    ['U'] = {0x3E,0x40,0x40,0x40,0x3E}, ['V'] = {0x1E,0x20,0x40,0x20,0x1E},
    ['W'] = {0x3E,0x40,0x30,0x40,0x3E}, ['X'] = {0x62,0x14,0x08,0x14,0x62},
    ['Y'] = {0x06,0x08,0x70,0x08,0x06}, ['Z'] = {0x62,0x52,0x4A,0x46,0x42},
    ['0'] = {0x3C,0x52,0x4A,0x46,0x3C}, ['1'] = {0x44,0x42,0x7E,0x40,0x40},
    ['2'] = {0x64,0x52,0x52,0x52,0x4C}, ['3'] = {0x24,0x42,0x4A,0x4A,0x34},
    ['4'] = {0x1E,0x10,0x10,0x7E,0x10}, ['5'] = {0x2E,0x4A,0x4A,0x4A,0x32},
    ['6'] = {0x3C,0x4A,0x4A,0x4A,0x30}, ['7'] = {0x02,0x72,0x0A,0x06,0x02},
    ['8'] = {0x34,0x4A,0x4A,0x4A,0x34}, ['9'] = {0x0C,0x52,0x52,0x52,0x3C},
    ['!'] = {0x00,0x00,0x5E,0x00,0x00}, [' '] = {0x00,0x00,0x00,0x00,0x00},
    ['.'] = {0x00,0x40,0x00,0x00,0x00}, [':'] = {0x00,0x36,0x36,0x00,0x00},
    ['-'] = {0x08,0x08,0x08,0x08,0x08}, ['/'] = {0x60,0x10,0x08,0x04,0x03},
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

static void draw_rect(uint32_t *fb, int pw, int ph,
                      int x, int y, int w, int h, uint32_t color) {
    for (int py = y; py < y + h; py++) {
        if (py < 0 || py >= ph) continue;
        for (int px = x; px < x + w; px++)
            if (px >= 0 && px < pw) fb[py * pw + px] = color;
    }
}

static const char *popup_toggle(bool value) {
    return value ? "ON" : "OFF";
}

static const char *popup_brightness(int value) {
    return value < 40 ? "LOW" : (value > 60 ? "HIGH" : "NORMAL");
}

static void popup_apply_cheats(GameState *gs) {
    /* Captive's original save/state format has not been recovered.  A cheat
       toggle must not silently mutate the old generated mission state. */
    (void)gs;
}

static void popup_handle_event(GameState *gs, OpenCaptiveConfig *config,
                               OpenCaptiveRenderer *renderer, const SDL_Event *event) {
    if (!event || event->type != SDL_EVENT_KEY_DOWN) return;
    SDL_Keycode key = event->key.key;
    if (key == SDLK_F10 || key == SDLK_ESCAPE) {
        runtime_popup.open = false;
        return;
    }
    if (key == SDLK_UP) {
        runtime_popup.selected = (runtime_popup.selected + POPUP_ITEMS - 1) % POPUP_ITEMS;
        return;
    }
    if (key == SDLK_DOWN) {
        runtime_popup.selected = (runtime_popup.selected + 1) % POPUP_ITEMS;
        return;
    }
    if (key != SDLK_LEFT && key != SDLK_RIGHT && key != SDLK_RETURN && key != SDLK_KP_ENTER)
        return;

    switch (runtime_popup.selected) {
        case POPUP_ENHANCED:
            /* There is verified Captive viewport media, but its original
             * composition routine is not reconstructed yet.  Do not replace
             * it with the former generated corridor just because F10 was
             * pressed.  Keep old configuration files compatible by
             * normalising this obsolete choice to the original path. */
            config->render_mode = CAPTIVE_RENDER_ORIGINAL;
            break;
        case POPUP_SCANLINES: config->scanlines = !config->scanlines; break;
        case POPUP_CRT: config->crt_curvature = !config->crt_curvature; break;
        case POPUP_BILINEAR: config->bilinear = !config->bilinear; break;
        case POPUP_BRIGHTNESS:
            config->brightness = config->brightness < 50 ? 50 :
                (config->brightness < 75 ? 75 : 25);
            break;
        case POPUP_MUSIC: music_set_enabled(&music_sys, !music_sys.enabled); break;
        case POPUP_SFX: sound_set_enabled(&sound_sys, !sound_sys.enabled); break;
        case POPUP_INVULNERABLE:
        case POPUP_INFINITE_ENERGY:
        case POPUP_COMPLETE_OBJECTIVE:
            /* Keep the option visible, but do not fabricate an effect until
             * the original game-state and command format is decoded. */
            break;
        case POPUP_CLOSE: runtime_popup.open = false; break;
    }
    gs->config = *config;
    renderer_set_effects(renderer, config->bilinear, config->scanlines,
                         config->crt_curvature,
                         config->brightness, config->contrast);
}

static void popup_render(const GameState *gs, uint32_t *fb, int pw, int ph) {
    static const char *labels[POPUP_ITEMS] = {
        "VIEW RECONSTRUCTION", "SCANLINES", "CRT CURVE", "BILINEAR",
        "BRIGHTNESS", "MUSIC", "SFX", "GOD MODE", "INFINITE ENERGY",
        "COMPLETE OBJECTIVE", "CLOSE",
    };
    int x = 30, y = 18, w = pw - 60, h = 164;
    draw_rect(fb, pw, ph, x, y, w, h, 0xFF101420);
    draw_rect(fb, pw, ph, x, y, w, 2, 0xFF55CCFF);
    draw_rect(fb, pw, ph, x, y + h - 2, w, 2, 0xFF55CCFF);
    draw_centered(fb, pw, ph, y + 8, "RUNTIME OPTIONS", 0xFFFFFFFF, 1);
    draw_centered(fb, pw, ph, y + 19, "ESC CLOSE", 0xFF99AACC, 1);
    for (int i = 0; i < POPUP_ITEMS; i++) {
        int row_y = y + 34 + i * 13;
        uint32_t color = i == runtime_popup.selected ? 0xFFFFFF44 : 0xFFCCDDEE;
        draw_simple_text(fb, pw, ph, x + 12, row_y, labels[i], color, 1);
        const char *value = "";
        switch (i) {
            case POPUP_ENHANCED: value = "PENDING"; break;
            case POPUP_SCANLINES: value = popup_toggle(gs->config.scanlines); break;
            case POPUP_CRT: value = popup_toggle(gs->config.crt_curvature); break;
            case POPUP_BILINEAR: value = popup_toggle(gs->config.bilinear); break;
            case POPUP_BRIGHTNESS: value = popup_brightness(gs->config.brightness); break;
            case POPUP_MUSIC: value = popup_toggle(music_sys.enabled); break;
            case POPUP_SFX: value = popup_toggle(sound_sys.enabled); break;
            case POPUP_INVULNERABLE:
            case POPUP_INFINITE_ENERGY:
            case POPUP_COMPLETE_OBJECTIVE: value = "PENDING"; break;
            default: break;
        }
        draw_simple_text(fb, pw, ph, x + w - 80, row_y, value, color, 1);
    }
    draw_centered(fb, pw, ph, y + h - 14, "UP DOWN ENTER", 0xFF99AACC, 1);
}

static void spawn_level_content(GameState *gs_ptr) {
    combat_init(&creatures);
    puzzle_init(&puzzles);
    for (int i = 0; i < gs_ptr->num_levels; i++) {
        combat_spawn_for_level(&creatures, &gs_ptr->levels[i], i, gs_ptr->mission_seed);
        puzzle_generate(&puzzles, &gs_ptr->levels[i], i, gs_ptr->mission_seed);
    }
}

static void start_liberation_session(GameState *gs_ptr) {
    gs_ptr->game_type = GAME_LIBERATION;
    gs_ptr->mode = STATE_GAME;
    liberation_intro_active = !skip_liberation_intro_requested &&
                              liberation_data.intro_frame.bitplanes != NULL;
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
            save_game(gs, &creatures, &puzzles, "opencaptive.sav");
            return;
        case SDLK_F9:
            load_game(gs, &creatures, &puzzles, "opencaptive.sav");
            return;
        case SDLK_PERIOD: // > stairs down
            game_state_change_floor(gs, 1);
            return;
        case SDLK_COMMA: // < stairs up
            game_state_change_floor(gs, -1);
            return;
        default: return;
    }

    // Try to move
    int nx = gs->party_x + dx;
    int ny = gs->party_y + dy;
    if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
        CellType cell = lvl->cells[ny][nx].type;
        if (cell != CELL_WALL && cell != CELL_DOOR && cell != CELL_DOOR_LOCKED) {
            gs->party_x = nx;
            gs->party_y = ny;
            sfx_play(&sfx, SFX_STEP);
        } else if (cell == CELL_DOOR_LOCKED) {
            sfx_play(&sfx, SFX_DOOR_LOCKED);
        }
    }
}

int main(int argc, char *argv[]) {
    printf("OpenCaptive v%d.%d.%d by Daniel Nylander\n",
           OPENCAPTIVE_VERSION_MAJOR,
           OPENCAPTIVE_VERSION_MINOR,
           OPENCAPTIVE_VERSION_PATCH);

    static char default_data_path[512];
    get_default_data_path(default_data_path, sizeof(default_data_path));

    OpenCaptiveConfig config = {
        .platform = CAPTIVE_PLATFORM_DOS,
        .render_mode = CAPTIVE_RENDER_ORIGINAL,
        .data_path = default_data_path,
        .scale_factor = 3,
        .vsync = true,
        .integer_scaling = true,
        .brightness = 50,
        .contrast = 50,
        .fps_limit = 60,
    };
    GameType requested_game = GAME_CAPTIVE;
    bool start_directly = false;
    const char *verify_data = NULL;
    const char *capture_frame_path = NULL;
    int exit_status = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf(
                "OpenCaptive v%d.%d.%d by Daniel Nylander\n"
                "A reimplementation of Captive (1990) and Liberation: Captive 2 (1993)\n\n"
                "Usage: opencaptive [options]\n\n"
                "Options:\n"
                "  --help, -h            Show this help message\n"
                "  --version, -v         Show version information\n"
                "  --data <path>         Set game data directory\n"
                "  --enhanced            Enable enhanced 3D renderer\n"
                "  --platform <name>     Set platform: dos, atari, amiga\n"
                "  --scale <n>           Window scale factor (1-5, default 3)\n"
                "  --fullscreen          Start in fullscreen mode\n"
                "  --vsync               Enable vertical sync (default)\n"
                "  --no-vsync            Disable vertical sync\n"
                "  --scanlines           Enable CRT scanline effect\n"
                "  --crt                 Enable CRT curvature effect\n"
                "  --bilinear            Enable bilinear texture filtering\n"
                "  --integer-scaling     Enable integer scaling (default)\n"
                "  --no-integer-scaling  Disable integer scaling\n"
                "  --fps <n>             FPS limit: 0 (unlimited), 30, 60, 120\n"
                "  --brightness <n>      Brightness 0-100 (default 50)\n"
                "  --contrast <n>        Contrast 0-100 (default 50)\n"
                "  --game <name>         Start game directly: captive, liberation\n\n"
                "  --verify-data <name>  Verify data by SHA-256: captive, liberation, all\n\n"
                "  --capture-frame <ppm> Save one unscaled native game frame, then exit\n\n"
                "  --skip-intro          Skip Liberation's original intro (automation only)\n\n"
                "Game data:\n"
                "  Place original Captive game files (or ZIP archives containing them)\n"
                "  in the data directory. Default location:\n"
#ifdef _WIN32
                "    <install dir>\\data\n"
#else
                "    ~/.opencaptive\n"
#endif
                "  ZIP files are read transparently.\n",
                OPENCAPTIVE_VERSION_MAJOR, OPENCAPTIVE_VERSION_MINOR,
                OPENCAPTIVE_VERSION_PATCH);
            return 0;
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("OpenCaptive v%d.%d.%d by Daniel Nylander\n",
                   OPENCAPTIVE_VERSION_MAJOR, OPENCAPTIVE_VERSION_MINOR,
                   OPENCAPTIVE_VERSION_PATCH);
            return 0;
        } else if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) {
            config.data_path = argv[++i];
        } else if (strcmp(argv[i], "--enhanced") == 0) {
            config.render_mode = CAPTIVE_RENDER_ENHANCED;
        } else if (strcmp(argv[i], "--platform") == 0 && i + 1 < argc) {
            const char *platform = argv[++i];
            if (strcmp(platform, "dos") == 0) config.platform = CAPTIVE_PLATFORM_DOS;
            else if (strcmp(platform, "atari") == 0) config.platform = CAPTIVE_PLATFORM_ATARI_ST;
            else if (strcmp(platform, "amiga") == 0) config.platform = CAPTIVE_PLATFORM_AMIGA;
            else {
                fprintf(stderr, "Unknown platform: %s (expected dos, atari or amiga)\n", platform);
                return 2;
            }
        } else if (strcmp(argv[i], "--scale") == 0 && i + 1 < argc) {
            if (!parse_int_option(argv[++i], 1, 5, &config.scale_factor)) {
                fprintf(stderr, "--scale must be an integer from 1 to 5\n");
                return 2;
            }
        } else if (strcmp(argv[i], "--fullscreen") == 0) {
            config.fullscreen = true;
        } else if (strcmp(argv[i], "--vsync") == 0) {
            config.vsync = true;
        } else if (strcmp(argv[i], "--no-vsync") == 0) {
            config.vsync = false;
        } else if (strcmp(argv[i], "--scanlines") == 0) {
            config.scanlines = true;
        } else if (strcmp(argv[i], "--crt") == 0) {
            config.crt_curvature = true;
        } else if (strcmp(argv[i], "--bilinear") == 0) {
            config.bilinear = true;
        } else if (strcmp(argv[i], "--integer-scaling") == 0) {
            config.integer_scaling = true;
        } else if (strcmp(argv[i], "--no-integer-scaling") == 0) {
            config.integer_scaling = false;
        } else if (strcmp(argv[i], "--fps") == 0 && i + 1 < argc) {
            if (!parse_int_option(argv[++i], 0, 120, &config.fps_limit) ||
                (config.fps_limit != 0 && config.fps_limit != 30 &&
                 config.fps_limit != 60 && config.fps_limit != 120)) {
                fprintf(stderr, "--fps must be one of 0, 30, 60 or 120\n");
                return 2;
            }
        } else if (strcmp(argv[i], "--brightness") == 0 && i + 1 < argc) {
            if (!parse_int_option(argv[++i], 0, 100, &config.brightness)) {
                fprintf(stderr, "--brightness must be an integer from 0 to 100\n");
                return 2;
            }
        } else if (strcmp(argv[i], "--contrast") == 0 && i + 1 < argc) {
            if (!parse_int_option(argv[++i], 0, 100, &config.contrast)) {
                fprintf(stderr, "--contrast must be an integer from 0 to 100\n");
                return 2;
            }
        } else if (strcmp(argv[i], "--game") == 0 && i + 1 < argc) {
            const char *game = argv[++i];
            if (strcmp(game, "captive") == 0) {
                requested_game = GAME_CAPTIVE;
                start_directly = true;
            } else if (strcmp(game, "liberation") == 0) {
                requested_game = GAME_LIBERATION;
                start_directly = true;
            } else {
                fprintf(stderr, "Unknown game: %s (expected captive or liberation)\n", game);
                return 2;
            }
        } else if (strcmp(argv[i], "--verify-data") == 0 && i + 1 < argc) {
            verify_data = argv[++i];
            if (strcmp(verify_data, "captive") != 0 &&
                strcmp(verify_data, "liberation") != 0 &&
                strcmp(verify_data, "all") != 0) {
                fprintf(stderr, "Unknown data set: %s (expected captive, liberation or all)\n", verify_data);
                return 2;
            }
        } else if (strcmp(argv[i], "--capture-frame") == 0 && i + 1 < argc) {
            capture_frame_path = argv[++i];
        } else if (strcmp(argv[i], "--skip-intro") == 0) {
            skip_liberation_intro_requested = true;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 2;
        }
    }

    if (verify_data) {
        DataVFS verify_vfs;
        LiberationData verify_liberation = {0};
        bool vfs_ok = vfs_init(&verify_vfs, config.data_path);
        bool ok = vfs_ok;
        bool check_captive = strcmp(verify_data, "liberation") != 0;
        bool check_liberation = strcmp(verify_data, "captive") != 0;
        if (check_captive) {
            bool valid = vfs_ok && validate_data_path(&verify_vfs);
            printf("Captive data: %s\n", valid ? "verified" : "not verified");
            ok = ok && valid;
        }
        if (check_liberation) {
            bool valid = vfs_ok && liberation_data_open(&verify_liberation, &verify_vfs);
            printf("Liberation data: %s\n", valid ? "verified" : "not verified");
            if (valid) {
                printf("Liberation presentation: intro=%s/%s city=%s/%s\n",
                       verify_liberation.intro_frame.bitplanes ? "decoded" : "unavailable",
                       verify_liberation.intro_script.bytes ? "SCPT" : "no-SCPT",
                       verify_liberation.city_frame.bitplanes ? "decoded" : "unavailable",
                       verify_liberation.city_script.bytes ? "SCPT" : "no-SCPT");
            }
            liberation_data_close(&verify_liberation);
            ok = ok && valid;
        }
        vfs_free(&verify_vfs);
        return ok ? 0 : 1;
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
    StartMenu menu = {0};
    sync_menu_from_config(&menu, &config, true, true);

    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    gs.config = config;

    // Virtual filesystem
    DataVFS vfs;
    vfs_init(&vfs, config.data_path);

    // Audio
    sound_init(&sound_sys);
    music_init(&music_sys, &sound_sys, &vfs);

    // Items and SFX
    item_db_init(&item_db);
    sfx_init(&sfx, &sound_sys);
    // Texture atlas
    TextureAtlas atlas = {0};
    uint32_t *hud_bg = NULL;
    bool textures_loaded = false;
    if (config.data_path) {
        textures_loaded = reload_captive_assets(&atlas, &vfs, &hud_bg);
    }

    // Intro animation (loaded on demand)
    ANMAnimation intro_anim = {0};
    bool intro_loaded = false;
    int intro_frame = 0;
    uint32_t intro_last_tick = 0;

    /* Keep command-line launches deterministic and useful for scripts and
     * desktop shortcuts.  The original HUD and intro are decoded from media;
     * do not seed the former generated dungeon beneath them. */
    if (start_directly && requested_game == GAME_CAPTIVE) {
        if (!validate_data_path(&vfs)) {
            show_missing_data_dialog(config.data_path);
        } else {
            gs.game_type = GAME_CAPTIVE;
            music_play(&music_sys, MUSIC_BASE);
            /* game_state_init() starts at the menu.  A direct command-line
             * launch must make the same state transition as selecting
             * Captive in the menu; without this the prepared mission was
             * hidden behind the start screen. */
            gs.mode = STATE_GAME;
            printf("Starting verified Captive game\n");
        }
    } else if (start_directly && requested_game == GAME_LIBERATION) {
        if (!liberation_data_open(&liberation_data, &vfs)) {
            show_missing_liberation_data_dialog(config.data_path);
        } else {
            start_liberation_session(&gs);
            printf("Starting verified Liberation game\n");
        }
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        uint64_t frame_started = SDL_GetTicks();
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                break;
            }

            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F10 &&
                gs.mode == STATE_GAME) {
                runtime_popup.open = !runtime_popup.open;
                continue;
            }
            if (runtime_popup.open) {
                popup_handle_event(&gs, &config, &renderer, &event);
                continue;
            }

            switch (gs.mode) {
                case STATE_MENU: {
                    MenuResult result;
                    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                        event.button.button == SDL_BUTTON_LEFT) {
                        int width = CAPTIVE_ORIGINAL_WIDTH;
                        int height = CAPTIVE_ORIGINAL_HEIGHT;
                        SDL_GetWindowSize(renderer.window, &width, &height);
                        float x = event.button.x * CAPTIVE_ORIGINAL_WIDTH / width;
                        float y = event.button.y * CAPTIVE_ORIGINAL_HEIGHT / height;
                        result = start_menu_handle_click(&menu, x, y);
                    } else {
                        result = start_menu_handle_event(&menu, &event);
                    }
                    switch (result) {
                        case MENU_RESULT_START_CAPTIVE:
                            gs.game_type = GAME_CAPTIVE;
                            apply_menu_config(&config, &menu);
                            gs.config = config;
                            renderer_set_effects(&renderer, config.bilinear,
                                                 config.scanlines,
                                                 config.crt_curvature,
                                                 config.brightness,
                                                 config.contrast);
                            renderer_apply_display(&renderer, &config);
                            music_set_enabled(&music_sys, menu.music_enabled);
                            sound_set_enabled(&sound_sys, menu.sfx_enabled);
                            vfs_free(&vfs);
                            vfs_init(&vfs, config.data_path);
                            if (!validate_data_path(&vfs)) {
                                show_missing_data_dialog(config.data_path);
                                break;
                            }
                            textures_loaded = reload_captive_assets(&atlas, &vfs, &hud_bg);
                            if (intro_loaded) {
                                anm_free(&intro_anim);
                                intro_loaded = false;
                            }
                            gs.mode = STATE_INTRO;
                            if (!intro_loaded) {
                                intro_loaded = load_intro_anm(&vfs, &intro_anim);
                                intro_frame = 0;
                                intro_last_tick = SDL_GetTicks();
                            }
                            if (!intro_loaded) {
                                music_play(&music_sys, MUSIC_BASE);
                                gs.mode = STATE_GAME;
                            }
                            break;
                        case MENU_RESULT_START_LIBERATION:
                            apply_menu_config(&config, &menu);
                            gs.config = config;
                            renderer_set_effects(&renderer, config.bilinear,
                                                 config.scanlines,
                                                 config.crt_curvature,
                                                 config.brightness,
                                                 config.contrast);
                            renderer_apply_display(&renderer, &config);
                            music_set_enabled(&music_sys, menu.music_enabled);
                            sound_set_enabled(&sound_sys, menu.sfx_enabled);
                            vfs_free(&vfs);
                            vfs_init(&vfs, config.data_path);
                            liberation_data_close(&liberation_data);
                            if (!liberation_data_open(&liberation_data, &vfs)) {
                                show_missing_liberation_data_dialog(config.data_path);
                                break;
                            }
                            start_liberation_session(&gs);
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
                        music_play(&music_sys, MUSIC_BASE);
                        gs.mode = STATE_GAME;
                    }
                    break;
                case STATE_GAME:
                    if (event.type == SDL_EVENT_KEY_DOWN &&
                        event.key.key == SDLK_ESCAPE) {
                        if (gs.game_type == GAME_LIBERATION && liberation_intro_active) {
                            liberation_intro_active = false;
                        } else {
                            gs.mode = STATE_MENU;
                            sync_menu_from_config(&menu, &config, music_sys.enabled,
                                                  sound_sys.enabled);
                            music_stop(&music_sys);
                        }
                    } else if (gs.game_type == GAME_LIBERATION) {
                        /* The verified ANIM presentation can advance from its
                         * intro to the city frame.  Movement and building
                         * entry remain disabled until their original CD32
                         * game-state format is recovered. */
                        if (event.type == SDL_EVENT_KEY_DOWN)
                            liberation_intro_active = false;
                    } /* Captive input remains disabled until the original
                         * map/state format is decoded. */
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

        /* Liberation is a PAL CD32 presentation and therefore uses a taller
         * canvas than Captive's 320x200 shell.  Switch at the game boundary,
         * not by stretching one game's framebuffer into the other. */
        int frame_width = CAPTIVE_ORIGINAL_WIDTH;
        int frame_height = (gs.mode == STATE_GAME && gs.game_type == GAME_LIBERATION)
            ? LIBERATION_SCREEN_HEIGHT : CAPTIVE_ORIGINAL_HEIGHT;
        if (renderer.canvas_width != frame_width || renderer.canvas_height != frame_height)
            renderer_set_canvas(&renderer, frame_width, frame_height, &config);

        // Render
        memset(framebuffer, 0, (size_t)frame_width * frame_height * sizeof(uint32_t));

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
                if (!runtime_popup.open) popup_apply_cheats(&gs);
                if (gs.game_type == GAME_LIBERATION) {
                    /* Liberation has its own city/building loop.  Captive's
                     * droid combat and generator completion conditions do not
                     * apply here: running them caused an unattended city game
                     * to advance or end after its timer elapsed. */
                    memset(framebuffer, 0, sizeof(framebuffer));
                    if (liberation_data.city_frame.bitplanes) {
                        const LiberationAnimFrame *frame = liberation_intro_active
                            ? &liberation_data.intro_frame : &liberation_data.city_frame;
                        int y = liberation_intro_active ? 47 : 44;
                        liberation_anim_blit(frame, framebuffer,
                                             LIBERATION_SCREEN_WIDTH,
                                             LIBERATION_SCREEN_HEIGHT, 0, y);
                    } else {
                        draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                      LIBERATION_SCREEN_HEIGHT, 118,
                                      "VERIFIED LIBERATION PRESENTATION DATA REQUIRED",
                                      0xFFCCDDEE, 1);
                    }
                } else {
                    /* Captive: the verified original GAME SCRN shell. */
                    if (hud_bg) {
                        memcpy(framebuffer, hud_bg,
                               CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT * sizeof(uint32_t));
                        /* GAME SCRN is a title/menu source frame; its dungeon
                         * rectangle contains a Captive logo, not an in-game
                         * view.  Once a game is selected, the original
                         * renderer replaces this exact interior.  Clear it
                         * until that renderer is recovered rather than
                         * passing title pixels off as a game scene. */
                        for (int y = 0; y < CAPTIVE_VIEWPORT_HEIGHT; ++y) {
                            uint32_t *row = framebuffer +
                                (CAPTIVE_VIEWPORT_Y + y) * CAPTIVE_ORIGINAL_WIDTH +
                                CAPTIVE_VIEWPORT_X;
                            memset(row, 0, CAPTIVE_VIEWPORT_WIDTH * sizeof(*row));
                        }
                    }
                    /* The original GAME SCRN frame is decoded from verified
                     * media.  Until the original viewport compositor is
                     * reconstructed, leave its dungeon window untouched.
                     * The former F10 path drew a generated corridor, lighting
                     * and substitute creature shapes from inferred state; it
                     * is deliberately not invoked when original data exists. */
                    (void)textures_loaded;
                    /* The original GAME SCRN resource already contains the
                     * complete control and status-panel shell.  Do not paint
                     * the replacement HUD over it in original-render mode. */
                    if (config.render_mode == CAPTIVE_RENDER_ENHANCED)
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

        if (runtime_popup.open && gs.mode == STATE_GAME)
            popup_render(&gs, framebuffer, frame_width, frame_height);

        if (capture_frame_path) {
            if (!write_frame_ppm(capture_frame_path, framebuffer, frame_width, frame_height)) {
                fprintf(stderr, "Could not write frame capture: %s\n", capture_frame_path);
                exit_status = 1;
                running = false;
            } else {
                printf("Wrote native frame capture: %s\n", capture_frame_path);
                running = false;
            }
        }

        music_update(&music_sys);
        sound_mix(&sound_sys);
        renderer_present(&renderer, framebuffer);
        if (config.fps_limit > 0) {
            uint64_t elapsed = SDL_GetTicks() - frame_started;
            uint64_t frame_budget = 1000U / (uint64_t)config.fps_limit;
            if (elapsed < frame_budget) SDL_Delay((uint32_t)(frame_budget - elapsed));
        }
    }

    vfs_free(&vfs);
    liberation_data_close(&liberation_data);
    music_shutdown(&music_sys);
    sound_shutdown(&sound_sys);
    if (intro_loaded) anm_free(&intro_anim);
    if (textures_loaded) texture_atlas_free(&atlas);
    renderer_shutdown(&renderer);
    SDL_Quit();
    return exit_status;
}
