#include "opencaptive.h"
#include "renderer.h"
#include "game_state.h"
#include "start_menu.h"
#include "hud.h"
#include "anm_decoder.h"
#include "pl5_decoder.h"
#include "combat.h"
#include "map_gen.h"
#include "save_load.h"
#include "texture_atlas.h"
#include "viewport.h"
#include "captive_view_window.h"
#include "captive_data.h"
#include "music.h"
#include "puzzle.h"
#include "sound.h"
#include "shop.h"
#include "inventory.h"
#include "droid_ui.h"
#include "terminal.h"
#include "sfx.h"
#include "data_vfs.h"
#include "captive_amiga_data.h"
#include "sha256.h"
#include "liberation_data.h"
#include "liberation_citygen.h"
#include "liberation_citygen_grid.h"
#include "liberation_city_nav.h"
#include "liberation_building_interact.h"
#include "liberation_viewport_3d.h"
#include "liberation_save.h"
#include "liberation_combat.h"
#include "liberation_plotgen.h"
#include "amos_sprite.h"
#include "dos_vga_reference.h"
#include "frame_compare.h"
#include "custom_features.h"
#include "i18n.h"
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

static uint32_t framebuffer[MENU_WIDTH * MENU_HEIGHT];

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

/* Hash the same P6 byte stream produced by the independent presentation
 * capture tool.  This makes --verify-data test decoded source pixels, not
 * merely the outer CD32 track and container boundaries. */
static bool liberation_frame_matches_ppm_sha256(const LiberationAnimFrame *frame,
                                                uint16_t expected_width,
                                                uint16_t expected_height,
                                                const char expected_sha256[65]) {
    if (!frame || !frame->bitplanes || frame->width != expected_width ||
        frame->height != expected_height || !expected_sha256) {
        return false;
    }
    const size_t count = (size_t)frame->width * frame->height;
    uint32_t *pixels = calloc(count, sizeof(*pixels));
    if (!pixels) return false;
    liberation_anim_blit(frame, pixels, frame->width, frame->height, 0, 0);

    char header[32];
    int header_size = snprintf(header, sizeof(header), "P6\n%u %u\n255\n",
                               frame->width, frame->height);
    SHA256Context hash;
    uint8_t digest[32];
    sha256_init(&hash);
    if (header_size <= 0 || (size_t)header_size >= sizeof(header)) {
        free(pixels);
        return false;
    }
    sha256_update(&hash, (const uint8_t *)header, (size_t)header_size);
    for (size_t i = 0; i < count; ++i) {
        uint8_t rgb[3] = {(uint8_t)(pixels[i] >> 16),
                          (uint8_t)(pixels[i] >> 8), (uint8_t)pixels[i]};
        sha256_update(&hash, rgb, sizeof(rgb));
    }
    sha256_final(&hash, digest);
    free(pixels);
    return sha256_matches_hex(digest, expected_sha256);
}

static bool write_dos_vga_reference(const char *dump_path, const char *output_path) {
    FILE *file = fopen(dump_path, "rb");
    if (!file) {
        fprintf(stderr, "Unable to open DOS VGA reference input\n");
        return false;
    }
    uint8_t *memory = malloc(DOS_VGA_MEMORY_SIZE);
    if (!memory) {
        fclose(file);
        return false;
    }
    size_t read = fread(memory, 1, DOS_VGA_MEMORY_SIZE, file);
    int trailing = fgetc(file);
    bool ok = read == DOS_VGA_MEMORY_SIZE && trailing == EOF;
    if (fclose(file) != 0) ok = false;
    if (!ok) {
        fprintf(stderr, "DOS VGA reference must be exactly 1048576 bytes\n");
        free(memory);
        return false;
    }

    uint32_t pixels[DOS_VGA_FRAME_SIZE];
    uint8_t digest[32];
    char digest_text[65];
    sha256_digest(memory, DOS_VGA_MEMORY_SIZE, digest);
    for (size_t i = 0; i < sizeof(digest); ++i)
        snprintf(digest_text + i * 2, 3, "%02x", digest[i]);
    ok = dos_vga_reference_decode(memory, DOS_VGA_MEMORY_SIZE, pixels,
                                  DOS_VGA_FRAME_SIZE) &&
         write_frame_ppm(output_path, pixels, DOS_VGA_FRAME_WIDTH,
                         DOS_VGA_FRAME_HEIGHT);
    free(memory);
    if (ok)
        printf("DOS VGA reference SHA-256: %s\n", digest_text);
    return ok;
}

static uint32_t *read_ppm_frame(const char *path, int *out_width, int *out_height) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    char magic[3] = {0};
    int width = 0, height = 0, maximum = 0;
    if (fscanf(file, "%2s%d%d%d", magic, &width, &height, &maximum) != 4 ||
        strcmp(magic, "P6") != 0 || width <= 0 || height <= 0 || maximum != 255 ||
        fgetc(file) == EOF) {
        fclose(file);
        return NULL;
    }
    size_t count = (size_t)width * (size_t)height;
    if (count > SIZE_MAX / sizeof(uint32_t) || count > SIZE_MAX / 3) {
        fclose(file);
        return NULL;
    }
    uint32_t *pixels = malloc(count * sizeof(*pixels));
    if (!pixels) { fclose(file); return NULL; }
    for (size_t i = 0; i < count; ++i) {
        uint8_t rgb[3];
        if (fread(rgb, 1, sizeof(rgb), file) != sizeof(rgb)) {
            free(pixels); fclose(file); return NULL;
        }
        pixels[i] = 0xFF000000u | ((uint32_t)rgb[0] << 16) |
            ((uint32_t)rgb[1] << 8) | rgb[2];
    }
    if (fgetc(file) != EOF || ferror(file) || fclose(file) != 0) {
        free(pixels); return NULL;
    }
    *out_width = width;
    *out_height = height;
    return pixels;
}

static int compare_ppm_frames(const char *expected_path, const char *actual_path,
                              const int *rect) {
    int expected_width = 0, expected_height = 0, actual_width = 0, actual_height = 0;
    uint32_t *expected = read_ppm_frame(expected_path, &expected_width, &expected_height);
    uint32_t *actual = read_ppm_frame(actual_path, &actual_width, &actual_height);
    if (!expected || !actual || expected_width != actual_width ||
        expected_height != actual_height) {
        fprintf(stderr, "PPM frames must be complete P6 images with equal dimensions\n");
        free(expected); free(actual);
        return 2;
    }
    int x = 0, y = 0, width = expected_width, height = expected_height;
    if (rect) {
        x = rect[0]; y = rect[1]; width = rect[2]; height = rect[3];
        if (x < 0 || y < 0 || width <= 0 || height <= 0 ||
            width > expected_width - x || height > expected_height - y) {
            fprintf(stderr, "Comparison rectangle is outside the frame\n");
            free(expected); free(actual);
            return 2;
        }
    }
    FrameComparison result = {0};
    for (int row = 0; row < height; ++row) {
        FrameComparison line = frame_compare_argb(
            expected + (size_t)(y + row) * expected_width + x,
            actual + (size_t)(y + row) * actual_width + x, (size_t)width);
        result.pixel_count += line.pixel_count;
        result.different_pixels += line.different_pixels;
        result.total_channel_difference += line.total_channel_difference;
        if (line.maximum_channel_difference > result.maximum_channel_difference)
            result.maximum_channel_difference = line.maximum_channel_difference;
    }
    printf("Frame comparison%s: %zu/%zu pixels differ; channel difference=%llu; max=%u\n",
           rect ? " (rectangle)" : "",
           result.different_pixels, result.pixel_count,
           (unsigned long long)result.total_channel_difference,
           result.maximum_channel_difference);
    free(expected); free(actual);
    return result.different_pixels == 0 ? 0 : 1;
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

#define MSG_LOG_SIZE 4
#define MSG_LOG_TTL  180
static struct {
    char text[64];
    uint32_t color;
    int ttl;
} msg_log[MSG_LOG_SIZE];

static void msg_push(const char *text, uint32_t color) {
    for (int i = MSG_LOG_SIZE - 1; i > 0; i--) msg_log[i] = msg_log[i-1];
    snprintf(msg_log[0].text, sizeof(msg_log[0].text), "%s", text);
    msg_log[0].color = color;
    msg_log[0].ttl = MSG_LOG_TTL;
}

static int damage_flash_ttl;
static SoundSystem sound_sys;
static MusicSystem music_sys;
static ItemDatabase item_db;
static ShopState shop;
static DroidUIState droid_ui;
static TerminalState terminal;
static SfxSystem sfx;
static LiberationData liberation_data;
static bool liberation_intro_active;
static bool liberation_mission_menu_active;
static bool skip_liberation_intro_requested;
static uint32_t *liberation_mission_menu_pixels;
static uint16_t liberation_mission_menu_width;
static uint16_t liberation_mission_menu_height;

enum { LIBERATION_MISSION_MENU_Y = 56 };

static CityGrid lib_buildings;
static CityGridState lib_grid;
static CityNavState lib_nav;
static Lib3dState lib_render;
static BuildingInteraction lib_interact;
static bool lib_city_generated;
static bool lib_in_building;
static LibCombatState lib_combat;
static PlotgenState lib_plot;
static bool lib_mission_briefing;
static bool lib_in_combat;
static bool lib_in_dungeon;
static int lib_dungeon_entry_x;
static int lib_dungeon_entry_y;
static int lib_inv_cursor;
static int pause_cursor;
static int droid_config_cursor;
static bool droid_config_renaming;
static int droid_config_name_pos;
static int taxi_flash_ttl;


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
    ['a'] = {0x20,0x54,0x54,0x54,0x78}, ['b'] = {0x7E,0x48,0x44,0x44,0x38},
    ['c'] = {0x38,0x44,0x44,0x44,0x20}, ['d'] = {0x38,0x44,0x44,0x48,0x7E},
    ['e'] = {0x38,0x54,0x54,0x54,0x18}, ['f'] = {0x08,0x7C,0x0A,0x02,0x00},
    ['g'] = {0x18,0xA4,0xA4,0xA4,0x7C}, ['h'] = {0x7E,0x08,0x04,0x04,0x78},
    ['i'] = {0x00,0x44,0x7D,0x40,0x00}, ['j'] = {0x40,0x80,0x84,0x7D,0x00},
    ['k'] = {0x7E,0x10,0x28,0x44,0x00}, ['l'] = {0x00,0x42,0x7E,0x40,0x00},
    ['m'] = {0x7C,0x04,0x18,0x04,0x78}, ['n'] = {0x7C,0x08,0x04,0x04,0x78},
    ['o'] = {0x38,0x44,0x44,0x44,0x38}, ['p'] = {0xFC,0x24,0x24,0x24,0x18},
    ['q'] = {0x18,0x24,0x24,0x24,0xFC}, ['r'] = {0x7C,0x08,0x04,0x04,0x08},
    ['s'] = {0x48,0x54,0x54,0x54,0x24}, ['t'] = {0x04,0x3E,0x44,0x40,0x20},
    ['u'] = {0x3C,0x40,0x40,0x20,0x7C}, ['v'] = {0x1C,0x20,0x40,0x20,0x1C},
    ['w'] = {0x3C,0x40,0x30,0x40,0x3C}, ['x'] = {0x44,0x28,0x10,0x28,0x44},
    ['y'] = {0x0C,0x50,0x50,0x50,0x3C}, ['z'] = {0x44,0x64,0x54,0x4C,0x44},
    ['0'] = {0x3C,0x52,0x4A,0x46,0x3C}, ['1'] = {0x44,0x42,0x7E,0x40,0x40},
    ['2'] = {0x64,0x52,0x52,0x52,0x4C}, ['3'] = {0x24,0x42,0x4A,0x4A,0x34},
    ['4'] = {0x1E,0x10,0x10,0x7E,0x10}, ['5'] = {0x2E,0x4A,0x4A,0x4A,0x32},
    ['6'] = {0x3C,0x4A,0x4A,0x4A,0x30}, ['7'] = {0x02,0x72,0x0A,0x06,0x02},
    ['8'] = {0x34,0x4A,0x4A,0x4A,0x34}, ['9'] = {0x0C,0x52,0x52,0x52,0x3C},
    ['!'] = {0x00,0x00,0x5E,0x00,0x00}, [' '] = {0x00,0x00,0x00,0x00,0x00},
    ['.'] = {0x00,0x40,0x00,0x00,0x00}, [':'] = {0x00,0x36,0x36,0x00,0x00},
    ['-'] = {0x08,0x08,0x08,0x08,0x08}, ['/'] = {0x60,0x10,0x08,0x04,0x03},
    [','] = {0x00,0x80,0x60,0x00,0x00}, ['?'] = {0x04,0x02,0x52,0x0A,0x04},
    ['('] = {0x00,0x3C,0x42,0x00,0x00}, [')'] = {0x00,0x42,0x3C,0x00,0x00},
    ['+'] = {0x08,0x08,0x3E,0x08,0x08}, ['%'] = {0x46,0x26,0x10,0x64,0x62},
    ['\'']= {0x00,0x00,0x06,0x00,0x00}, ['"'] = {0x00,0x06,0x00,0x06,0x00},
};

static uint32_t utf8_decode(const char **p) {
    const uint8_t *s = (const uint8_t *)*p;
    uint32_t cp;
    if (s[0] < 0x80) { cp = s[0]; *p += 1; }
    else if ((s[0] & 0xE0) == 0xC0 && (s[1] & 0xC0) == 0x80) {
        cp = ((uint32_t)(s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        *p += 2;
    } else if ((s[0] & 0xF0) == 0xE0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80) {
        cp = ((uint32_t)(s[0] & 0x0F) << 12) | ((uint32_t)(s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        *p += 3;
    } else { cp = '?'; *p += 1; }
    return cp;
}

static uint8_t unicode_to_ascii(uint32_t cp) {
    if (cp < 128) return (uint8_t)cp;
    switch (cp) {
        case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: return 'A';
        case 0xC6: return 'A';
        case 0xC7: return 'C';
        case 0xC8: case 0xC9: case 0xCA: case 0xCB: return 'E';
        case 0xCC: case 0xCD: case 0xCE: case 0xCF: return 'I';
        case 0xD0: return 'D';
        case 0xD1: return 'N';
        case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD8: return 'O';
        case 0xD9: case 0xDA: case 0xDB: case 0xDC: return 'U';
        case 0xDD: return 'Y';
        case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: return 'a';
        case 0xE6: return 'a';
        case 0xE7: return 'c';
        case 0xE8: case 0xE9: case 0xEA: case 0xEB: return 'e';
        case 0xEC: case 0xED: case 0xEE: case 0xEF: return 'i';
        case 0xF0: return 'd';
        case 0xF1: return 'n';
        case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: case 0xF8: return 'o';
        case 0xF9: case 0xFA: case 0xFB: case 0xFC: return 'u';
        case 0xFD: case 0xFF: return 'y';
        case 0x010C: case 0x010D: return 'C';
        case 0x010E: case 0x010F: return 'D';
        case 0x011A: case 0x011B: return 'E';
        case 0x0141: case 0x0142: return 'L';
        case 0x0143: case 0x0144: return 'N';
        case 0x0158: case 0x0159: return 'R';
        case 0x015A: case 0x015B: return 'S';
        case 0x0160: case 0x0161: return 'S';
        case 0x0164: case 0x0165: return 'T';
        case 0x016E: case 0x016F: return 'U';
        case 0x017D: case 0x017E: return 'Z';
        case 0x0104: case 0x0105: return 'A';
        case 0x0106: case 0x0107: return 'C';
        case 0x0118: case 0x0119: return 'E';
        case 0x0179: case 0x017A: case 0x017B: case 0x017C: return 'Z';
        default: return '?';
    }
}

static void draw_simple_text(uint32_t *fb, int pw, int ph,
                             int x, int y, const char *text, uint32_t color, int scale) {
    const char *p = text;
    int gi = 0;
    while (*p) {
        uint32_t cp = utf8_decode(&p);
        uint8_t ch = unicode_to_ascii(cp);
        if (ch >= sizeof(simple_font)/sizeof(simple_font[0])) { gi++; continue; }
        const uint8_t *glyph = simple_font[ch];
        for (int col = 0; col < 5; col++) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 7; row++) {
                if (bits & (1 << row)) {
                    for (int sy = 0; sy < scale; sy++)
                        for (int sx = 0; sx < scale; sx++) {
                            int px = x + (gi * 6 + col) * scale + sx;
                            int py = y + row * scale + sy;
                            if (px >= 0 && px < pw && py >= 0 && py < ph)
                                fb[py * pw + px] = color;
                        }
                }
            }
        }
        gi++;
    }
}

static void draw_centered(uint32_t *fb, int pw, int ph,
                          int y, const char *text, uint32_t color, int scale) {
    int len = 0;
    const char *p = text;
    while (*p) { utf8_decode(&p); len++; }
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
    if (!gs || gs->game_type != GAME_CAPTIVE) return;
    if (runtime_popup.invulnerable) {
        for (int i = 0; i < 4; ++i) gs->droids[i].hp = gs->droids[i].hp_max;
    }
    if (runtime_popup.infinite_energy) {
        for (int i = 0; i < 4; ++i) gs->droids[i].energy = gs->droids[i].energy_max;
    }
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
            runtime_popup.invulnerable = !runtime_popup.invulnerable;
            break;
        case POPUP_INFINITE_ENERGY:
            runtime_popup.infinite_energy = !runtime_popup.infinite_energy;
            break;
        case POPUP_COMPLETE_OBJECTIVE:
            if (gs->game_type == GAME_CAPTIVE && gs->generators_total > 0) {
                gs->generators_destroyed = gs->generators_total;
                game_state_complete_mission(gs);
            }
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
            case POPUP_INVULNERABLE: value = popup_toggle(runtime_popup.invulnerable); break;
            case POPUP_INFINITE_ENERGY: value = popup_toggle(runtime_popup.infinite_energy); break;
            case POPUP_COMPLETE_OBJECTIVE: value = "ACTIVATE"; break;
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
    liberation_mission_menu_active = false;

    if (!lib_city_generated) {
        uint16_t seed = (uint16_t)(gs_ptr->mission_seed & 0xFFFF);
        uint16_t seed_hi = (uint16_t)((gs_ptr->mission_seed >> 16) & 0xFFFF);
        if (!seed) seed = 0x1234;
        if (!seed_hi) seed_hi = 0x5678;
        citygen_generate(&lib_buildings, seed, (uint16_t)gs_ptr->mission);
        citygrid_init(&lib_grid, seed_hi, seed, gs_ptr->mission);
        citygrid_generate(&lib_grid);
        citygrid_map_buildings(&lib_grid, &lib_buildings);
        int start_x = lib_grid.entry_point % CITYGRID_WIDTH;
        int start_y = lib_grid.entry_point / CITYGRID_WIDTH;
        if (start_x == 0 && start_y == 0) { start_x = 32; start_y = 32; }
        city_nav_init(&lib_nav, start_x, start_y, CITY_DIR_NORTH);
        lib3d_init(&lib_render);
        {
            static const struct { uint32_t sky, ground; uint8_t wall; } city_themes[] = {
                {0xFF4466AA, 0xFF446644, 0x20}, // blue sky, green ground
                {0xFF664422, 0xFF554433, 0x40}, // desert
                {0xFF222244, 0xFF333333, 0x30}, // industrial
                {0xFF446688, 0xFF335544, 0x50}, // coastal
                {0xFF553355, 0xFF443344, 0x28}, // twilight
                {0xFF445566, 0xFF334422, 0x38}, // forest
                {0xFF666655, 0xFF555544, 0x48}, // arid
                {0xFF334455, 0xFF223322, 0x58}, // tundra
            };
            int ti = (gs_ptr->mission - 1) % 8;
            lib_render.wall_color_base = city_themes[ti].wall;
        }
        building_interact_init(&lib_interact);
        lib_in_building = false;
        lib_combat_init(&lib_combat);
        lib_in_combat = false;
        lib_city_generated = true;

        plotgen_init(&lib_plot, seed);
        plotgen_generate_buildings(&lib_plot);
        plotgen_generate_names(&lib_plot);
        lib_mission_briefing = true;
    }
}

static bool load_liberation_mission_menu(void) {
    free(liberation_mission_menu_pixels);
    liberation_mission_menu_pixels = NULL;
    liberation_mission_menu_width = 0;
    liberation_mission_menu_height = 0;

    size_t size = 0;
    uint8_t *bytes = liberation_data_read(&liberation_data,
                                           LIBERATION_RESOURCE_MISSION_MENU,
                                           &size);
    AmosSprite sprite = {0};
    if (!bytes || !amos_sprite_get(bytes, size, 0, &sprite)) {
        free(bytes);
        return false;
    }
    size_t count = (size_t)sprite.width * sprite.height;
    uint32_t *pixels = calloc(count, sizeof(*pixels));
    bool ok = pixels && amos_sprite_decode_argb(&sprite, pixels, count);
    free(bytes);
    if (!ok) {
        free(pixels);
        return false;
    }
    liberation_mission_menu_pixels = pixels;
    liberation_mission_menu_width = sprite.width;
    liberation_mission_menu_height = sprite.height;
    return true;
}

static int quicksave_slot = 0;
static CustomFeatures *custom_feat_ptr = NULL;

static void lib_transfer_purchases(GameState *gs) {
    for (int i = 0; i < lib_interact.purchased_count; i++) {
        if (gs->lib_inventory_count >= 40) break;
        snprintf(gs->lib_inventory[gs->lib_inventory_count].name,
                 sizeof(gs->lib_inventory[0].name), "%s",
                 lib_interact.purchased[i].name);
        gs->lib_inventory[gs->lib_inventory_count].item_type =
            lib_interact.purchased[i].item_type;
        gs->lib_inventory_count++;
    }
    lib_interact.purchased_count = 0;
}

static bool all_droids_dead(const GameState *gs) {
    for (int i = 0; i < 4; i++)
        if (gs->droids[i].hp > 0) return false;
    return true;
}

static void liberation_handle_input(GameState *gs, const SDL_Event *event) {
    if (event->type != SDL_EVENT_KEY_DOWN) return;

    if (lib_in_building) {
        switch (event->key.key) {
            case SDLK_ESCAPE:
                building_interact_leave(&lib_interact);
                lib_transfer_purchases(gs);
                lib_in_building = false;
                if (lib_interact.fine_paid) {
                    lib_interact.fine_paid = false;
                    gs->reputation += 15;
                    if (gs->reputation > 100) gs->reputation = 100;
                    msg_push("Fine paid. Rep +15", 0xFF44FF44);
                }
                if (lib_interact.industrial_hazard) {
                    lib_interact.industrial_hazard = false;
                    int dmg = 5 + gs->mission * 2;
                    for (int di = 0; di < 4; di++)
                        if (gs->droids[di].hp > 0) {
                            gs->droids[di].hp -= (int16_t)dmg;
                            if (gs->droids[di].hp < 0) gs->droids[di].hp = 0;
                        }
                    char hmsg[64];
                    snprintf(hmsg, sizeof(hmsg), "Industrial hazard! %d damage!", dmg);
                    msg_push(hmsg, 0xFFFF8800);
                }
                if (lib_interact.bar_fight) {
                    lib_interact.bar_fight = false;
                    lib_combat_generate_encounter(&lib_combat,
                        (uint16_t)(gs->tick * 0x5E5), gs->mission);
                    lib_in_combat = true;
                    gs->reputation -= 10;
                    if (gs->reputation < -100) gs->reputation = -100;
                    msg_push("Bar fight! Rep -10", 0xFFFF4444);
                }
                return;
            case SDLK_UP: {
                unsigned count = building_interact_choice_count(&lib_interact);
                if (count > 0) building_interact_choose(&lib_interact, 0);
                return;
            }
            case SDLK_DOWN: {
                unsigned count = building_interact_choice_count(&lib_interact);
                if (count > 1) building_interact_choose(&lib_interact, 1);
                return;
            }
            case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4:
            case SDLK_5: case SDLK_6: case SDLK_7: case SDLK_8:
            case SDLK_9: {
                unsigned idx = (unsigned)(event->key.key - SDLK_1);
                unsigned count = building_interact_choice_count(&lib_interact);
                if (idx < count) building_interact_choose(&lib_interact, idx);
                return;
            }
            case SDLK_RETURN:
            case SDLK_SPACE:
                building_interact_advance(&lib_interact);
                if (!lib_interact.active) {
                    lib_transfer_purchases(gs);
                    lib_in_building = false;
                    if (lib_interact.fine_paid) {
                        lib_interact.fine_paid = false;
                        gs->reputation += 15;
                        if (gs->reputation > 100) gs->reputation = 100;
                        msg_push("Fine paid. Rep +15", 0xFF44FF44);
                    }
                    if (lib_interact.industrial_hazard) {
                        lib_interact.industrial_hazard = false;
                        int dmg = 5 + gs->mission * 2;
                        for (int di = 0; di < 4; di++)
                            if (gs->droids[di].hp > 0) {
                                gs->droids[di].hp -= (int16_t)dmg;
                                if (gs->droids[di].hp < 0) gs->droids[di].hp = 0;
                            }
                        char hmsg[64];
                        snprintf(hmsg, sizeof(hmsg), "Industrial hazard! %d damage!", dmg);
                        msg_push(hmsg, 0xFFFF8800);
                    }
                    if (lib_interact.bar_fight) {
                        lib_interact.bar_fight = false;
                        lib_combat_generate_encounter(&lib_combat,
                            (uint16_t)(gs->tick * 0x5E5), gs->mission);
                        lib_in_combat = true;
                        gs->reputation -= 10;
                        if (gs->reputation < -100) gs->reputation = -100;
                        msg_push("Bar fight! Rep -10", 0xFFFF4444);
                    } else if (lib_interact.mission_complete) {
                        lib_interact.mission_complete = false;
                        lib_in_dungeon = true;
                        lib_dungeon_entry_x = lib_nav.cell_x;
                        lib_dungeon_entry_y = lib_nav.cell_y;
                        gs->current_level = 0;
                        map_generate_base(gs->levels, &gs->num_levels,
                                          gs->mission_seed + (uint32_t)gs->mission);
                        gs->party_x = 1; gs->party_y = 1;
                        for (int y2 = 0; y2 < MAP_HEIGHT; y2++)
                            for (int x2 = 0; x2 < MAP_WIDTH; x2++)
                                if (gs->levels[0].cells[y2][x2].type == CELL_FLOOR) {
                                    gs->party_x = x2; gs->party_y = y2;
                                    goto found_start;
                                }
                        found_start:
                        gs->party_dir = DIR_SOUTH;
                        gs->generators_total = 0;
                        gs->generators_destroyed = 0;
                        for (int f = 0; f < gs->num_levels; f++)
                            for (int y3 = 0; y3 < MAP_HEIGHT; y3++)
                                for (int x3 = 0; x3 < MAP_WIDTH; x3++)
                                    if (gs->levels[f].cells[y3][x3].type == CELL_GENERATOR)
                                        gs->generators_total++;
                        combat_init(&creatures);
                        combat_spawn_for_level(&creatures, &gs->levels[0], 0,
                                               gs->mission_seed);
                        msg_push("Entered building interior", 0xFF44AAFF);
                    }
                }
                return;
            default: return;
        }
    }

    if (lib_in_combat) {
        switch (event->key.key) {
            case SDLK_ESCAPE:
                lib_combat.active = false;
                lib_in_combat = false;
                return;
            case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4: {
                int droid_idx = (int)(event->key.key - SDLK_1);
                if (lib_combat_droid_attack(&lib_combat, gs, droid_idx)) {
                    if (lib_combat_is_over(&lib_combat, gs)) {
                        lib_in_combat = false;
                        lib_combat.active = false;
                        if (all_droids_dead(gs)) gs->mode = STATE_GAMEOVER;
                    } else {
                        lib_combat_enemy_turn(&lib_combat, gs);
                        if (lib_combat_is_over(&lib_combat, gs)) {
                            lib_in_combat = false;
                            lib_combat.active = false;
                            if (all_droids_dead(gs)) gs->mode = STATE_GAMEOVER;
                        }
                    }
                }
                return;
            }
            case SDLK_TAB:
                lib_combat.selected_target =
                    (lib_combat.selected_target + 1) % lib_combat.enemy_count;
                return;
            default: return;
        }
    }

    if (lib_nav.moving) return;

    switch (event->key.key) {
        case SDLK_W:
        case SDLK_UP:
            if (city_nav_can_move_forward(&lib_nav, &lib_grid))
                city_nav_move_forward(&lib_nav, &lib_grid);
            break;
        case SDLK_S:
        case SDLK_DOWN:
            if (city_nav_can_move_backward(&lib_nav, &lib_grid))
                city_nav_move_backward(&lib_nav, &lib_grid);
            break;
        case SDLK_A:
        case SDLK_LEFT:
            city_nav_turn_left(&lib_nav);
            break;
        case SDLK_D:
        case SDLK_RIGHT:
            city_nav_turn_right(&lib_nav);
            break;
        case SDLK_Q:
            city_nav_turn_around(&lib_nav);
            break;
        case SDLK_I:
            gs->mode = STATE_INVENTORY;
            break;
        case SDLK_1: gs->selected_droid = 0; break;
        case SDLK_2: gs->selected_droid = 1; break;
        case SDLK_3: gs->selected_droid = 2; break;
        case SDLK_4: gs->selected_droid = 3; break;
        case SDLK_M:
            gs->map_overlay = !gs->map_overlay;
            break;
        case SDLK_F:
        case SDLK_RETURN: {
            int fwd_dx = (int[]){0,1,0,-1}[lib_nav.facing];
            int fwd_dy = (int[]){-1,0,1,0}[lib_nav.facing];
            int fx = lib_nav.cell_x + fwd_dx;
            int fy = lib_nav.cell_y + fwd_dy;
            if (fx >= 0 && fx < 64 && fy >= 0 && fy < 64 &&
                city_nav_get_cell(&lib_grid, fx, fy) == 0x23 && gs->gold >= 50) {
                gs->gold -= 50;
                for (int ty = 0; ty < 64; ty++) {
                    for (int tx = 0; tx < 64; tx++) {
                        if (city_nav_get_cell(&lib_grid, tx, ty) != 0x0A) continue;
                        int off = ty * 64 + tx;
                        uint8_t bid = lib_grid.plane2[off];
                        if (bid == 0 || bid == 0xFF) continue;
                        int bg = (bid - 1) % lib_buildings.total_buildings;
                        if (bg >= 0 && bg < lib_buildings.total_buildings &&
                            lib_buildings.buildings[bg].type == 8) {
                            lib_nav.cell_x = tx;
                            lib_nav.cell_y = ty;
                            taxi_flash_ttl = 15;
                            msg_push("Taxi: 50 gold", 0xFFFFAA00);
                            goto lib_interact_done;
                        }
                    }
                }
                gs->gold += 50;
            } else if (city_nav_is_building_entrance(&lib_grid,
                    lib_nav.cell_x, lib_nav.cell_y)) {
                if (building_interact_enter(&lib_interact, &lib_grid,
                        &lib_buildings, lib_nav.cell_x, lib_nav.cell_y,
                        (uint32_t *)&gs->gold)) {
                    lib_in_building = true;
                }
            }
            lib_interact_done:
            break;
        }
        case SDLK_F5: {
            LibSaveData save;
            LibSaveDroid sd[4];
            for (int i = 0; i < 4; i++) {
                snprintf(sd[i].name, sizeof(sd[i].name), "%s", gs->droids[i].name);
                sd[i].hp = gs->droids[i].hp;
                sd[i].hp_max = gs->droids[i].hp_max;
                sd[i].energy = gs->droids[i].energy;
                sd[i].energy_max = gs->droids[i].energy_max;
                sd[i].level = 1;
                memset(sd[i].skills, 0, sizeof(sd[i].skills));
                memset(sd[i].equipment, 0, sizeof(sd[i].equipment));
            }
            uint16_t seed = (uint16_t)(gs->mission_seed & 0xFFFF);
            uint16_t seed_hi = (uint16_t)((gs->mission_seed >> 16) & 0xFFFF);
            lib_save_from_state(&save, seed_hi, seed,
                (uint16_t)gs->mission, (uint16_t)gs->mission,
                (uint32_t)gs->gold, gs->tick, &lib_nav, sd, 4);
            lib_save_write(&save, "liberation.sav");
            break;
        }
        case SDLK_F9: {
            LibSaveData save;
            if (lib_save_read(&save, "liberation.sav")) {
                gs->gold = (int)save.gold;
                gs->tick = save.tick;
                city_nav_init(&lib_nav, save.city_x, save.city_y,
                    (CityDirection)save.facing);
                for (int i = 0; i < 4 && i < save.num_droids; i++) {
                    snprintf(gs->droids[i].name, sizeof(gs->droids[i].name),
                        "%s", save.droids[i].name);
                    gs->droids[i].hp = save.droids[i].hp;
                    gs->droids[i].hp_max = save.droids[i].hp_max;
                    gs->droids[i].energy = save.droids[i].energy;
                    gs->droids[i].energy_max = save.droids[i].energy_max;
                }
            }
            break;
        }
        default: break;
    }
}

static void game_handle_input(GameState *gs, const SDL_Event *event) {
    if (event->type == SDL_EVENT_MOUSE_MOTION && custom_feat_ptr &&
        custom_feat_ptr->mouse_look && gs->mode == STATE_GAME) {
        float dx = event->motion.xrel * custom_feat_ptr->mouse_sensitivity;
        if (dx > 5.0f) gs->party_dir = (gs->party_dir + 1) % 4;
        else if (dx < -5.0f) gs->party_dir = (gs->party_dir + 3) % 4;
        return;
    }
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
            if (combat_droid_attack(gs, &creatures, gs->selected_droid)) {
                sfx_play(&sfx, SFX_SHOOT);
                if (creatures.creature_killed) {
                    sfx_play(&sfx, SFX_DEATH);
                    creatures.creature_killed = false;
                }
                if (creatures.level_up_occurred) {
                    sfx_play(&sfx, SFX_LEVEL_UP);
                    msg_push("LEVEL UP!", 0xFFFFFF00);
                    creatures.level_up_occurred = false;
                }
                char atk_msg[64];
                snprintf(atk_msg, sizeof(atk_msg), "Droid %d fires!",
                         gs->selected_droid + 1);
                msg_push(atk_msg, 0xFF44FF44);
                if (all_droids_dead(gs)) gs->mode = STATE_GAMEOVER;
            }
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
                int gen_before = gs->generators_destroyed;
                CellType cell_before = CELL_WALL;
                if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT)
                    cell_before = gs->levels[gs->current_level].cells[ty][tx].type;
                combat_interact(gs, &item_db);
                if (gs->generators_destroyed > gen_before)
                    sfx_play(&sfx, SFX_GENERATOR);
                if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT) {
                    CellType cell_after = gs->levels[gs->current_level].cells[ty][tx].type;
                    if (cell_before == CELL_DOOR_LOCKED && cell_after == CELL_FLOOR)
                        sfx_play(&sfx, SFX_DOOR_OPEN);
                    else if (cell_before == CELL_DOOR && cell_after == CELL_FLOOR)
                        sfx_play(&sfx, SFX_DOOR_OPEN);
                }
            }
            return;
        }
        case SDLK_F5:
            if (custom_feat_ptr && custom_feat_ptr->quicksave) {
                char path[64];
                snprintf(path, sizeof(path), "opencaptive_slot%d.sav", quicksave_slot);
                save_game(gs, &creatures, &puzzles, path);
            } else {
                save_game(gs, &creatures, &puzzles, "opencaptive.sav");
            }
            return;
        case SDLK_F6:
            if (custom_feat_ptr && custom_feat_ptr->quicksave) {
                quicksave_slot = (quicksave_slot + 1) % 10;
            }
            return;
        case SDLK_F9:
            if (custom_feat_ptr && custom_feat_ptr->quicksave) {
                char path[64];
                snprintf(path, sizeof(path), "opencaptive_slot%d.sav", quicksave_slot);
                load_game(gs, &creatures, &puzzles, path);
            } else {
                load_game(gs, &creatures, &puzzles, "opencaptive.sav");
            }
            return;
        case SDLK_F7:
            if (custom_feat_ptr) {
                custom_feat_ptr->debug_hud = !custom_feat_ptr->debug_hud;
            }
            return;
        case SDLK_F8:
            if (custom_feat_ptr) {
                custom_feat_ptr->minimap = !custom_feat_ptr->minimap;
            }
            return;
        case SDLK_KP_PLUS:
            if (custom_feat_ptr && custom_feat_ptr->speed_control) {
                custom_feat_ptr->game_speed *= 1.5f;
                if (custom_feat_ptr->game_speed > 4.0f)
                    custom_feat_ptr->game_speed = 4.0f;
            }
            return;
        case SDLK_KP_MINUS:
            if (custom_feat_ptr && custom_feat_ptr->speed_control) {
                custom_feat_ptr->game_speed /= 1.5f;
                if (custom_feat_ptr->game_speed < 0.25f)
                    custom_feat_ptr->game_speed = 0.25f;
            }
            return;
        case SDLK_PERIOD: // > stairs down
            game_state_change_floor(gs, 1);
            return;
        case SDLK_COMMA: // < stairs up
            game_state_change_floor(gs, -1);
            return;
        case SDLK_H:
            gs->mode = STATE_HELP;
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
            for (int di = 0; di < 4; di++) {
                if (gs->droids[di].hp > 0 && gs->droids[di].energy > 0)
                    gs->droids[di].energy--;
            }
            puzzle_check_step(&puzzles, gs, nx, ny);
            {
                MapCell *step_cell = &gs->levels[gs->current_level].cells[ny][nx];
                if (step_cell->item_id > 0) {
                    Droid *d = &gs->droids[gs->selected_droid];
                    for (int si = 0; si < 10; si++) {
                        if (d->items[si] == 0) {
                            d->items[si] = step_cell->item_id;
                            char pickup_msg[64];
                            snprintf(pickup_msg, sizeof(pickup_msg),
                                     "Droid %d picked up item", gs->selected_droid + 1);
                            msg_push(pickup_msg, 0xFF44AAFF);
                            sfx_play(&sfx, SFX_PICKUP);
                            step_cell->item_id = 0;
                            break;
                        }
                    }
                }
            }
            if (cell == CELL_PIT) {
                for (int di = 0; di < 4; di++) {
                    if (gs->droids[di].hp > 0) {
                        int dmg = 5 + (gs->current_level * 2);
                        gs->droids[di].hp -= (int16_t)dmg;
                        if (gs->droids[di].hp < 0) gs->droids[di].hp = 0;
                    }
                }
                msg_push("Fell into a pit!", 0xFFFF4444);
                sfx_play(&sfx, SFX_HIT);
                if (all_droids_dead(gs)) gs->mode = STATE_GAMEOVER;
            } else if (cell == CELL_PRESSURE_PLATE) {
                msg_push("Click!", 0xFFAAAA44);
                sfx_play(&sfx, SFX_DOOR_OPEN);
            }
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
        .integer_scaling = false,
        .brightness = 50,
        .contrast = 50,
        .fps_limit = 60,
    };
    CustomFeatures custom;
    custom_features_defaults(&custom);
    Automap automap_state;
    automap_init(&automap_state);
    ReplaySystem replay;
    replay_init(&replay);

    GameType requested_game = GAME_CAPTIVE;
    bool start_directly = false;
    bool show_liberation_mission_menu_requested = false;
    const char *lang_override = NULL;
    const char *verify_data = NULL;
    const char *capture_frame_path = NULL;
    const char *dos_vga_dump_path = NULL;
    const char *dos_vga_output_path = NULL;
    const char *expected_frame_path = NULL;
    const char *actual_frame_path = NULL;
    int compare_rect[4] = {0};
    bool compare_rect_set = false;
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
                "  --resolution <WxH>    Window resolution (e.g. 1920x1080)\n"
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
                "  --game <name>         Start game directly: captive, liberation\n"
                "  --lang <code>         Language: en, sv, de, fr, es, it, etc.\n\n"
                "Custom features:\n"
                "  --all-features        Enable all custom features\n"
                "  --hd-upscale          HD texture upscaling (xBRZ)\n"
                "  --upscale-factor <n>  Upscale factor: 2, 3, or 4 (default 2)\n"
                "  --widescreen          Widescreen viewport\n"
                "  --quicksave           Multi-slot quicksave (F5/F6/F9)\n"
                "  --minimap             Persistent minimap overlay (F8 toggle)\n"
                "  --mouse-look          FPS-style mouse control\n"
                "  --debug-hud           Debug overlay (F7 toggle)\n"
                "  --reverb              Audio reverb effect\n"
                "  --automap             Remember visited cells\n"
                "  --dynamic-lighting    Distance-based lighting\n"
                "  --speed <n>           Game speed multiplier (default 1.0)\n"
                "  --fast-travel         Enable fast travel in cities\n"
                "  --replay-record       Record inputs for replay\n"
                "  --replay-play <file>  Play back a recorded replay\n"
                "  --cross-save-export   Enable portable save export\n"
                "  --features-config <f> Load features from config file\n\n"
                "  --verify-data <name>  Verify data by SHA-256: captive, liberation, all\n\n"
                "  --capture-frame <ppm> Save one unscaled native game frame, then exit\n\n"
                "  --extract-dos-vga <dump> <ppm>  Extract a 320x200 DOS VGA reference frame\n\n"
                "  --compare-frames <expected> <actual>  Compare two native PPM frames\n"
                "  --compare-frames-rect <expected> <actual> <x> <y> <w> <h>\n"
                "                         Compare one native frame rectangle exactly\n\n"
                "  --skip-intro          Skip Liberation's original intro (automation only)\n\n"
                "  --show-liberation-mission-menu\n"
                "                         Show the verified original Liberation menu (automation only)\n\n"
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
        } else if (strcmp(argv[i], "--resolution") == 0 && i + 1 < argc) {
            int rw = 0, rh = 0;
            if (sscanf(argv[++i], "%dx%d", &rw, &rh) == 2 && rw >= 320 && rh >= 200) {
                config.window_width = rw;
                config.window_height = rh;
            } else {
                fprintf(stderr, "--resolution must be WxH (e.g. 1920x1080, min 320x200)\n");
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
        } else if (strcmp(argv[i], "--hd-upscale") == 0) {
            custom.hd_upscale = true;
        } else if (strcmp(argv[i], "--upscale-factor") == 0 && i + 1 < argc) {
            custom.upscale_factor = atoi(argv[++i]);
            if (custom.upscale_factor < 2) custom.upscale_factor = 2;
            if (custom.upscale_factor > 4) custom.upscale_factor = 4;
            custom.hd_upscale = true;
        } else if (strcmp(argv[i], "--widescreen") == 0) {
            custom.widescreen = true;
        } else if (strcmp(argv[i], "--quicksave") == 0) {
            custom.quicksave = true;
        } else if (strcmp(argv[i], "--minimap") == 0) {
            custom.minimap = true;
        } else if (strcmp(argv[i], "--mouse-look") == 0) {
            custom.mouse_look = true;
        } else if (strcmp(argv[i], "--debug-hud") == 0) {
            custom.debug_hud = true;
        } else if (strcmp(argv[i], "--reverb") == 0) {
            custom.audio_reverb = true;
        } else if (strcmp(argv[i], "--automap") == 0) {
            custom.automap = true;
        } else if (strcmp(argv[i], "--dynamic-lighting") == 0) {
            custom.dynamic_lighting = true;
        } else if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc) {
            custom.game_speed = (float)atof(argv[++i]);
            custom.speed_control = true;
        } else if (strcmp(argv[i], "--fast-travel") == 0) {
            custom.fast_travel = true;
            custom.speed_control = true;
        } else if (strcmp(argv[i], "--replay-record") == 0) {
            custom.replay_record = true;
            replay.recording = true;
        } else if (strcmp(argv[i], "--replay-play") == 0 && i + 1 < argc) {
            if (replay_load(&replay, argv[++i])) {
                custom.replay_playback = true;
            } else {
                fprintf(stderr, "Failed to load replay: %s\n", argv[i]);
                return 2;
            }
        } else if (strcmp(argv[i], "--cross-save-export") == 0) {
            custom.cross_save = true;
        } else if (strcmp(argv[i], "--features-config") == 0 && i + 1 < argc) {
            custom_features_load(&custom, argv[++i]);
        } else if (strcmp(argv[i], "--all-features") == 0) {
            custom.hd_upscale = true;
            custom.quicksave = true;
            custom.minimap = true;
            custom.debug_hud = true;
            custom.audio_reverb = true;
            custom.automap = true;
            custom.dynamic_lighting = true;
            custom.speed_control = true;
            custom.texture_filter = true;
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
        } else if (strcmp(argv[i], "--extract-dos-vga") == 0 && i + 2 < argc) {
            dos_vga_dump_path = argv[++i];
            dos_vga_output_path = argv[++i];
        } else if (strcmp(argv[i], "--compare-frames") == 0 && i + 2 < argc) {
            expected_frame_path = argv[++i];
            actual_frame_path = argv[++i];
        } else if (strcmp(argv[i], "--compare-frames-rect") == 0 && i + 6 < argc) {
            expected_frame_path = argv[++i];
            actual_frame_path = argv[++i];
            for (int field = 0; field < 4; ++field) {
                if (!parse_int_option(argv[++i], field < 2 ? 0 : 1, INT_MAX,
                                      &compare_rect[field])) {
                    fprintf(stderr, "Comparison rectangle must use integer x y width height\n");
                    return 2;
                }
            }
            compare_rect_set = true;
        } else if (strcmp(argv[i], "--lang") == 0 && i + 1 < argc) {
            lang_override = argv[++i];
        } else if (strcmp(argv[i], "--skip-intro") == 0) {
            skip_liberation_intro_requested = true;
        } else if (strcmp(argv[i], "--show-liberation-mission-menu") == 0) {
            requested_game = GAME_LIBERATION;
            start_directly = true;
            show_liberation_mission_menu_requested = true;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 2;
        }
    }

    if (dos_vga_dump_path)
        return write_dos_vga_reference(dos_vga_dump_path, dos_vga_output_path) ? 0 : 1;
    if (expected_frame_path)
        return compare_ppm_frames(expected_frame_path, actual_frame_path,
                                  compare_rect_set ? compare_rect : NULL);

    if (verify_data) {
        DataVFS verify_vfs;
        LiberationData verify_liberation = {0};
        bool vfs_ok = vfs_init(&verify_vfs, config.data_path);
        bool ok = vfs_ok;
        bool check_captive = strcmp(verify_data, "liberation") != 0;
        bool check_liberation = strcmp(verify_data, "captive") != 0;
        if (check_captive) {
            bool dos_valid = vfs_ok && validate_data_path(&verify_vfs);
            bool amiga_valid = vfs_ok && captive_amiga_data_verify(&verify_vfs);
            printf("Captive source media: DOS=%s Amiga=%s\n",
                   dos_valid ? "verified" : "not verified",
                   amiga_valid ? "verified/RNC" : "not verified");
            ok = ok && (dos_valid || amiga_valid);
        }
        if (check_liberation) {
            bool valid = vfs_ok && liberation_data_open(&verify_liberation, &verify_vfs);
            printf("Liberation data: %s\n", valid ? "verified" : "not verified");
            if (valid) {
                bool intro_pixels = liberation_frame_matches_ppm_sha256(
                    &verify_liberation.intro_frame, 320, 162,
                    "c65df735ccd785dee5cbe118c3f51153f270f6260411d3e04c6ca278b2d6fab3");
                bool city_pixels = liberation_frame_matches_ppm_sha256(
                    &verify_liberation.city_frame, 320, 167,
                    "b7c326d1cdd36bb3574b33add3d68cff9739e7a5e339d800f44af3c79f510bb1");
                printf("Liberation presentation: intro=%s/%s city=%s/%s\n",
                       verify_liberation.intro_frame.bitplanes ? "decoded" : "unavailable",
                       verify_liberation.intro_script.bytes ? "SCPT" : "no-SCPT",
                       verify_liberation.city_frame.bitplanes ? "decoded" : "unavailable",
                       verify_liberation.city_script.bytes ? "SCPT" : "no-SCPT");
                printf("Liberation first-frame pixels: intro=%s city=%s\n",
                       intro_pixels ? "verified" : "not verified",
                       city_pixels ? "verified" : "not verified");
                valid = valid && intro_pixels && city_pixels;
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

    i18n_init(lang_override);

    if (config.window_width <= 0 || config.window_height <= 0) {
        config.window_width = CAPTIVE_ORIGINAL_WIDTH * config.scale_factor;
        config.window_height = CAPTIVE_ORIGINAL_HEIGHT * config.scale_factor;
        if (config.window_width < 640) config.window_width = 640;
        if (config.window_height < 400) config.window_height = 400;
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
            gs.mode = STATE_DROID_CONFIG;
            droid_config_cursor = 0;
            printf("Starting verified Captive presentation\n");
        }
    } else if (start_directly && requested_game == GAME_LIBERATION) {
        if (!liberation_data_open(&liberation_data, &vfs)) {
            show_missing_liberation_data_dialog(config.data_path);
        } else {
            load_liberation_mission_menu();
            start_liberation_session(&gs);
            if (show_liberation_mission_menu_requested) {
                liberation_intro_active = false;
                liberation_mission_menu_active = liberation_mission_menu_pixels != NULL;
            }
            printf("Starting verified Liberation presentation\n");
        }
    }

    custom_feat_ptr = &custom;
    if (custom.mouse_look)
        SDL_SetWindowRelativeMouseMode(renderer.window, true);

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
                        int width = MENU_WIDTH;
                        int height = MENU_HEIGHT;
                        SDL_GetWindowSize(renderer.window, &width, &height);
                        float x = event.button.x * MENU_WIDTH / width;
                        float y = event.button.y * MENU_HEIGHT / height;
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
                                gs.mode = STATE_DROID_CONFIG;
                                droid_config_cursor = 0;
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
                            load_liberation_mission_menu();
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
                        gs.mode = STATE_DROID_CONFIG;
                        droid_config_cursor = 0;
                    }
                    break;
                case STATE_DROID_CONFIG:
                    if (event.type == SDL_EVENT_KEY_DOWN) {
                        if (droid_config_renaming) {
                            SDL_Keycode k = event.key.key;
                            Droid *rd = &gs.droids[droid_config_cursor];
                            if (k == SDLK_RETURN || k == SDLK_ESCAPE) {
                                droid_config_renaming = false;
                            } else if (k == SDLK_BACKSPACE && droid_config_name_pos > 0) {
                                rd->name[--droid_config_name_pos] = '\0';
                            } else if (droid_config_name_pos < 14) {
                                char ch = 0;
                                if (k >= SDLK_A && k <= SDLK_Z)
                                    ch = (char)('A' + (k - SDLK_A));
                                else if (k >= SDLK_0 && k <= SDLK_9)
                                    ch = (char)('0' + (k - SDLK_0));
                                else if (k == SDLK_MINUS) ch = '-';
                                else if (k == SDLK_SPACE) ch = ' ';
                                if (ch) {
                                    rd->name[droid_config_name_pos++] = ch;
                                    rd->name[droid_config_name_pos] = '\0';
                                }
                            }
                        } else {
                            if (event.key.key == SDLK_UP && droid_config_cursor > 0)
                                droid_config_cursor--;
                            else if (event.key.key == SDLK_DOWN && droid_config_cursor < 3)
                                droid_config_cursor++;
                            else if (event.key.key == SDLK_R) {
                                droid_config_renaming = true;
                                droid_config_name_pos = (int)strlen(gs.droids[droid_config_cursor].name);
                            } else if (event.key.key == SDLK_S) {
                                // Swap weapons between selected droid and next
                                int next = (droid_config_cursor + 1) % 4;
                                uint8_t tmp[2];
                                tmp[0] = gs.droids[droid_config_cursor].weapons[0];
                                tmp[1] = gs.droids[droid_config_cursor].weapons[1];
                                gs.droids[droid_config_cursor].weapons[0] = gs.droids[next].weapons[0];
                                gs.droids[droid_config_cursor].weapons[1] = gs.droids[next].weapons[1];
                                gs.droids[next].weapons[0] = tmp[0];
                                gs.droids[next].weapons[1] = tmp[1];
                                uint16_t td = gs.droids[droid_config_cursor].weapon_damage;
                                gs.droids[droid_config_cursor].weapon_damage = gs.droids[next].weapon_damage;
                                gs.droids[next].weapon_damage = td;
                            } else if (event.key.key == SDLK_RETURN)
                                gs.mode = STATE_GAME;
                        }
                    }
                    break;
                case STATE_GAME:
                    if (event.type == SDL_EVENT_KEY_DOWN &&
                        event.key.key == SDLK_ESCAPE) {
                        if (gs.game_type == GAME_LIBERATION && liberation_intro_active) {
                            liberation_intro_active = false;
                            liberation_mission_menu_active =
                                liberation_mission_menu_pixels != NULL;
                        } else {
                            gs.mode = STATE_PAUSE;
                            gs.paused = true;
                        }
                    } else if (gs.game_type == GAME_LIBERATION) {
                        if (liberation_intro_active && event.type == SDL_EVENT_KEY_DOWN) {
                            liberation_intro_active = false;
                            liberation_mission_menu_active =
                                liberation_mission_menu_pixels != NULL;
                        } else if (liberation_mission_menu_active &&
                                   event.type == SDL_EVENT_KEY_DOWN &&
                                   (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)) {
                            liberation_mission_menu_active = false;
                        } else if (liberation_mission_menu_active &&
                                   event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                                   event.button.button == SDL_BUTTON_LEFT) {
                            int ww = LIBERATION_SCREEN_WIDTH, wh = LIBERATION_SCREEN_HEIGHT;
                            SDL_GetWindowSize(renderer.window, &ww, &wh);
                            int x = (int)(event.button.x * LIBERATION_SCREEN_WIDTH / ww);
                            int y = (int)(event.button.y * LIBERATION_SCREEN_HEIGHT / wh);
                            int local_y = y - LIBERATION_MISSION_MENU_Y;
                            if (x >= 89 && x < 233 &&
                                local_y >= 89 && local_y < liberation_mission_menu_height)
                                liberation_mission_menu_active = false;
                        } else if (lib_mission_briefing &&
                                   event.type == SDL_EVENT_KEY_DOWN &&
                                   event.key.key == SDLK_RETURN) {
                            lib_mission_briefing = false;
                        } else if (!liberation_intro_active && !liberation_mission_menu_active &&
                                   !lib_mission_briefing && lib_city_generated) {
                            if (lib_in_dungeon) {
                                if (event.type == SDL_EVENT_KEY_DOWN &&
                                    event.key.key == SDLK_ESCAPE) {
                                    lib_in_dungeon = false;
                                    msg_push("Left building", 0xFF44AAFF);
                                } else {
                                    game_handle_input(&gs, &event);
                                    if (gs.generators_destroyed >= gs.generators_total &&
                                        gs.generators_total > 0) {
                                        lib_in_dungeon = false;
                                        gs.mission++;
                                        if (gs.mission >= 256) {
                                            gs.mode = STATE_VICTORY;
                                        } else {
                                            gs.mission_seed = gs.mission_seed * 0x5E5 + gs.mission;
                                            lib_city_generated = false;
                                            start_liberation_session(&gs);
                                        }
                                    }
                                }
                            } else {
                                liberation_handle_input(&gs, &event);
                            }
                        }
                    } else {
                        /* Keep the input/state loop live while the final
                         * source-panel compositor is being recovered.  The
                         * controls operate on the same game state consumed by
                         * the 19-cell view window; disabling them made
                         * Captive appear frozen even where its original-data
                         * shell and verified map path were loaded. */
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
                        if (gs.game_type == GAME_LIBERATION) {
                            switch (event.key.key) {
                                case SDLK_ESCAPE:
                                    gs.mode = STATE_GAME;
                                    break;
                                case SDLK_UP:
                                    if (lib_inv_cursor > 0) lib_inv_cursor--;
                                    break;
                                case SDLK_DOWN:
                                    if (lib_inv_cursor < gs.lib_inventory_count - 1)
                                        lib_inv_cursor++;
                                    break;
                                case SDLK_1: gs.selected_droid = 0; break;
                                case SDLK_2: gs.selected_droid = 1; break;
                                case SDLK_3: gs.selected_droid = 2; break;
                                case SDLK_4: gs.selected_droid = 3; break;
                                case SDLK_RETURN: {
                                    if (lib_inv_cursor >= 0 &&
                                        lib_inv_cursor < gs.lib_inventory_count) {
                                        Droid *d = &gs.droids[gs.selected_droid];
                                        for (int si = 0; si < 10; si++) {
                                            if (d->items[si] == 0) {
                                                d->items[si] = (uint8_t)gs.lib_inventory[lib_inv_cursor].item_type;
                                                for (int j = lib_inv_cursor; j < gs.lib_inventory_count - 1; j++)
                                                    gs.lib_inventory[j] = gs.lib_inventory[j + 1];
                                                gs.lib_inventory_count--;
                                                if (lib_inv_cursor >= gs.lib_inventory_count && lib_inv_cursor > 0)
                                                    lib_inv_cursor--;
                                                break;
                                            }
                                        }
                                    }
                                    break;
                                }
                                case SDLK_U: {
                                    Droid *d = &gs.droids[gs.selected_droid];
                                    for (int si = 0; si < 10; si++) {
                                        if (d->items[si] != 0 && gs.lib_inventory_count < 40) {
                                            const Item *it = item_db_get(&item_db, d->items[si]);
                                            snprintf(gs.lib_inventory[gs.lib_inventory_count].name,
                                                     sizeof(gs.lib_inventory[0].name), "%s",
                                                     it ? it->name : "ITEM");
                                            gs.lib_inventory[gs.lib_inventory_count].item_type = d->items[si];
                                            gs.lib_inventory_count++;
                                            d->items[si] = 0;
                                            break;
                                        }
                                    }
                                    break;
                                }
                                case SDLK_E: {
                                    Droid *d = &gs.droids[gs.selected_droid];
                                    for (int si = 0; si < 10; si++) {
                                        if (d->items[si] != 0) {
                                            const Item *it = item_db_get(&item_db, d->items[si]);
                                            if (it && (it->category >= ITEM_WEAPON_MELEE &&
                                                       it->category <= ITEM_WEAPON_SPRAY)) {
                                                for (int w = 0; w < 2; w++) {
                                                    if (d->weapons[w] == 0) {
                                                        d->weapons[w] = d->items[si];
                                                        d->items[si] = 0;
                                                        droid_recalc_weapon_damage(d, &item_db);
                                                        goto lib_equip_done;
                                                    }
                                                }
                                                uint8_t old = d->weapons[0];
                                                d->weapons[0] = d->items[si];
                                                d->items[si] = old;
                                                droid_recalc_weapon_damage(d, &item_db);
                                                goto lib_equip_done;
                                            }
                                        }
                                    }
                                    lib_equip_done:
                                    break;
                                }
                                default: break;
                            }
                        } else {
                            switch (event.key.key) {
                                case SDLK_ESCAPE:
                                    gs.mode = STATE_GAME;
                                    break;
                                case SDLK_UP:
                                    droid_ui_handle_key(&droid_ui, &gs, &item_db, 0x50);
                                    break;
                                case SDLK_DOWN:
                                    droid_ui_handle_key(&droid_ui, &gs, &item_db, 0x51);
                                    break;
                                case SDLK_TAB:
                                    droid_ui_handle_key(&droid_ui, &gs, &item_db, 0x09);
                                    break;
                                case SDLK_RETURN:
                                    droid_ui_handle_key(&droid_ui, &gs, &item_db, 0x0D);
                                    break;
                                default: break;
                            }
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
                            case SDLK_R:
                                shop_repair(&shop, &gs, gs.selected_droid);
                                break;
                            default: break;
                        }
                    }
                    break;
                case STATE_HOLAMAP:
                    if (event.type == SDL_EVENT_KEY_DOWN) {
                        switch (event.key.key) {
                            case SDLK_RETURN:
                                game_state_new_mission(&gs, gs.mission + 1);
                                gs.mode = STATE_GAME;
                                music_play(&music_sys, MUSIC_BASE);
                                break;
                            case SDLK_S:
                                gs.mode = STATE_SHOP;
                                shop_init(&shop, &item_db, gs.mission, gs.mission_seed);
                                shop.gold = gs.gold;
                                break;
                            case SDLK_ESCAPE:
                                gs.mode = STATE_MENU;
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
                case STATE_HELP:
                    if (event.type == SDL_EVENT_KEY_DOWN) {
                        gs.mode = STATE_GAME;
                    }
                    break;
                case STATE_PAUSE:
                    if (event.type == SDL_EVENT_KEY_DOWN) {
                        switch (event.key.key) {
                            case SDLK_ESCAPE:
                                gs.mode = STATE_GAME;
                                gs.paused = false;
                                break;
                            case SDLK_UP:
                                if (pause_cursor > 0) pause_cursor--;
                                break;
                            case SDLK_DOWN:
                                if (pause_cursor < 2) pause_cursor++;
                                break;
                            case SDLK_RETURN:
                            case SDLK_RETURN2:
                                if (pause_cursor == 0) {
                                    gs.mode = STATE_GAME;
                                    gs.paused = false;
                                } else if (pause_cursor == 1) {
                                    gs.mode = STATE_MENU;
                                    sync_menu_from_config(&menu, &config,
                                                          music_sys.enabled,
                                                          sound_sys.enabled);
                                    music_stop(&music_sys);
                                    gs.paused = false;
                                } else if (pause_cursor == 2) {
                                    gs.mode = STATE_MENU;
                                    start_menu_init(&menu);
                                    music_stop(&music_sys);
                                    gs.paused = false;
                                }
                                break;
                            default: break;
                        }
                    }
                    break;
                default: break;
            }
        }

        /* Liberation is a PAL CD32 presentation and therefore uses a taller
         * canvas than Captive's 320x200 shell.  Switch at the game boundary,
         * not by stretching one game's framebuffer into the other. */
        int frame_width, frame_height;
        if (gs.mode == STATE_MENU) {
            frame_width = MENU_WIDTH;
            frame_height = MENU_HEIGHT;
        } else if (gs.mode == STATE_GAME && gs.game_type == GAME_LIBERATION && !lib_in_dungeon) {
            frame_width = LIBERATION_SCREEN_WIDTH;
            frame_height = LIBERATION_SCREEN_HEIGHT;
        } else {
            frame_width = CAPTIVE_ORIGINAL_WIDTH;
            frame_height = CAPTIVE_ORIGINAL_HEIGHT;
        }
        if (renderer.canvas_width != frame_width || renderer.canvas_height != frame_height)
            renderer_set_canvas(&renderer, frame_width, frame_height, &config);

        memset(framebuffer, 0, (size_t)frame_width * frame_height * sizeof(uint32_t));

        switch (gs.mode) {
            case STATE_MENU:
                start_menu_render(&menu, framebuffer, MENU_WIDTH, MENU_HEIGHT);
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
                if (gs.game_type == GAME_LIBERATION && !lib_in_dungeon) {
                    memset(framebuffer, 0, sizeof(framebuffer));
                    if (liberation_intro_active) {
                        if (liberation_data.intro_frame.bitplanes) {
                            liberation_anim_blit(&liberation_data.intro_frame,
                                framebuffer, LIBERATION_SCREEN_WIDTH,
                                LIBERATION_SCREEN_HEIGHT, 0, 47);
                        }
                    } else if (liberation_mission_menu_active && liberation_mission_menu_pixels) {
                        for (uint16_t y = 0; y < liberation_mission_menu_height; ++y) {
                            if (y + LIBERATION_MISSION_MENU_Y >= LIBERATION_SCREEN_HEIGHT) break;
                            memcpy(framebuffer + (size_t)(y + LIBERATION_MISSION_MENU_Y) *
                                   LIBERATION_SCREEN_WIDTH,
                                   liberation_mission_menu_pixels +
                                   (size_t)y * liberation_mission_menu_width,
                                   (size_t)liberation_mission_menu_width * sizeof(*framebuffer));
                        }
                    } else if (lib_mission_briefing && lib_city_generated) {
                        draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                      LIBERATION_SCREEN_HEIGHT, 30,
                                      "MISSION BRIEFING", 0xFF44FF44, 2);
                        char line[128];
                        snprintf(line, sizeof(line), "City: %s", lib_plot.city_name);
                        draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                      LIBERATION_SCREEN_HEIGHT, 70, line, 0xFFFFFF44, 1);
                        snprintf(line, sizeof(line), "Find: %s (%s)",
                                 lib_plot.victim_name, lib_plot.victim_title);
                        draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                      LIBERATION_SCREEN_HEIGHT, 90, line, 0xFFFFAAAA, 1);
                        snprintf(line, sizeof(line), "Source: %s", lib_plot.news_source);
                        draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                      LIBERATION_SCREEN_HEIGHT, 110, line, 0xFF8888CC, 1);
                        snprintf(line, sizeof(line), "%d buildings in city",
                                 lib_plot.num_buildings);
                        draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                      LIBERATION_SCREEN_HEIGHT, 140, line, 0xFFAAAAAA, 1);
                        draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                      LIBERATION_SCREEN_HEIGHT, 180,
                                      "PRESS ENTER TO BEGIN", 0xFF888888, 1);
                    } else if (lib_city_generated) {
                        uint32_t now = SDL_GetTicks();
                        float dt = (now - gs.last_frame_ms) / 1000.0f;
                        if (dt > 0.1f) dt = 0.1f;
                        gs.last_frame_ms = now;
                        bool was_moving = lib_nav.moving;
                        city_nav_update(&lib_nav, dt);
                        if (was_moving && !lib_nav.moving && !lib_in_combat && !lib_in_building) {
                            uint16_t encounter_roll = (uint16_t)(gs.tick * 0x5E5 + lib_nav.cell_x * 31 + lib_nav.cell_y * 17);
                            if ((encounter_roll & 0x1F) == 0) {
                                lib_combat_generate_encounter(&lib_combat, encounter_roll, gs.mission);
                                lib_in_combat = true;
                            }
                        }
                        {
                            int hour = (int)((gs.tick / 60) % 24);
                            if (hour >= 6 && hour < 18) {
                                lib_render.sky_color = 0xFF4466AA;
                                lib_render.ground_color = 0xFF446644;
                            } else if (hour >= 18 && hour < 21) {
                                lib_render.sky_color = 0xFF332255;
                                lib_render.ground_color = 0xFF333322;
                            } else {
                                lib_render.sky_color = 0xFF111133;
                                lib_render.ground_color = 0xFF222211;
                            }
                        }
                        city_nav_render(&lib_nav, &lib_grid, &lib_render,
                                        NULL, NULL, 0);
                        for (int dy = 0; dy < LIBERATION_SCREEN_HEIGHT - 40; dy++) {
                            int sy = dy * LIB3D_VP_HEIGHT / (LIBERATION_SCREEN_HEIGHT - 40);
                            for (int dx = 0; dx < LIBERATION_SCREEN_WIDTH; dx++) {
                                int sx = dx * LIB3D_VP_WIDTH / LIBERATION_SCREEN_WIDTH;
                                framebuffer[dy * LIBERATION_SCREEN_WIDTH + dx] =
                                    lib_render.framebuffer[sy * LIB3D_VP_WIDTH + sx];
                            }
                        }
                        if (taxi_flash_ttl > 0) {
                            taxi_flash_ttl--;
                            uint32_t flash = 0xFF000000 | ((taxi_flash_ttl * 17) << 8);
                            for (int i = 0; i < LIBERATION_SCREEN_WIDTH * (LIBERATION_SCREEN_HEIGHT - 40); i++)
                                framebuffer[i] = flash;
                            draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                          LIBERATION_SCREEN_HEIGHT, 80,
                                          "TAXI", 0xFFFFFF00, 3);
                        }
                        char pos_str[64];
                        snprintf(pos_str, sizeof(pos_str), "%s (%d,%d) %s",
                            lib_plot.city_name[0] ? lib_plot.city_name : lib_buildings.city_name,
                            lib_nav.cell_x, lib_nav.cell_y,
                            city_nav_is_building_entrance(&lib_grid,
                                lib_nav.cell_x, lib_nav.cell_y) ? "[ENTER]" : "");
                        draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                            LIBERATION_SCREEN_HEIGHT, 2, LIBERATION_SCREEN_HEIGHT - 10,
                            pos_str, 0xFFFFFFFF, 1);
                        if (lib_in_combat) {
                            for (int y = 0; y < LIBERATION_SCREEN_HEIGHT; y++)
                                for (int x = 0; x < LIBERATION_SCREEN_WIDTH; x++)
                                    framebuffer[y * LIBERATION_SCREEN_WIDTH + x] =
                                        (framebuffer[y * LIBERATION_SCREEN_WIDTH + x] >> 1) & 0x7F7F7F;
                            draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                LIBERATION_SCREEN_HEIGHT, 8, 8, "COMBAT", 0xFFFF0000, 1);
                            for (int ei = 0; ei < lib_combat.enemy_count; ei++) {
                                LibCombatEnemy *e = &lib_combat.enemies[ei];
                                char line[80];
                                snprintf(line, sizeof(line), "%s%s HP:%d/%d DMG:%d",
                                    ei == lib_combat.selected_target ? ">" : " ",
                                    e->name, e->hp, e->hp_max, e->damage);
                                uint32_t color = e->alive ? 0xFFFFFFFF : 0xFF666666;
                                draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                    LIBERATION_SCREEN_HEIGHT, 8, 22 + ei * 10, line, color, 1);
                            }
                            draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                LIBERATION_SCREEN_HEIGHT, 8, LIBERATION_SCREEN_HEIGHT - 30,
                                "1-4:Attack TAB:Target ESC:Flee", 0xFFCCCCCC, 1);
                            if (lib_combat_is_over(&lib_combat, &gs)) {
                                draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                    LIBERATION_SCREEN_HEIGHT, 8, LIBERATION_SCREEN_HEIGHT / 2,
                                    lib_combat_player_won(&lib_combat) ? "VICTORY!" : "DEFEATED",
                                    0xFFFFFF00, 1);
                            }
                        } else if (lib_in_building) {
                            const char *text = building_interact_text(&lib_interact);
                            if (text) {
                                for (int y = LIBERATION_SCREEN_HEIGHT / 2;
                                     y < LIBERATION_SCREEN_HEIGHT; y++)
                                    for (int x = 0; x < LIBERATION_SCREEN_WIDTH; x++)
                                        framebuffer[y * LIBERATION_SCREEN_WIDTH + x] =
                                            (framebuffer[y * LIBERATION_SCREEN_WIDTH + x] >> 2) & 0x3F3F3F;
                                draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                    LIBERATION_SCREEN_HEIGHT, 8, LIBERATION_SCREEN_HEIGHT / 2 + 8,
                                    text, 0xFFFFFFFF, 1);
                                unsigned count = building_interact_choice_count(&lib_interact);
                                for (unsigned ci = 0; ci < count && ci < 6; ci++) {
                                    char label[64];
                                    snprintf(label, sizeof(label), "%d. %s", ci + 1,
                                        building_interact_choice_label(&lib_interact, ci));
                                    draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                        LIBERATION_SCREEN_HEIGHT, 8,
                                        LIBERATION_SCREEN_HEIGHT / 2 + 20 + (int)ci * 10,
                                        label, 0xFFCCCCCC, 1);
                                }
                            }
                        }
                    } else if (liberation_data.city_frame.bitplanes) {
                        liberation_anim_blit(&liberation_data.city_frame,
                            framebuffer, LIBERATION_SCREEN_WIDTH,
                            LIBERATION_SCREEN_HEIGHT, 0, 44);
                    } else {
                        draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                      LIBERATION_SCREEN_HEIGHT, 118,
                                      "VERIFIED LIBERATION PRESENTATION DATA REQUIRED",
                                      0xFFCCDDEE, 1);
                    }
                } else {
                    /* Captive game tick: energy regen every ~5 seconds */
                    gs.tick++;
                    if (gs.tick % 300 == 0) {
                        for (int di = 0; di < 4; di++) {
                            if (gs.droids[di].hp > 0) {
                                if (gs.droids[di].energy < gs.droids[di].energy_max)
                                    gs.droids[di].energy++;
                                if (gs.droids[di].hp < gs.droids[di].hp_max)
                                    gs.droids[di].hp++;
                            }
                        }
                    }
                    if (gs.tick % 10 == 0) {
                        combat_tick(&creatures, &gs);
                        if (creatures.attack_occurred) {
                            int ti = creatures.last_attack_target;
                            char msg[64];
                            snprintf(msg, sizeof(msg), "Droid %d hit for %d!",
                                     ti + 1,
                                     creatures.last_attack_damage);
                            msg_push(msg, 0xFFFF4444);
                            damage_flash_ttl = 8;
                            sfx_play(&sfx, SFX_HIT);
                            if (ti >= 0 && ti < 4 && gs.droids[ti].hp <= 0) {
                                char dmsg[64];
                                snprintf(dmsg, sizeof(dmsg), "Droid %d destroyed!", ti + 1);
                                msg_push(dmsg, 0xFFFF0000);
                                sfx_play(&sfx, SFX_DEATH);
                                int alive = 0;
                                for (int di = 0; di < 4; di++)
                                    if (gs.droids[di].hp > 0) alive++;
                                if (alive == 0) {
                                    msg_push("All droids destroyed! Mission failed.", 0xFFFF0000);
                                    gs.mode = STATE_GAMEOVER;
                                }
                            }
                        }
                    }
                    for (int mi = 0; mi < MSG_LOG_SIZE; mi++)
                        if (msg_log[mi].ttl > 0) msg_log[mi].ttl--;
                    /* Captive: the verified original GAME SCRN shell. */
                    if (hud_bg) {
                        memcpy(framebuffer, hud_bg,
                               CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT * sizeof(uint32_t));
                        if (gs.mode == STATE_GAME && textures_loaded) {
                            CaptiveViewWindow vw;
                            captive_view_window_build(&gs, &vw);
                            viewport_render(&vw, &atlas, framebuffer,
                                            CAPTIVE_ORIGINAL_WIDTH,
                                            CAPTIVE_ORIGINAL_HEIGHT);
                            viewport_render_creatures(&gs, &creatures, &atlas,
                                            framebuffer, CAPTIVE_ORIGINAL_WIDTH,
                                            CAPTIVE_ORIGINAL_HEIGHT);
                            if (damage_flash_ttl > 0) {
                                damage_flash_ttl--;
                                for (int fy = CAPTIVE_VIEWPORT_Y; fy < CAPTIVE_VIEWPORT_Y + CAPTIVE_VIEWPORT_HEIGHT; fy++)
                                    for (int fx = CAPTIVE_VIEWPORT_X; fx < CAPTIVE_VIEWPORT_X + CAPTIVE_VIEWPORT_WIDTH; fx++) {
                                        uint32_t *p = &framebuffer[fy * CAPTIVE_ORIGINAL_WIDTH + fx];
                                        uint32_t r = (*p >> 16) & 0xFF;
                                        r = r + 60 > 255 ? 255 : r + 60;
                                        *p = (*p & 0xFF00FFFF) | (r << 16);
                                    }
                            }
                            for (int mi = 0; mi < MSG_LOG_SIZE; mi++) {
                                if (msg_log[mi].ttl <= 0) continue;
                                draw_simple_text(framebuffer, CAPTIVE_ORIGINAL_WIDTH,
                                    CAPTIVE_ORIGINAL_HEIGHT,
                                    CAPTIVE_VIEWPORT_X + 4,
                                    CAPTIVE_VIEWPORT_Y + CAPTIVE_VIEWPORT_HEIGHT - 12 - mi * 10,
                                    msg_log[mi].text, msg_log[mi].color, 1);
                            }
                        } else {
                            for (int y = 0; y < CAPTIVE_VIEWPORT_HEIGHT; ++y) {
                                uint32_t *row = framebuffer +
                                    (CAPTIVE_VIEWPORT_Y + y) * CAPTIVE_ORIGINAL_WIDTH +
                                    CAPTIVE_VIEWPORT_X;
                                memset(row, 0, CAPTIVE_VIEWPORT_WIDTH * sizeof(*row));
                            }
                        }
                    }
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
                if (gs.game_type == GAME_LIBERATION) {
                    memset(framebuffer, 0, sizeof(framebuffer));
                    draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                  LIBERATION_SCREEN_HEIGHT, 4,
                                  "INVENTORY", 0xFF44AAFF, 2);
                    {
                        int sel_x = 10 + gs.selected_droid * 78;
                        const Droid *sd = &gs.droids[gs.selected_droid];
                        for (int di = 0; di < 4; di++) {
                            int col_x = 10 + di * 78;
                            char dname[20];
                            snprintf(dname, sizeof(dname), "DROID %d", di + 1);
                            draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                LIBERATION_SCREEN_HEIGHT, col_x, 22, dname,
                                di == gs.selected_droid ? 0xFFFFAA00 : 0xFFAAAAAA, 1);
                        }
                        char hp_str[16];
                        snprintf(hp_str, sizeof(hp_str), "HP %d/%d", sd->hp, sd->hp_max);
                        draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                            LIBERATION_SCREEN_HEIGHT, sel_x, 32, hp_str, 0xFF00AA00, 1);
                        char en_str[16];
                        snprintf(en_str, sizeof(en_str), "EN %d/%d", sd->energy, sd->energy_max);
                        draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                            LIBERATION_SCREEN_HEIGHT, sel_x, 42, en_str, 0xFF0088CC, 1);
                        for (int w = 0; w < 2; w++) {
                            const Item *wi = sd->weapons[w] ? item_db_get(&item_db, sd->weapons[w]) : NULL;
                            char wstr[32];
                            snprintf(wstr, sizeof(wstr), "%s: %s", w == 0 ? "LH" : "RH",
                                     wi ? wi->name : "EMPTY");
                            draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                LIBERATION_SCREEN_HEIGHT, sel_x, 52 + w * 10, wstr, 0xFF88FF88, 1);
                        }
                        for (int si = 0; si < 10; si++) {
                            if (sd->items[si] == 0) continue;
                            const Item *it = item_db_get(&item_db, sd->items[si]);
                            if (!it) continue;
                            draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                LIBERATION_SCREEN_HEIGHT, sel_x, 72 + si * 8, it->name,
                                0xFFAAAADD, 1);
                        }
                    }
                    draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                  LIBERATION_SCREEN_HEIGHT, 10, 160,
                                  "SHARED ITEMS", 0xFFAAAA44, 1);
                    for (int i = 0; i < gs.lib_inventory_count && i < 20; i++) {
                        int ix = 10 + (i % 2) * 155;
                        int iy = 172 + (i / 2) * 10;
                        bool sel = (i == lib_inv_cursor);
                        draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                            LIBERATION_SCREEN_HEIGHT, ix, iy,
                            gs.lib_inventory[i].name,
                            sel ? 0xFFFFFF00 : 0xFFCCCCCC, 1);
                    }
                    char gold_str[32];
                    snprintf(gold_str, sizeof(gold_str), "GOLD: %d", gs.gold);
                    draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                  LIBERATION_SCREEN_HEIGHT, 10, 240,
                                  gold_str, 0xFFFFFF00, 1);
                    draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                  LIBERATION_SCREEN_HEIGHT, 10, 250,
                                  "1-4:DROID ENTER:GIVE E:EQUIP U:UNEQUIP", 0xFF666688, 1);
                } else {
                    droid_ui_render(&droid_ui, &gs, &item_db, framebuffer,
                                    CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
                }
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

            case STATE_HOLAMAP: {
                memset(framebuffer, 0, sizeof(framebuffer));
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              30, "MISSION COMPLETE!", 0xFF44FF44, 2);
                char mission_str[64];
                snprintf(mission_str, sizeof(mission_str),
                         "Next: Mission %d of 10", gs.mission + 1);
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              70, mission_str, 0xFFFFFF44, 1);
                char planet_name[32];
                uint32_t pname_seed = gs.mission_seed + (uint32_t)gs.mission;
                captive_generate_planet_name(&pname_seed,
                                             planet_name, sizeof(planet_name));
                char planet_str[64];
                snprintf(planet_str, sizeof(planet_str), "Planet: %s", planet_name);
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              90, planet_str, 0xFFCCCCFF, 1);
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              120, "ENTER: Launch    S: Shop", 0xFF888888, 1);
                break;
            }

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

            case STATE_HELP: {
                int pw = (gs.game_type == GAME_LIBERATION)
                    ? LIBERATION_SCREEN_WIDTH : CAPTIVE_ORIGINAL_WIDTH;
                int ph = (gs.game_type == GAME_LIBERATION)
                    ? LIBERATION_SCREEN_HEIGHT : CAPTIVE_ORIGINAL_HEIGHT;
                memset(framebuffer, 0, (size_t)pw * ph * sizeof(uint32_t));
                draw_centered(framebuffer, pw, ph, 5, "CONTROLS", 0xFFFFFF44, 2);
                const char *help[] = {
                    "W/UP: Move forward",
                    "S/DOWN: Move backward",
                    "A/LEFT: Turn left",
                    "D/RIGHT: Turn right",
                    "F/ENTER: Interact",
                    "TAB: Inventory",
                    "1-4: Select droid",
                    "SPACE: Attack",
                    ".: Stairs down",
                    ",: Stairs up",
                    "F5: Save  F9: Load",
                    "H: This help",
                    "ESC: Pause menu",
                    "",
                    "Press any key",
                };
                int nlines = (int)(sizeof(help) / sizeof(help[0]));
                for (int i = 0; i < nlines; i++)
                    draw_simple_text(framebuffer, pw, ph,
                                     8, 30 + i * 11, help[i], 0xFFCCCCCC, 1);
                break;
            }

            case STATE_PAUSE: {
                int pw = (gs.game_type == GAME_LIBERATION)
                    ? LIBERATION_SCREEN_WIDTH : CAPTIVE_ORIGINAL_WIDTH;
                int ph = (gs.game_type == GAME_LIBERATION)
                    ? LIBERATION_SCREEN_HEIGHT : CAPTIVE_ORIGINAL_HEIGHT;
                for (int i = 0; i < pw * ph; i++)
                    framebuffer[i] = (framebuffer[i] & 0xFF000000)
                        | ((framebuffer[i] & 0xFEFEFE) >> 1);
                draw_centered(framebuffer, pw, ph, 40, "PAUSED", 0xFFFFFFFF, 3);
                const char *opts[] = {"RESUME", "SETTINGS", "QUIT"};
                for (int i = 0; i < 3; i++) {
                    uint32_t c = (i == pause_cursor) ? 0xFFFFFF44 : 0xFF888888;
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%s %s",
                             (i == pause_cursor) ? ">" : " ", opts[i]);
                    draw_centered(framebuffer, pw, ph, 90 + i * 20, buf, c, 1);
                }
                draw_centered(framebuffer, pw, ph, ph - 20,
                              "ESC: RESUME", 0xFF666666, 1);
                break;
            }

            case STATE_DROID_CONFIG: {
                int cw = CAPTIVE_ORIGINAL_WIDTH;
                int ch = CAPTIVE_ORIGINAL_HEIGHT;
                for (int i = 0; i < cw * ch; i++)
                    framebuffer[i] = 0xFF000000;
                draw_centered(framebuffer, cw, ch, 10, "DROID CONFIGURATION", 0xFF00FF00, 2);
                for (int d = 0; d < 4; d++) {
                    int yy = 45 + d * 35;
                    uint32_t col = (d == droid_config_cursor) ? 0xFFFFFF44 : 0xFF888888;
                    char line[80];
                    snprintf(line, sizeof(line), "%s %s  HP:%d  EN:%d",
                             (d == droid_config_cursor) ? ">" : " ",
                             gs.droids[d].name, gs.droids[d].hp, gs.droids[d].energy);
                    draw_simple_text(framebuffer, cw, ch, 10, yy, line, col, 1);
                    char parts[64];
                    snprintf(parts, sizeof(parts), "  Parts: %d %d %d %d %d %d",
                             gs.droids[d].body_part_hp[0], gs.droids[d].body_part_hp[1],
                             gs.droids[d].body_part_hp[2], gs.droids[d].body_part_hp[3],
                             gs.droids[d].body_part_hp[4], gs.droids[d].body_part_hp[5]);
                    draw_simple_text(framebuffer, cw, ch, 10, yy + 12, parts, 0xFF666666, 1);
                }
                if (droid_config_renaming) {
                    char ren[48];
                    snprintf(ren, sizeof(ren), "RENAME: %s_", gs.droids[droid_config_cursor].name);
                    draw_centered(framebuffer, cw, ch, ch - 35, ren, 0xFFFFFF00, 1);
                } else {
                    draw_simple_text(framebuffer, cw, ch, 10, ch - 35,
                                     "R:RENAME  S:SWAP WEAPONS", 0xFF666666, 1);
                }
                draw_centered(framebuffer, cw, ch, ch - 20, "ENTER: START MISSION", 0xFF00CC00, 1);
                break;
            }

            default: break;
        }

        if (gs.mode == STATE_GAME) {
            if (custom.automap)
                automap_mark(&automap_state, gs.current_level, gs.party_x, gs.party_y);

            if (custom.minimap) {
                const DungeonLevel *mlvl = &gs.levels[gs.current_level];
                minimap_render(framebuffer, frame_width, frame_height,
                               mlvl, gs.party_x, gs.party_y, gs.party_dir,
                               custom.automap ? automap_state.visited : NULL,
                               &custom);
            }

            if (custom.debug_hud)
                debug_hud_render(framebuffer, frame_width, frame_height, &gs, &custom);

            if (custom.replay_record)
                replay.seed = gs.mission_seed;
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
        sfx_update(&sfx);
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
    free(liberation_mission_menu_pixels);
    music_shutdown(&music_sys);
    sound_shutdown(&sound_sys);
    if (intro_loaded) anm_free(&intro_anim);
    if (textures_loaded) texture_atlas_free(&atlas);
    renderer_shutdown(&renderer);
    i18n_free();
    SDL_Quit();
    return exit_status;
}
