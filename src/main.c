#include "opencaptive.h"
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
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
#include "xp_level.h"
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

static int quicksave_slot = 0;
/* Captive can open the same shop from active gameplay or from the mission
 * Holomap.  Keep the caller state so Escape never drops the player into the
 * previous mission after shopping from the Holomap. */
static GameStateMode shop_return_mode = STATE_GAME;
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#if defined(__APPLE__) && TARGET_OS_OSX
#include "macos_menu.h"
#endif

static uint32_t framebuffer[MENU_WIDTH * MENU_HEIGHT];

/* renderer_present() preserves the canvas aspect ratio and centers it inside
 * the window.  SDL mouse coordinates are window-relative, so direct
 * width/height scaling is wrong whenever the window has letterbox bars. */
static bool window_to_canvas(SDL_Window *window, float window_x, float window_y,
                             int canvas_width, int canvas_height,
                             float *canvas_x, float *canvas_y) {
    if (!window || !canvas_x || !canvas_y || canvas_width <= 0 || canvas_height <= 0)
        return false;
    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);
    if (window_width <= 0 || window_height <= 0) return false;
    float scale_x = (float)window_width / canvas_width;
    float scale_y = (float)window_height / canvas_height;
    float scale = scale_x < scale_y ? scale_x : scale_y;
    if (!(scale > 0.0f)) return false;
    float offset_x = (window_width - canvas_width * scale) / 2.0f;
    float offset_y = (window_height - canvas_height * scale) / 2.0f;
    float x = (window_x - offset_x) / scale;
    float y = (window_y - offset_y) / scale;
    if (x < 0.0f || x >= canvas_width || y < 0.0f || y >= canvas_height)
        return false;
    *canvas_x = x;
    *canvas_y = y;
    return true;
}

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
    int trailing = EOF;
    /* Do not query the stream after a short read: a failed read may leave the
     * file position indeterminate.  A complete reference must still have no
     * trailing byte. */
    if (read == DOS_VGA_MEMORY_SIZE)
        trailing = fgetc(file);
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
        strcmp(magic, "P6") != 0 || width <= 0 || height <= 0 || maximum != 255) {
        fclose(file);
        return NULL;
    }
    int separator = fgetc(file);
    if (separator == EOF) {
        fclose(file);
        return NULL;
    }
    if (separator == '\r') {
        int next = fgetc(file);
        if (next == EOF) {
            fclose(file);
            return NULL;
        }
        if (next != '\n') ungetc(next, file);
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
    int trailing = fgetc(file);
    bool read_ok = trailing == EOF && !ferror(file);
    int close_result = fclose(file);
    if (!read_ok || close_result != 0) {
        free(pixels); return NULL;
    }
    *out_width = width;
    *out_height = height;
    return pixels;
}

static int compare_ppm_frames(const char *expected_path, const char *actual_path,
                              const int *expected_rect, const int *actual_rect) {
    int expected_width = 0, expected_height = 0, actual_width = 0, actual_height = 0;
    uint32_t *expected = read_ppm_frame(expected_path, &expected_width, &expected_height);
    uint32_t *actual = read_ppm_frame(actual_path, &actual_width, &actual_height);
    if (!expected || !actual ||
        (!expected_rect && !actual_rect &&
         (expected_width != actual_width || expected_height != actual_height))) {
        fprintf(stderr, "PPM frames must be complete P6 images; full-frame comparisons require equal dimensions\n");
        free(expected); free(actual);
        return 2;
    }
    int expected_x = 0, expected_y = 0;
    int actual_x = 0, actual_y = 0;
    int width = expected_width, height = expected_height;
    if (expected_rect || actual_rect) {
        if (!expected_rect || !actual_rect) {
            fprintf(stderr, "Both comparison regions are required\n");
            free(expected); free(actual);
            return 2;
        }
        expected_x = expected_rect[0]; expected_y = expected_rect[1];
        actual_x = actual_rect[0]; actual_y = actual_rect[1];
        width = expected_rect[2]; height = expected_rect[3];
        if (width != actual_rect[2] || height != actual_rect[3] ||
            expected_x < 0 || expected_y < 0 || actual_x < 0 || actual_y < 0 ||
            width <= 0 || height <= 0 ||
            width > expected_width - expected_x || height > expected_height - expected_y ||
            width > actual_width - actual_x || height > actual_height - actual_y) {
            fprintf(stderr, "Comparison region is outside a frame or has mismatched dimensions\n");
            free(expected); free(actual);
            return 2;
        }
    }
    FrameComparison result = {0};
    for (int row = 0; row < height; ++row) {
        FrameComparison line = frame_compare_argb(
            expected + (size_t)(expected_y + row) * expected_width + expected_x,
            actual + (size_t)(actual_y + row) * actual_width + actual_x,
            (size_t)width);
        result.pixel_count += line.pixel_count;
        result.different_pixels += line.different_pixels;
        result.total_channel_difference += line.total_channel_difference;
        if (line.maximum_channel_difference > result.maximum_channel_difference)
            result.maximum_channel_difference = line.maximum_channel_difference;
    }
    printf("Frame comparison%s: %zu/%zu pixels differ; channel difference=%llu; max=%u\n",
           expected_rect ? " (region)" : "",
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

static bool parse_float_option(const char *text, float *value) {
    if (!text || !value) return false;
    char *end = NULL;
    errno = 0;
    float parsed = strtof(text, &end);
    if (errno == ERANGE || end == text || *end != '\0' || !isfinite(parsed) || parsed <= 0.0f)
        return false;
    *value = parsed;
    return true;
}

static bool parse_resolution_option(const char *text, int *width, int *height) {
    if (!text || !width || !height) return false;
    const char *separator = strchr(text, 'x');
    if (!separator || separator == text || !separator[1] || strchr(separator + 1, 'x')) return false;
    size_t left_len = (size_t)(separator - text);
    if (left_len >= 32 || strlen(separator + 1) >= 32) return false;
    char left[32], right[32];
    memcpy(left, text, left_len);
    left[left_len] = '\0';
    snprintf(right, sizeof(right), "%s", separator + 1);
    return parse_int_option(left, 320, INT_MAX, width) &&
           parse_int_option(right, 200, INT_MAX, height);
}

/* Replay actions use a stable byte code rather than SDL keycode values,
 * which are not guaranteed to be identical across platforms. */
static bool replay_encode_key(SDL_Keycode key, uint8_t *action) {
    if (!action) return false;
    switch (key) {
        case SDLK_W: case SDLK_UP: *action = 1; return true;
        case SDLK_S: case SDLK_DOWN: *action = 2; return true;
        case SDLK_Q: *action = 3; return true;
        case SDLK_E: *action = 4; return true;
        case SDLK_A: case SDLK_LEFT: *action = 5; return true;
        case SDLK_D: case SDLK_RIGHT: *action = 6; return true;
        case SDLK_M: *action = 7; return true;
        case SDLK_I: *action = 8; return true;
        case SDLK_T: *action = 9; return true;
        case SDLK_1: *action = 10; return true;
        case SDLK_2: *action = 11; return true;
        case SDLK_3: *action = 12; return true;
        case SDLK_4: *action = 13; return true;
        case SDLK_SPACE: *action = 14; return true;
        case SDLK_F: *action = 15; return true;
        case SDLK_PERIOD: *action = 16; return true;
        case SDLK_COMMA: *action = 17; return true;
        case SDLK_H: *action = 18; return true;
        default: return false;
    }
}

static SDL_Keycode replay_decode_key(uint8_t action) {
    static const SDL_Keycode keys[] = {
        SDLK_UNKNOWN, SDLK_W, SDLK_S, SDLK_Q, SDLK_E, SDLK_A, SDLK_D,
        SDLK_M, SDLK_I, SDLK_T, SDLK_1, SDLK_2, SDLK_3, SDLK_4,
        SDLK_SPACE, SDLK_F, SDLK_PERIOD, SDLK_COMMA, SDLK_H
    };
    return action < sizeof(keys) / sizeof(keys[0]) ? keys[action] : SDLK_UNKNOWN;
}

static void get_default_data_path(char *buf, size_t bufsize) {
#ifdef _WIN32
    // Windows: <exe_dir>\data
    char exe_path[512] = {0};
    GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    char *last_sep = strrchr(exe_path, '\\');
    if (last_sep) *last_sep = '\0';
    snprintf(buf, bufsize, "%s\\data", exe_path);
#elif defined(__ANDROID__)
    // Android: /sdcard/OpenCaptive (external storage, accessible to user)
    const char *ext = getenv("EXTERNAL_STORAGE");
    if (ext) {
        snprintf(buf, bufsize, "%s/OpenCaptive", ext);
    } else {
        snprintf(buf, bufsize, "/sdcard/OpenCaptive");
    }
#elif defined(__IPHONEOS__) || (defined(__APPLE__) && defined(TARGET_OS_IOS) && TARGET_OS_IOS)
    // iOS: <app>/Documents/OpenCaptive (iTunes File Sharing / Files app)
    const char *home = getenv("HOME");
    if (home) {
        snprintf(buf, bufsize, "%s/Documents/OpenCaptive", home);
    } else {
        snprintf(buf, bufsize, "Documents/OpenCaptive");
    }
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
    return captive_data_available(vfs);
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

static void apply_menu_config(OpenCaptiveConfig *config, const StartMenu *menu,
                              CustomFeatures *feat) {
    static const int window_widths[] = {960, 1280, 1600, 1920};
    static const int window_heights[] = {600, 800, 1000, 1200};
    config->data_path = menu->data_path;
    config->platform = menu->platform;
    config->render_mode = menu->enhanced_mode
        ? CAPTIVE_RENDER_ENHANCED : CAPTIVE_RENDER_ORIGINAL;
    config->renderer_backend = menu->renderer_backend;
    config->scale_factor = menu->scale_factor;
    int window_size = menu->window_size;
    if (window_size < 0 || window_size >= 4) window_size = 1;
    if (menu->scale_custom && menu->scale_factor >= 1 &&
        menu->scale_factor <= 5) {
        /* The menu's default scale is intentionally neutral: WINDOW SIZE
         * controls the normal presets.  A deliberately changed SCALE value
         * is a custom native-size request and must not be overwritten by the
         * preset assignment above. */
        config->window_width = CAPTIVE_ORIGINAL_WIDTH * menu->scale_factor;
        config->window_height = CAPTIVE_ORIGINAL_HEIGHT * menu->scale_factor;
        if (config->window_width < 640) config->window_width = 640;
        if (config->window_height < 400) config->window_height = 400;
    } else {
        config->window_width = window_widths[window_size];
        config->window_height = window_heights[window_size];
    }
    config->fullscreen = menu->fullscreen;
    config->vsync = menu->vsync;
    config->scanlines = menu->scanlines;
    config->crt_curvature = menu->crt_curvature;
    config->bilinear = menu->bilinear;
    config->integer_scaling = menu->integer_scaling;
    config->fps_limit = menu->fps_limit;
    config->brightness = menu->brightness;
    config->contrast = menu->contrast;
    config->gamma = menu->gamma;
    config->master_volume = menu->master_volume;
    if (feat) {
        static const int sample_rates[] = {22050, 44100, 48000};
        feat->audio_sample_rate = sample_rates[menu->audio_sample_rate];
        feat->audio_reverb = menu->audio_reverb;
        static const float speeds[] = {0.5f, 1.0f, 2.0f};
        feat->game_speed = speeds[menu->game_speed];
        feat->speed_control = (menu->game_speed != 1);
        feat->mouse_sensitivity = (float)menu->mouse_sensitivity;
    }
}

static void release_liberation_session_assets(void);

static void sync_menu_from_config(StartMenu *menu, const OpenCaptiveConfig *config,
                                  const CustomFeatures *feat,
                                  bool music_enabled, bool sfx_enabled) {
    /* Returning from pause must retain the menu's loaded artwork and fonts.
     * start_menu_init() clears those pointers without freeing them; the
     * resource-preserving reinit also remains safe for the zeroed menu used
     * during the first startup. */
    release_liberation_session_assets();
    start_menu_reinit(menu);
    strncpy(menu->data_path, config->data_path, sizeof(menu->data_path) - 1);
    menu->data_path[sizeof(menu->data_path) - 1] = '\0';
    menu->data_path_cursor = (int)strlen(menu->data_path);
    menu->enhanced_mode = config->render_mode == CAPTIVE_RENDER_ENHANCED;
    menu->renderer_backend = config->renderer_backend;
    if (menu->renderer_backend < 0 || menu->renderer_backend > 2)
        menu->renderer_backend = 0;
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
    menu->scale_custom = false;
    if (config->scale_factor >= 1 && config->scale_factor <= 5 &&
        config->scale_factor != 3) {
        int scaled_width = CAPTIVE_ORIGINAL_WIDTH * config->scale_factor;
        int scaled_height = CAPTIVE_ORIGINAL_HEIGHT * config->scale_factor;
        if (scaled_width < 640) scaled_width = 640;
        if (scaled_height < 400) scaled_height = 400;
        menu->scale_custom = config->window_width == scaled_width &&
                             config->window_height == scaled_height;
    }
    menu->gamma = config->gamma;
    menu->master_volume = config->master_volume;
    {
        static const int window_widths[] = {960, 1280, 1600, 1920};
        static const int window_heights[] = {600, 800, 1000, 1200};
        menu->window_size = 1;
        for (int i = 0; i < 4; ++i) {
            if (config->window_width == window_widths[i] &&
                config->window_height == window_heights[i]) {
                menu->window_size = i;
                break;
            }
        }
    }
    if (feat) {
        if (feat->audio_sample_rate == 22050) menu->audio_sample_rate = 0;
        else if (feat->audio_sample_rate == 48000) menu->audio_sample_rate = 2;
        else menu->audio_sample_rate = 1;
        menu->audio_reverb = feat->audio_reverb;
        if (feat->game_speed <= 0.6f) menu->game_speed = 0;
        else if (feat->game_speed >= 1.5f) menu->game_speed = 2;
        else menu->game_speed = 1;
        menu->mouse_sensitivity = (int)feat->mouse_sensitivity;
        if (menu->mouse_sensitivity < 1) menu->mouse_sensitivity = 1;
        if (menu->mouse_sensitivity > 10) menu->mouse_sensitivity = 10;
    }
    /* Keep the first launcher frame responsive: the same incremental scanner
     * used by D runs in the background and refreshes the card badges as it
     * completes.  Game start still performs a final synchronous validation. */
    start_menu_start_scan(menu, menu->data_path, false);
    start_menu_check_saves(menu);
    if (menu->captive_save_slot >= 0)
        quicksave_slot = menu->captive_save_slot;
}

static CreatureList creatures;
static PuzzleList puzzles;

static void generate_captive_puzzles(GameState *gs) {
    if (!gs || gs->game_type != GAME_CAPTIVE ||
        gs->num_levels < 1 || gs->num_levels > MAX_LEVELS) return;
    puzzle_init(&puzzles);
    for (int level = 0; level < gs->num_levels; level++)
        puzzle_generate(&puzzles, &gs->levels[level], level,
                        gs->mission_seed);
}

static void generate_captive_encounters(const GameState *gs) {
    if (!gs || gs->game_type != GAME_CAPTIVE ||
        gs->num_levels < 1 || gs->num_levels > MAX_LEVELS) return;
    combat_init(&creatures);
    for (int level = 0; level < gs->num_levels; level++)
        combat_spawn_for_level_avoiding_party(&creatures, &gs->levels[level],
                                              level, gs->mission_seed, gs);
}

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
static int levelup_flash_ttl;
static int generator_flash_ttl;
static int recharge_flash_ttl;
static int stair_flash_ttl;
static int door_flash_ttl;
static int fire_flash_ttl;
static int creature_death_flash_ttl;
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

static int menu_audio_sample_rate(const StartMenu *menu) {
    static const int sample_rates[] = {22050, 44100, 48000};
    int choice = menu ? menu->audio_sample_rate : 1;
    if (choice < 0 || choice >= (int)(sizeof(sample_rates) / sizeof(sample_rates[0])))
        choice = 1;
    return sample_rates[choice];
}

static void apply_menu_audio(const StartMenu *menu) {
    if (!menu) return;

    /* The menu owns this setting, but SDL's audio stream and the MIDI
     * renderer are created before the menu loop.  Rebuild both when the
     * selected rate changes so the choice is effective for the next session
     * without requiring a process restart. */
    int sample_rate = menu_audio_sample_rate(menu);
    if ((int)sound_sys.sample_rate != sample_rate) {
        const DataVFS *vfs = music_sys.vfs;
        int old_rate = sound_sys.sample_rate ? (int)sound_sys.sample_rate
                                             : SOUND_SAMPLE_RATE;
        bool sound_enabled = sound_sys.enabled;
        bool reverb_enabled = sound_sys.reverb_enabled;
        float reverb_amount = sound_sys.reverb_amount;
        float sound_volume = sound_sys.master_volume;
        bool music_enabled = music_sys.enabled;
        float music_volume = music_sys.master_volume;
        bool high_quality = music_sys.high_quality;

        music_shutdown(&music_sys);
        sound_shutdown(&sound_sys);
        if (!sound_init(&sound_sys, (uint32_t)sample_rate)) {
            /* Keep a usable audio path if the requested device format is not
             * accepted by the host.  The menu selection remains persisted
             * and will be retried on the next session. */
            sound_init(&sound_sys, (uint32_t)old_rate);
        }
        sound_set_reverb(&sound_sys, reverb_enabled, reverb_amount);
        sound_set_volume(&sound_sys, sound_volume);
        sound_set_enabled(&sound_sys, sound_enabled);
        music_init(&music_sys, &sound_sys, vfs, (int)sound_sys.sample_rate,
                   high_quality);
        music_set_volume(&music_sys, music_volume);
        music_set_enabled(&music_sys, music_enabled);
    }

    /* Reverb is a menu-owned setting too.  Keep the amount selected by the
     * feature configuration, but apply the menu's enable/disable choice to
     * the live mixer before the next game session starts. */
    sound_set_reverb(&sound_sys, menu->audio_reverb, sound_sys.reverb_amount);
    float volume = (float)menu->master_volume / 100.0f;
    sound_set_volume(&sound_sys, volume);
    music_set_volume(&music_sys, volume);
    music_set_enabled(&music_sys, menu->music_enabled);
    sound_set_enabled(&sound_sys, menu->sfx_enabled);
}
/* The CD32 media currently verifies presentation only.  Keep the recovered
 * menu and ANIM frames live, but do not enter the older generated city and
 * 3D-projection prototype from the default game path. */
static const bool liberation_prototype_gameplay_enabled = false;
static uint32_t *liberation_mission_menu_pixels;
static uint16_t liberation_mission_menu_width;
static uint16_t liberation_mission_menu_height;

enum { LIBERATION_MISSION_MENU_Y = 56 };

static CityGrid lib_buildings;
static CityGridState lib_grid;
static CityNavState lib_nav;
static Lib3dState lib_render;
static bool liberation_texture_filter;
static bool liberation_dynamic_lighting;
static BuildingInteraction lib_interact;
static bool lib_city_generated;
static bool lib_in_building;
/* A bar fight is resolved after leaving the bar, but its police fine is
 * offered during a later visit to a police station.  Keep that consequence
 * outside the short-lived BuildingInteraction instance. */
static bool lib_bar_fight_pending;
static LibCombatState lib_combat;
static PlotgenState lib_plot;
static bool lib_mission_briefing;
static bool lib_in_combat;
static bool lib_in_dungeon;
static int lib_dungeon_entry_x;
static int lib_dungeon_entry_y;
static int lib_inv_cursor;

/* Liberation saves keep their shared item type wide enough for the original
 * format, while the active OpenCaptive droid inventory uses the Captive
 * database's 8-bit IDs.  Never truncate a shared/save item into a different
 * runtime item. */
static bool liberation_runtime_item_id(uint16_t item_type, uint8_t *out_id) {
    if (!out_id || item_type == 0 || item_type > UINT8_MAX) return false;
    uint8_t id = (uint8_t)item_type;
    if (!item_db_get(&item_db, id)) return false;
    *out_id = id;
    return true;
}

static bool liberation_is_armor_id(uint8_t item_id) {
    const Item *item = item_db_get(&item_db, item_id);
    return item && item->category >= ITEM_ARMOR_HEAD &&
           item->category <= ITEM_ARMOR_HAND;
}

static bool liberation_is_weapon_id(uint8_t item_id) {
    const Item *item = item_db_get(&item_db, item_id);
    return item && item->category >= ITEM_WEAPON_MELEE &&
           item->category <= ITEM_WEAPON_SPRAY;
}

static void liberation_restore_nav_position(int saved_x, int saved_y,
                                            CityDirection facing) {
    int x = saved_x;
    int y = saved_y;
    if (liberation_prototype_gameplay_enabled && lib_city_generated &&
        !city_nav_is_road(&lib_grid, x, y)) {
        int best_dist = INT_MAX;
        for (int cy = 0; cy < CITYGRID_HEIGHT; cy++) {
            for (int cx = 0; cx < CITYGRID_WIDTH; cx++) {
                if (!city_nav_is_road(&lib_grid, cx, cy)) continue;
                int dist = abs(cx - saved_x) + abs(cy - saved_y);
                if (dist < best_dist) {
                    best_dist = dist;
                    x = cx;
                    y = cy;
                }
            }
        }
    }
    city_nav_init(&lib_nav, x, y, facing);
}

static int pause_cursor;
static int droid_config_cursor;
static bool droid_config_renaming;
static int droid_config_name_pos;
static int taxi_flash_ttl;

static void release_liberation_session_assets(void) {
    liberation_data_close(&liberation_data);
    free(liberation_mission_menu_pixels);
    liberation_mission_menu_pixels = NULL;
    liberation_mission_menu_width = 0;
    liberation_mission_menu_height = 0;
    liberation_intro_active = false;
    liberation_mission_menu_active = false;
}


typedef struct {
    bool open;
    int selected;
    bool invulnerable;
    bool infinite_energy;
} RuntimePopup;

static RuntimePopup runtime_popup;
/* The F10 renderer is defined before the game-loop setup that assigns this
 * pointer, so the declaration must live with the other popup state. */
static CustomFeatures *custom_feat_ptr = NULL;

enum {
    POPUP_ENHANCED,
    POPUP_SCANLINES,
    POPUP_CRT,
    POPUP_BILINEAR,
    POPUP_TEXTURE_FILTER,
    POPUP_DYNAMIC_LIGHTING,
    POPUP_BRIGHTNESS,
    POPUP_MUSIC,
    POPUP_SFX,
    POPUP_INVULNERABLE,
    POPUP_INFINITE_ENERGY,
    POPUP_MINIMAP,
    POPUP_REVEAL_MAP,
    POPUP_DEBUG_HUD,
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
    ['>'] = {0x40,0x20,0x10,0x20,0x40},
};

static uint32_t utf8_decode(const char **p) {
    const uint8_t *s = (const uint8_t *)*p;
    uint32_t cp;
    if (s[0] < 0x80) { cp = s[0]; *p += 1; }
    else if ((s[0] & 0xE0) == 0xC0 && s[1] != '\0' &&
             (s[1] & 0xC0) == 0x80) {
        cp = ((uint32_t)(s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        *p += 2;
    } else if ((s[0] & 0xF0) == 0xE0 && s[1] != '\0' && s[2] != '\0' &&
               (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80) {
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
    return value ? _("ON") : _("OFF");
}

static const char *popup_brightness(int value) {
    return value < 40 ? _("LOW") : (value > 60 ? _("HIGH") : _("NORMAL"));
}

static void popup_apply_cheats(GameState *gs) {
    if (!gs) return;
    bool restored_health = false;
    if (runtime_popup.invulnerable) {
        for (int i = 0; i < 4; ++i) gs->droids[i].hp = gs->droids[i].hp_max;
        restored_health = true;
    }
    if (runtime_popup.infinite_energy) {
        for (int i = 0; i < 4; ++i) gs->droids[i].energy = gs->droids[i].energy_max;
    }
    /* A lethal hazard may set game-over during the same input event that was
     * processed after the popup's previous frame.  Invulnerability must undo
     * that transient transition as well as restore the droid HP. */
    if (restored_health && gs->mode == STATE_GAMEOVER)
        gs->mode = STATE_GAME;
}

static void popup_handle_event(GameState *gs, OpenCaptiveConfig *config,
                               OpenCaptiveRenderer *renderer,
                               CustomFeatures *features, const SDL_Event *event) {
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
            config->render_mode = config->render_mode == CAPTIVE_RENDER_ENHANCED
                ? CAPTIVE_RENDER_ORIGINAL : CAPTIVE_RENDER_ENHANCED;
            break;
        case POPUP_SCANLINES: config->scanlines = !config->scanlines; break;
        case POPUP_CRT: config->crt_curvature = !config->crt_curvature; break;
        case POPUP_BILINEAR: config->bilinear = !config->bilinear; break;
        case POPUP_TEXTURE_FILTER:
            if (features) features->texture_filter = !features->texture_filter;
            liberation_texture_filter = features && features->texture_filter;
            lib3d_set_texture_filter(&lib_render, liberation_texture_filter);
            break;
        case POPUP_DYNAMIC_LIGHTING:
            if (features) features->dynamic_lighting = !features->dynamic_lighting;
            liberation_dynamic_lighting = features && features->dynamic_lighting;
            lib3d_set_dynamic_lighting(&lib_render, liberation_dynamic_lighting);
            break;
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
        case POPUP_MINIMAP:
            if (features) features->minimap = !features->minimap;
            break;
        case POPUP_REVEAL_MAP:
            if (features) features->reveal_map = !features->reveal_map;
            break;
        case POPUP_DEBUG_HUD:
            if (features) features->debug_hud = !features->debug_hud;
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
    /* Keep the renderer's live mode/display state in sync with the popup.
     * The menu path already applies this after configuration changes, but
     * F10 must do it immediately while a game is running. */
    renderer_apply_display(renderer, config);
    renderer_set_effects(renderer, config->bilinear, config->scanlines,
                         config->crt_curvature,
                         config->brightness, config->contrast, config->gamma);
    popup_apply_cheats(gs);
}

static const char *popup_label(int item) {
    switch (item) {
        case POPUP_ENHANCED: return _("ENHANCED DISPLAY");
        case POPUP_SCANLINES: return _("SCANLINES");
        case POPUP_CRT: return _("CRT CURVE");
        case POPUP_BILINEAR: return _("BILINEAR");
        case POPUP_TEXTURE_FILTER: return _("TEXTURE FILTER");
        case POPUP_DYNAMIC_LIGHTING: return _("DYNAMIC LIGHTING");
        case POPUP_BRIGHTNESS: return _("BRIGHTNESS");
        case POPUP_MUSIC: return _("MUSIC");
        case POPUP_SFX: return _("SFX");
        case POPUP_INVULNERABLE: return _("GOD MODE");
        case POPUP_INFINITE_ENERGY: return _("INFINITE ENERGY");
        case POPUP_MINIMAP: return _("MINIMAP");
        case POPUP_REVEAL_MAP: return _("REVEAL MAP");
        case POPUP_DEBUG_HUD: return _("DEBUG HUD");
        case POPUP_COMPLETE_OBJECTIVE: return _("COMPLETE OBJECTIVE");
        case POPUP_CLOSE: return _("CLOSE");
        default: return "";
    }
}

static void popup_render(const GameState *gs, const CustomFeatures *features,
                         uint32_t *fb, int pw, int ph) {
    int x = 6, y = 6, w = pw - 12, h = ph - 12;
    bool compact = ph < 190;
    int row_start = y + (compact ? 20 : 29);
    int row_step = compact ? 7 : 9;
    draw_rect(fb, pw, ph, x, y, w, h, 0xFF101420);
    draw_rect(fb, pw, ph, x, y, w, 2, 0xFF55CCFF);
    draw_rect(fb, pw, ph, x, y + h - 2, w, 2, 0xFF55CCFF);
    draw_centered(fb, pw, ph, y + (compact ? 2 : 7),
                  _("RUNTIME OPTIONS"), 0xFFFFFFFF, 1);
    if (!compact)
        draw_centered(fb, pw, ph, y + 17, _("F10 OR ESC CLOSE"), 0xFF99AACC, 1);
    for (int i = 0; i < POPUP_ITEMS; i++) {
        int row_y = row_start + i * row_step;
        uint32_t color = i == runtime_popup.selected ? 0xFFFFFF44 : 0xFFCCDDEE;
        draw_simple_text(fb, pw, ph, x + 12, row_y, popup_label(i), color, 1);
        const char *value = "";
        switch (i) {
            case POPUP_ENHANCED:
                value = popup_toggle(gs->config.render_mode == CAPTIVE_RENDER_ENHANCED);
                break;
            case POPUP_SCANLINES: value = popup_toggle(gs->config.scanlines); break;
            case POPUP_CRT: value = popup_toggle(gs->config.crt_curvature); break;
            case POPUP_BILINEAR: value = popup_toggle(gs->config.bilinear); break;
            case POPUP_TEXTURE_FILTER:
                value = popup_toggle(features && features->texture_filter); break;
            case POPUP_DYNAMIC_LIGHTING:
                value = popup_toggle(features && features->dynamic_lighting); break;
            case POPUP_BRIGHTNESS: value = popup_brightness(gs->config.brightness); break;
            case POPUP_MUSIC: value = popup_toggle(music_sys.enabled); break;
            case POPUP_SFX: value = popup_toggle(sound_sys.enabled); break;
            case POPUP_INVULNERABLE: value = popup_toggle(runtime_popup.invulnerable); break;
            case POPUP_INFINITE_ENERGY: value = popup_toggle(runtime_popup.infinite_energy); break;
            case POPUP_MINIMAP: value = popup_toggle(features && features->minimap); break;
            case POPUP_REVEAL_MAP: value = popup_toggle(features && features->reveal_map); break;
            case POPUP_DEBUG_HUD: value = popup_toggle(features && features->debug_hud); break;
            case POPUP_COMPLETE_OBJECTIVE: value = _("ACTIVATE"); break;
            default: break;
        }
        draw_simple_text(fb, pw, ph, x + w - 80, row_y, value, color, 1);
    }
    if (!compact)
        draw_centered(fb, pw, ph, y + h - 10,
                      _("UP DOWN ENTER TOGGLE"), 0xFF99AACC, 1);
}

static void restore_liberation_save_state(GameState *gs_ptr,
                                          const LibSaveData *save) {
    if (!gs_ptr || !save) return;
    /* Pending police fines belong to the live interaction flow and are not
     * represented in the legacy Liberation save format.  Do not let a fine
     * from the pre-load session leak into the restored session. */
    lib_bar_fight_pending = false;
    memset(gs_ptr->droids, 0, sizeof(gs_ptr->droids));
    for (int i = 0; i < 4; i++)
        memset(gs_ptr->droids[i].body_part_hp, 255,
               sizeof(gs_ptr->droids[i].body_part_hp));
    gs_ptr->mission = (int)save->mission;
    gs_ptr->mission_seed = ((uint32_t)save->seed_hi << 16) | save->seed_lo;
    gs_ptr->gold = save->gold > (uint32_t)INT_MAX ? INT_MAX :
        (int)save->gold;
    gs_ptr->lib_inventory_count = save->shared_inventory_count > 40 ? 0 :
        save->shared_inventory_count;
    for (int i = 0; i < gs_ptr->lib_inventory_count; i++) {
        snprintf(gs_ptr->lib_inventory[i].name,
                 sizeof(gs_ptr->lib_inventory[i].name), "%s",
                 save->shared_inventory[i].name);
        gs_ptr->lib_inventory[i].item_type =
            save->shared_inventory[i].item_type;
    }
    gs_ptr->tick = save->tick;
    gs_ptr->generators_destroyed = (int)save->generators_destroyed;
    gs_ptr->generators_total = (int)save->generators_total;
    gs_ptr->reputation = save->version >= LIB_SAVE_REPUTATION_VERSION ? save->reputation : 0;
    memcpy(gs_ptr->lib_mission_complete, save->mission_complete,
           sizeof(gs_ptr->lib_mission_complete));
    for (int i = 0; i < 4 && i < save->num_droids; i++) {
        snprintf(gs_ptr->droids[i].name, sizeof(gs_ptr->droids[i].name),
                 "%s", save->droids[i].name);
        gs_ptr->droids[i].hp = save->droids[i].hp;
        gs_ptr->droids[i].hp_max = save->droids[i].hp_max;
        gs_ptr->droids[i].energy = save->droids[i].energy;
        gs_ptr->droids[i].energy_max = save->droids[i].energy_max;
        gs_ptr->droids[i].xp = save->droids[i].xp;
        memset(gs_ptr->droids[i].skill_levels, 0,
               sizeof(gs_ptr->droids[i].skill_levels));
        memcpy(gs_ptr->droids[i].skill_levels, save->droids[i].skills, 8);
        if (save->version >= LIB_SAVE_SKILLS_VERSION)
            memcpy(gs_ptr->droids[i].skill_levels + 8,
                   save->droids[i].skills + 8, 2);
        for (int p = 0; p < 6; p++) {
            uint8_t id;
            if (liberation_runtime_item_id(save->droids[i].equipment[p], &id) &&
                liberation_is_armor_id(id))
                gs_ptr->droids[i].body_parts[p] = id;
        }
        for (int w = 0; w < 2; w++) {
            uint8_t id;
            if (liberation_runtime_item_id(save->droids[i].equipment[6 + w], &id) &&
                liberation_is_weapon_id(id))
                gs_ptr->droids[i].weapons[w] = id;
        }
        memcpy(gs_ptr->droids[i].items, save->droids[i].inventory,
               sizeof(gs_ptr->droids[i].items));
        for (size_t si = 0; si < sizeof(gs_ptr->droids[i].items); si++)
            if (!item_db_get(&item_db, gs_ptr->droids[i].items[si]))
                gs_ptr->droids[i].items[si] = 0;
        if (save->version >= LIB_SAVE_BODY_PART_VERSION)
            memcpy(gs_ptr->droids[i].body_part_hp, save->droids[i].body_part_hp,
                   sizeof(gs_ptr->droids[i].body_part_hp));
        droid_recalc_weapon_damage(&gs_ptr->droids[i], &item_db);
    }
    liberation_restore_nav_position(save->city_x, save->city_y,
                                    (CityDirection)save->facing);
}

static void start_liberation_session(GameState *gs_ptr) {
    if (!gs_ptr) return;
    if (gs_ptr->mission < 1) gs_ptr->mission = 1;
    gs_ptr->game_type = GAME_LIBERATION;
    gs_ptr->mode = STATE_GAME;
    /* These belong to the live session, not to the previous game or to a
     * save file.  Reset them before the prototype-only setup below so the
     * verified presentation path cannot inherit a stale dungeon/building/
     * combat state when starting another Liberation session. */
    lib_in_building = false;
    lib_in_combat = false;
    lib_in_dungeon = false;
    lib_bar_fight_pending = false;
    lib_inv_cursor = 0;
    liberation_intro_active = !skip_liberation_intro_requested &&
                              liberation_data.intro_frame.bitplanes != NULL;
    liberation_mission_menu_active = false;
    lib_mission_briefing = false;

    if (liberation_prototype_gameplay_enabled && !lib_city_generated) {
        uint16_t seed = (uint16_t)(gs_ptr->mission_seed & 0xFFFF);
        uint16_t seed_hi = (uint16_t)((gs_ptr->mission_seed >> 16) & 0xFFFF);
        if (!seed) seed = 0x1234;
        if (!seed_hi) seed_hi = 0x5678;
        citygen_generate(&lib_buildings, seed, (uint16_t)gs_ptr->mission);
        citygrid_init(&lib_grid, seed_hi, seed, gs_ptr->mission);
        citygrid_generate(&lib_grid);
        citygrid_map_buildings(&lib_grid, &lib_buildings);
        int start_x, start_y;
        if (lib_grid.entry_point >= 0 && lib_grid.entry_point < CITYGRID_CELLS) {
            start_x = lib_grid.entry_point % CITYGRID_WIDTH;
            start_y = lib_grid.entry_point / CITYGRID_WIDTH;
        } else {
            start_x = 32;
            start_y = 32;
        }
        if (start_x == 0 && start_y == 0) { start_x = 32; start_y = 32; }
        city_nav_init(&lib_nav, start_x, start_y, CITY_DIR_NORTH);
        lib3d_init(&lib_render);
        lib3d_set_texture_filter(&lib_render, liberation_texture_filter);
        lib3d_set_dynamic_lighting(&lib_render, liberation_dynamic_lighting);
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
        lib_bar_fight_pending = false;
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
    /* The verified resource is a 320x109 composition placed at y=56.  Keep
     * malformed or incompatible data from making the later row copy write
     * past the native Liberation canvas. */
    if (sprite.width > LIBERATION_SCREEN_WIDTH ||
        sprite.height > LIBERATION_SCREEN_HEIGHT - LIBERATION_MISSION_MENU_Y) {
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

static void apply_droid_damage(Droid *d, int damage) {
    if (!d || damage <= 0 || d->hp <= 0) return;
    if (damage >= d->hp)
        d->hp = 0;
    else
        d->hp = (int16_t)(d->hp - damage);
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
                    lib_bar_fight_pending = false;
                    gs->reputation += 15;
                    if (gs->reputation > 100) gs->reputation = 100;
                    msg_push(_("Fine paid. Rep +15"), 0xFF44FF44);
                }
                if (lib_interact.industrial_hazard) {
                    lib_interact.industrial_hazard = false;
                    int dmg = 5 + gs->mission * 2;
                    for (int di = 0; di < 4; di++)
                        if (gs->droids[di].hp > 0) {
                            apply_droid_damage(&gs->droids[di], dmg);
                        }
                    char hmsg[64];
                    snprintf(hmsg, sizeof(hmsg), _("Industrial hazard! %d damage!"), dmg);
                    msg_push(hmsg, 0xFFFF8800);
                    if (all_droids_dead(gs)) {
                        lib_combat.active = false;
                        lib_in_combat = false;
                        gs->mode = STATE_GAMEOVER;
                        return;
                    }
                }
                if (lib_interact.fine_refused) {
                    lib_interact.fine_refused = false;
                    lib_bar_fight_pending = false;
                    lib_combat_generate_encounter(&lib_combat,
                        (uint16_t)(gs->tick * 0x5E5 + 17U), gs->mission);
                    lib_in_combat = true;
                } else if (lib_interact.bar_fight) {
                    lib_interact.bar_fight = false;
                    lib_bar_fight_pending = true;
                    lib_combat_generate_encounter(&lib_combat,
                        (uint16_t)(gs->tick * 0x5E5), gs->mission);
                    lib_in_combat = true;
                    gs->reputation -= 10;
                    if (gs->reputation < -100) gs->reputation = -100;
                    msg_push(_("Bar fight! Rep -10"), 0xFFFF4444);
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
                else if (count == 0 && lib_interact.shop_menu_active &&
                         (lib_interact.type == INTERACT_SHOP ||
                          lib_interact.type == INTERACT_BAR) &&
                         idx < lib_interact.shop.item_count) {
                    bool inventory_full = gs->lib_inventory_count < 0 ||
                        gs->lib_inventory_count >= 40 ||
                        lib_interact.purchased_count < 0 ||
                        lib_interact.purchased_count >= 20 ||
                        gs->lib_inventory_count + lib_interact.purchased_count >= 40;
                    if (inventory_full) {
                        msg_push(_("Shared inventory is full."), 0xFFFF8844);
                    } else if (building_interact_buy(&lib_interact, idx)) {
                        char buy_msg[96];
                        snprintf(buy_msg, sizeof(buy_msg),
                                 _("Purchased: %s"),
                                 lib_interact.shop.items[idx].name);
                        msg_push(buy_msg, 0xFF44FF44);
                    } else {
                        msg_push(_("Cannot buy that item."), 0xFFFF8844);
                    }
                }
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
                        lib_bar_fight_pending = false;
                        gs->reputation += 15;
                        if (gs->reputation > 100) gs->reputation = 100;
                        msg_push(_("Fine paid. Rep +15"), 0xFF44FF44);
                    }
                    if (lib_interact.industrial_hazard) {
                        lib_interact.industrial_hazard = false;
                        int dmg = 5 + gs->mission * 2;
                        for (int di = 0; di < 4; di++)
                            if (gs->droids[di].hp > 0) {
                                apply_droid_damage(&gs->droids[di], dmg);
                            }
                        char hmsg[64];
                        snprintf(hmsg, sizeof(hmsg), _("Industrial hazard! %d damage!"), dmg);
                        msg_push(hmsg, 0xFFFF8800);
                        if (all_droids_dead(gs)) {
                            lib_combat.active = false;
                            lib_in_combat = false;
                            gs->mode = STATE_GAMEOVER;
                            return;
                        }
                    }
                    if (lib_interact.fine_refused) {
                        lib_interact.fine_refused = false;
                        lib_bar_fight_pending = false;
                        lib_combat_generate_encounter(&lib_combat,
                            (uint16_t)(gs->tick * 0x5E5 + 17U), gs->mission);
                        lib_in_combat = true;
                    } else if (lib_interact.bar_fight) {
                        lib_interact.bar_fight = false;
                        lib_bar_fight_pending = true;
                        lib_combat_generate_encounter(&lib_combat,
                            (uint16_t)(gs->tick * 0x5E5), gs->mission);
                        lib_in_combat = true;
                        gs->reputation -= 10;
                        if (gs->reputation < -100) gs->reputation = -100;
                        msg_push(_("Bar fight! Rep -10"), 0xFFFF4444);
                    } else if (lib_interact.mission_complete) {
                        lib_interact.mission_complete = false;
                        lib_in_dungeon = true;
                        lib_dungeon_entry_x = lib_nav.cell_x;
                        lib_dungeon_entry_y = lib_nav.cell_y;
                        gs->current_level = 0;
                        map_generate_base(gs->levels, &gs->num_levels,
                                          gs->mission_seed + (uint32_t)gs->mission +
                                          (uint32_t)lib_interact.building_index * 7919);
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
                        combat_spawn_for_level_avoiding_party(
                            &creatures, &gs->levels[0], 0, gs->mission_seed, gs);
                        msg_push(_("Entered building interior"), 0xFF44AAFF);
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
                {
                    int combat_enemy_count = lib_combat.enemy_count;
                    if (combat_enemy_count < 0) combat_enemy_count = 0;
                    if (combat_enemy_count > LIB_COMBAT_MAX_ENEMIES)
                        combat_enemy_count = LIB_COMBAT_MAX_ENEMIES;
                    if (combat_enemy_count > 0) {
                    lib_combat.selected_target =
                        (lib_combat.selected_target + 1) % combat_enemy_count;
                    }
                }
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
            if (event->key.mod & SDL_KMOD_SHIFT)
                gs->mode = STATE_CITY_MAP;
            else
                gs->map_overlay = !gs->map_overlay;
            break;
        case SDLK_F:
        case SDLK_RETURN: {
            if (lib_nav.facing < CITY_DIR_NORTH || lib_nav.facing > CITY_DIR_WEST)
                break;
            int fwd_dx = (int[]){0,1,0,-1}[lib_nav.facing];
            int fwd_dy = (int[]){-1,0,1,0}[lib_nav.facing];
            int fx = lib_nav.cell_x + fwd_dx;
            int fy = lib_nav.cell_y + fwd_dy;
            if (custom_feat_ptr && custom_feat_ptr->fast_travel &&
                fx >= 0 && fx < 64 && fy >= 0 && fy < 64 &&
                city_nav_get_cell(&lib_grid, fx, fy) == 0x23 && gs->gold >= 50) {
                gs->gold -= 50;
                for (int ty = 0; ty < 64; ty++) {
                    for (int tx = 0; tx < 64; tx++) {
                        if (city_nav_get_cell(&lib_grid, tx, ty) != 0x0A) continue;
                        int off = ty * 64 + tx;
                        uint8_t raw_bid = lib_grid.building_ids[off];
                        if (raw_bid == 0 || raw_bid == 0xFF) continue;
                        uint8_t bid = raw_bid & 0x7F;
                        if (bid == 0 || lib_buildings.total_buildings == 0 ||
                            lib_buildings.total_buildings > CITYGEN_MAX_BUILDINGS)
                            continue;
                        int bg = (bid - 1) % lib_buildings.total_buildings;
                        if (bg >= 0 && bg < lib_buildings.total_buildings &&
                            lib_buildings.buildings[bg].type == 8) {
                            if (!city_nav_teleport(&lib_nav, tx, ty)) {
                                gs->gold += 50;
                                goto lib_interact_done;
                            }
                            taxi_flash_ttl = 15;
                            msg_push(_("Taxi: 50 gold"), 0xFFFFAA00);
                            goto lib_interact_done;
                        }
                    }
                }
                gs->gold += 50;
            } else if (city_nav_is_building_entrance(&lib_grid,
                    lib_nav.cell_x, lib_nav.cell_y)) {
                if (building_interact_enter(&lib_interact, &lib_grid,
                        &lib_buildings, lib_nav.cell_x, lib_nav.cell_y,
                        &gs->gold)) {
                    if (lib_interact.type == INTERACT_POLICE)
                        building_interact_set_bar_fight(&lib_interact,
                                                        lib_bar_fight_pending);
                    building_interact_set_reputation(&lib_interact, gs->reputation);
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
                sd[i].level = (uint8_t)xp_to_display_level(gs->droids[i].xp);
                sd[i].xp = gs->droids[i].xp;
                memcpy(sd[i].skills, gs->droids[i].skill_levels,
                       sizeof(sd[i].skills));
                for (int p = 0; p < 6; p++)
                    sd[i].equipment[p] = gs->droids[i].body_parts[p];
                for (int w = 0; w < 2; w++)
                    sd[i].equipment[6 + w] = gs->droids[i].weapons[w];
                memcpy(sd[i].inventory, gs->droids[i].items,
                       sizeof(sd[i].inventory));
                memcpy(sd[i].body_part_hp, gs->droids[i].body_part_hp,
                       sizeof(sd[i].body_part_hp));
            }
            uint16_t seed = (uint16_t)(gs->mission_seed & 0xFFFF);
            uint16_t seed_hi = (uint16_t)((gs->mission_seed >> 16) & 0xFFFF);
            uint32_t save_gold = gs->gold < 0 ? 0u : (uint32_t)gs->gold;
            lib_save_from_state(&save, seed_hi, seed,
                (uint16_t)gs->mission, (uint16_t)gs->mission,
                save_gold, gs->tick, &lib_nav, sd, 4);
            save.generators_destroyed = gs->generators_destroyed < 0 ? 0 :
                (gs->generators_destroyed > UINT16_MAX ? UINT16_MAX :
                 (uint16_t)gs->generators_destroyed);
            save.generators_total = gs->generators_total < 0 ? 0 :
                (gs->generators_total > UINT16_MAX ? UINT16_MAX :
                 (uint16_t)gs->generators_total);
            if (save.generators_destroyed > save.generators_total)
                save.generators_destroyed = save.generators_total;
            save.reputation = gs->reputation < -100 ? -100 :
                (gs->reputation > 100 ? 100 : (int16_t)gs->reputation);
            memcpy(save.mission_complete, gs->lib_mission_complete,
                   sizeof(save.mission_complete));
            save.shared_inventory_count = gs->lib_inventory_count < 0 ? 0 :
                (gs->lib_inventory_count > 40 ? 40 : gs->lib_inventory_count);
            for (int i = 0; i < save.shared_inventory_count; i++) {
                snprintf(save.shared_inventory[i].name,
                         sizeof(save.shared_inventory[i].name), "%s",
                         gs->lib_inventory[i].name);
                save.shared_inventory[i].item_type = gs->lib_inventory[i].item_type;
            }
            if (!lib_save_write(&save, "liberation.sav"))
                fprintf(stderr, "Could not write Liberation save: liberation.sav\n");
            break;
        }
        case SDLK_F9: {
            LibSaveData save;
            if (lib_save_read(&save, "liberation.sav")) {
                /* The save contains the mission seed, while the generated
                 * city is intentionally derived at runtime.  Rebuild that
                 * deterministic world before restoring the saved navigator;
                 * otherwise F9 could place a mission from one city into the
                 * buildings and plot state of the previous live session. */
                if (liberation_prototype_gameplay_enabled) {
                    gs->mission = (int)save.mission;
                    gs->mission_seed = ((uint32_t)save.seed_hi << 16) |
                                       save.seed_lo;
                    lib_city_generated = false;
                    start_liberation_session(gs);
                    liberation_intro_active = false;
                    liberation_mission_menu_active = false;
                    lib_mission_briefing = false;
                }
                restore_liberation_save_state(gs, &save);
            } else {
                fprintf(stderr, "Could not load Liberation save: liberation.sav\n");
            }
            break;
        }
        default: break;
    }
}

static void game_handle_input(GameState *gs, const SDL_Event *event) {
    if (!gs || !event || gs->current_level < 0 ||
        gs->current_level >= gs->num_levels || gs->num_levels > MAX_LEVELS ||
        gs->party_x < 0 || gs->party_x >= MAP_WIDTH ||
        gs->party_y < 0 || gs->party_y >= MAP_HEIGHT ||
        gs->party_dir < DIR_NORTH || gs->party_dir > DIR_WEST ||
        gs->selected_droid < 0 || gs->selected_droid >= 4) return;
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
                fire_flash_ttl = 3;
                if (creatures.creature_killed) {
                    sfx_play(&sfx, SFX_DEATH);
                    creature_death_flash_ttl = 5;
                    msg_push(_("Creature destroyed!"), 0xFF44AAFF);
                    creatures.creature_killed = false;
                }
                if (creatures.level_up_occurred) {
                    sfx_play(&sfx, SFX_LEVEL_UP);
                    msg_push(_("LEVEL UP!"), 0xFFFFFF00);
                    levelup_flash_ttl = 10;
                    creatures.level_up_occurred = false;
                }
                char atk_msg[64];
                snprintf(atk_msg, sizeof(atk_msg), _("Droid %d fires!"),
                         gs->selected_droid + 1);
                msg_push(atk_msg, 0xFF44FF44);
                if (all_droids_dead(gs)) gs->mode = STATE_GAMEOVER;
            }
            return;
        case SDLK_F: {
            const DungeonLevel *cur = &gs->levels[gs->current_level];
            // Check if on shop cell
            if (cur->cells[gs->party_y][gs->party_x].type == CELL_SHOP) {
                shop_return_mode = STATE_GAME;
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
            {
                bool has_clipboard = false;
                for (int di = 0; di < 4; di++)
                    for (int si = 0; si < 10; si++)
                        if (gs->droids[di].items[si] == 49) has_clipboard = true;
                if (has_clipboard) {
                    char hint[64];
                    if (puzzle_get_clipboard_hint(&puzzles, gs, tx, ty, face, hint, sizeof(hint)) ||
                        puzzle_get_clipboard_hint(&puzzles, gs, gs->party_x, gs->party_y, gs->party_dir, hint, sizeof(hint)))
                        msg_push(hint, 0xFF88CCFF);
                }
            }
            {
                int interaction_x = gs->party_x;
                int interaction_y = gs->party_y;
                int16_t en_before = gs->droids[gs->selected_droid].energy;
                bool puzzle_handled =
                    puzzle_interact(&puzzles, gs, gs->party_x, gs->party_y, gs->party_dir) ||
                    puzzle_interact(&puzzles, gs, tx, ty, face);
                if (combat_cell_occupied(&creatures, gs->current_level,
                                         gs->party_x, gs->party_y)) {
                    gs->party_x = interaction_x;
                    gs->party_y = interaction_y;
                }
                if (gs->droids[gs->selected_droid].energy > en_before)
                    recharge_flash_ttl = 6;
                if (puzzle_handled) sfx_play(&sfx, SFX_BUTTON);
                if (!puzzle_handled) {
                int gen_before = gs->generators_destroyed;
                CellType cell_before = CELL_WALL;
                if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT)
                    cell_before = gs->levels[gs->current_level].cells[ty][tx].type;
                combat_interact(gs, &item_db);
                if (gs->generators_destroyed > gen_before) {
                    sfx_play(&sfx, SFX_GENERATOR);
                    generator_flash_ttl = 12;
                }
                if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT) {
                    CellType cell_after = gs->levels[gs->current_level].cells[ty][tx].type;
                    if (cell_before == CELL_DOOR_LOCKED && cell_after == CELL_FLOOR) {
                        sfx_play(&sfx, SFX_DOOR_OPEN);
                        door_flash_ttl = 4;
                    } else if (cell_before == CELL_DOOR && cell_after == CELL_FLOOR) {
                        sfx_play(&sfx, SFX_DOOR_OPEN);
                        door_flash_ttl = 4;
                    }
                }
            }}
            return;
        }
        case SDLK_F5:
            if (custom_feat_ptr && custom_feat_ptr->quicksave) {
                char path[64];
                snprintf(path, sizeof(path), "opencaptive_slot%d.sav", quicksave_slot);
                if (!save_game(gs, &creatures, &puzzles, path))
                    fprintf(stderr, "Could not write Captive save: %s\n", path);
                if (custom_feat_ptr->cross_save) {
                    char cross_path[64];
                    snprintf(cross_path, sizeof(cross_path),
                             "opencaptive_slot%d.ocsv", quicksave_slot);
                    if (!cross_save_export(gs, cross_path))
                        fprintf(stderr, "Could not write cross-save: %s\n", cross_path);
                }
            } else {
                if (!save_game(gs, &creatures, &puzzles, "opencaptive.sav"))
                    fprintf(stderr, "Could not write Captive save: opencaptive.sav\n");
                if (custom_feat_ptr && custom_feat_ptr->cross_save)
                    if (!cross_save_export(gs, "opencaptive.ocsv"))
                        fprintf(stderr, "Could not write cross-save: opencaptive.ocsv\n");
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
                if (!load_game(gs, &creatures, &puzzles, path))
                    fprintf(stderr, "Could not load Captive save: %s\n", path);
            } else {
                if (!load_game(gs, &creatures, &puzzles, "opencaptive.sav"))
                    fprintf(stderr, "Could not load Captive save: opencaptive.sav\n");
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
            if (combat_change_floor_if_clear(gs, &creatures, 1))
                stair_flash_ttl = 6;
            return;
        case SDLK_COMMA: // < stairs up
            if (combat_change_floor_if_clear(gs, &creatures, -1))
                stair_flash_ttl = 6;
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
        if (cell != CELL_WALL && cell != CELL_DOOR && cell != CELL_DOOR_LOCKED &&
            !combat_cell_occupied(&creatures, gs->current_level, nx, ny)) {
            gs->party_x = nx;
            gs->party_y = ny;
            for (int di = 0; di < 4; di++) {
                if (gs->droids[di].hp > 0 && gs->droids[di].energy > 0)
                    gs->droids[di].energy--;
            }
            puzzle_check_step(&puzzles, gs, nx, ny);
            if (gs->mode == STATE_GAMEOVER) return;
            if (combat_cell_occupied(&creatures, gs->current_level,
                                     gs->party_x, gs->party_y)) {
                /* Teleporters have no creature-list dependency.  Reject a
                 * destination occupied by an active creature and leave the
                 * party on the safe tile that triggered the teleport. */
                gs->party_x = nx;
                gs->party_y = ny;
            }
            /* A teleporter may move the party while resolving the step.
             * All post-movement effects must use the actual landing cell,
             * not the cell that triggered the move. */
            int landed_x = gs->party_x;
            int landed_y = gs->party_y;
            CellType landed_cell =
                gs->levels[gs->current_level].cells[landed_y][landed_x].type;
            {
                MapCell *step_cell =
                    &gs->levels[gs->current_level].cells[landed_y][landed_x];
                if (step_cell->item_id > 0) {
                    Droid *d = &gs->droids[gs->selected_droid];
                    for (int si = 0; si < 10; si++) {
                        if (d->items[si] == 0) {
                            d->items[si] = step_cell->item_id;
                            char pickup_msg[64];
                            snprintf(pickup_msg, sizeof(pickup_msg),
                                     _("Droid %d picked up item"), gs->selected_droid + 1);
                            msg_push(pickup_msg, 0xFF44AAFF);
                            sfx_play(&sfx, SFX_PICKUP);
                            step_cell->item_id = 0;
                            break;
                        }
                    }
                }
            }
            if (landed_cell == CELL_PIT) {
                for (int di = 0; di < 4; di++) {
                    if (gs->droids[di].hp > 0) {
                        int dmg = 5 + (gs->current_level * 2);
                        apply_droid_damage(&gs->droids[di], dmg);
                    }
                }
                msg_push(_("Fell into a pit!"), 0xFFFF4444);
                sfx_play(&sfx, SFX_HIT);
                if (all_droids_dead(gs)) gs->mode = STATE_GAMEOVER;
            } else if (landed_cell == CELL_PRESSURE_PLATE) {
                msg_push(_("Click!"), 0xFFAAAA44);
                sfx_play(&sfx, SFX_DOOR_OPEN);
            }
            sfx_play(&sfx, SFX_STEP);
        } else if (cell == CELL_DOOR_LOCKED) {
            sfx_play(&sfx, SFX_DOOR_LOCKED);
        }
    }
}

int main(int argc, char *argv[]) {
    static char default_data_path[512];
    get_default_data_path(default_data_path, sizeof(default_data_path));

    OpenCaptiveConfig config = {
        .platform = CAPTIVE_PLATFORM_DOS,
        .render_mode = CAPTIVE_RENDER_ORIGINAL,
        .renderer_backend = 0,
        .data_path = default_data_path,
        .scale_factor = 3,
        .window_width = 1280,
        .window_height = 800,
        .vsync = true,
        .integer_scaling = true,
        .brightness = 50,
        .contrast = 50,
        .gamma = 50,
        .master_volume = 80,
        .fps_limit = 60,
    };
    CustomFeatures custom;
    custom_features_defaults(&custom);
    Automap automap_state;
    automap_init(&automap_state);
    ReplaySystem replay;
    replay_init(&replay);
    const char *replay_output_path = "opencaptive.ocrp";

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
    int compare_actual_rect[4] = {0};
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
                "  --data-dir <path>     Alias for --data\n"
                "  --enhanced            Enable enhanced 3D renderer\n"
                "  --platform <name>     Set playable Captive platform: dos\n"
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
                "  --lang <code>         Language: en, sv, de, fr, es, it, etc.\n"
                "  --scan-data           Scan and verify all supported game data\n"
                "  --scan-game-data      Alias for --scan-data\n\n"
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
                "  --hq-midi             Apply the enhanced MIDI output filter\n"
                "  --automap             Remember visited cells\n"
                "  --dynamic-lighting    Distance-based lighting\n"
                "  --speed <n>           Game speed multiplier (default 1.0)\n"
                "  --fast-travel         Enable fast travel in cities\n"
                "  --replay-record       Record inputs to opencaptive.ocrp\n"
                "  --replay-output <f>  Replay output file for recording\n"
                "  --replay-play <file>  Play back a recorded replay\n"
                "  --cross-save-export   Enable portable save export\n"
                "  --features-config <f> Load features from config file\n\n"
                "  --verify-data <name>  Verify data by SHA-256: captive, liberation, all\n\n"
                "  --capture-frame <ppm> Save one unscaled native game frame, then exit\n\n"
                "  --extract-dos-vga <dump> <ppm>  Extract a 320x200 DOS VGA reference frame\n\n"
                "  --compare-frames <expected> <actual>  Compare two native PPM frames\n"
                "  --compare-frames-rect <expected> <actual> <x> <y> <w> <h>\n"
                "                         Compare one native frame rectangle exactly\n\n"
                "  --compare-frames-regions <expected> <actual> <ex> <ey> <ax> <ay> <w> <h>\n"
                "                         Compare same-sized regions at independent offsets\n\n"
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
        } else if ((strcmp(argv[i], "--data") == 0 ||
                    strcmp(argv[i], "--data-dir") == 0) && i + 1 < argc) {
            config.data_path = argv[++i];
        } else if (strcmp(argv[i], "--enhanced") == 0) {
            config.render_mode = CAPTIVE_RENDER_ENHANCED;
        } else if (strcmp(argv[i], "--platform") == 0 && i + 1 < argc) {
            const char *platform = argv[++i];
            if (strcmp(platform, "dos") == 0) config.platform = CAPTIVE_PLATFORM_DOS;
            else {
                fprintf(stderr, "Unsupported Captive platform: %s (only dos is playable)\n",
                        platform);
                return 2;
            }
        } else if (strcmp(argv[i], "--scale") == 0 && i + 1 < argc) {
            if (!parse_int_option(argv[++i], 1, 5, &config.scale_factor)) {
                fprintf(stderr, "--scale must be an integer from 1 to 5\n");
                return 2;
            }
            /* Keep the command-line scale meaningful even though the
             * settings menu now supplies an explicit default window size.
             * A later --resolution still overrides this, as expected for
             * command-line options processed from left to right. */
            config.window_width = CAPTIVE_ORIGINAL_WIDTH * config.scale_factor;
            config.window_height = CAPTIVE_ORIGINAL_HEIGHT * config.scale_factor;
            if (config.window_width < 640) config.window_width = 640;
            if (config.window_height < 400) config.window_height = 400;
        } else if (strcmp(argv[i], "--resolution") == 0 && i + 1 < argc) {
            int rw = 0, rh = 0;
            if (parse_resolution_option(argv[++i], &rw, &rh)) {
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
            if (!parse_int_option(argv[++i], 2, 4, &custom.upscale_factor)) {
                fprintf(stderr, "--upscale-factor must be an integer from 2 to 4\n");
                return 2;
            }
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
        } else if (strcmp(argv[i], "--hq-midi") == 0) {
            custom.hq_midi = true;
        } else if (strcmp(argv[i], "--automap") == 0) {
            custom.automap = true;
        } else if (strcmp(argv[i], "--dynamic-lighting") == 0) {
            custom.dynamic_lighting = true;
        } else if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc) {
            if (!parse_float_option(argv[++i], &custom.game_speed)) {
                fprintf(stderr, "--speed must be a finite positive number\n");
                return 2;
            }
            custom.speed_control = true;
        } else if (strcmp(argv[i], "--fast-travel") == 0) {
            custom.fast_travel = true;
            custom.speed_control = true;
        } else if (strcmp(argv[i], "--replay-record") == 0) {
            custom.replay_record = true;
            replay.recording = true;
        } else if (strcmp(argv[i], "--replay-output") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "--replay-output requires a file path\n");
                return 2;
            }
            replay_output_path = argv[++i];
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
            custom.hq_midi = true;
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
        } else if (strcmp(argv[i], "--scan-data") == 0 ||
                   strcmp(argv[i], "--scan-game-data") == 0) {
            verify_data = "all";
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
            memcpy(compare_actual_rect, compare_rect, sizeof(compare_rect));
            compare_rect_set = true;
        } else if (strcmp(argv[i], "--compare-frames-regions") == 0 && i + 8 < argc) {
            expected_frame_path = argv[++i];
            actual_frame_path = argv[++i];
            int values[6] = {0};
            for (int field = 0; field < 6; ++field) {
                if (!parse_int_option(argv[++i], field < 4 ? 0 : 1, INT_MAX,
                                      &values[field])) {
                    fprintf(stderr, "Comparison regions must use integer x y x y width height\n");
                    return 2;
                }
            }
            compare_rect[0] = values[0]; compare_rect[1] = values[1];
            compare_actual_rect[0] = values[2]; compare_actual_rect[1] = values[3];
            compare_rect[2] = compare_actual_rect[2] = values[4];
            compare_rect[3] = compare_actual_rect[3] = values[5];
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
                                  compare_rect_set ? compare_rect : NULL,
                                  compare_rect_set ? compare_actual_rect : NULL);

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
#if defined(__APPLE__) && TARGET_OS_OSX
    macos_localize_menus();
#endif

    // State
    StartMenu menu = {0};
    sync_menu_from_config(&menu, &config, &custom, true, true);
    liberation_texture_filter = custom.texture_filter;
    liberation_dynamic_lighting = custom.dynamic_lighting;

    /* GameState owns every generated dungeon level.  Keeping it automatic
     * here can exhaust the default Windows main-thread stack before the
     * renderer has even started. */
    static GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    gs.config = config;

    // Virtual filesystem
    DataVFS vfs;
    vfs_init(&vfs, config.data_path);

    {
        struct stat st;
        if (config.data_path && stat(config.data_path, &st) != 0) {
            menu.show_setup_popup = true;
        }
    }

    // Audio
    sound_init(&sound_sys, (uint32_t)custom.audio_sample_rate);
    sound_set_reverb(&sound_sys, custom.audio_reverb, custom.reverb_amount);
    music_init(&music_sys, &sound_sys, &vfs, custom.audio_sample_rate,
               custom.hq_midi);
    apply_menu_audio(&menu);

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
        } else if (!textures_loaded) {
            /* The startup hash set is a fast identity gate.  The renderer
             * still needs every decoded PL5 surface; do not enter the game
             * with a verified but unusable partial atlas. */
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
            printf("Starting Captive original presentation (dungeon compositor pending)\n");
        }
    } else if (start_directly && requested_game == GAME_LIBERATION) {
        if (!liberation_data_open(&liberation_data, &vfs)) {
            show_missing_liberation_data_dialog(config.data_path);
        } else {
            load_liberation_mission_menu();
            lib_city_generated = false;
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

            if (custom.replay_record && gs.game_type == GAME_CAPTIVE &&
                gs.mode == STATE_GAME && event.type == SDL_EVENT_KEY_DOWN) {
                uint8_t action;
                if (replay_encode_key(event.key.key, &action))
                    replay_record_input(&replay, gs.tick, action, 0);
            }

            if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                event.key.key == SDLK_F10 &&
                gs.mode == STATE_GAME) {
                runtime_popup.open = !runtime_popup.open;
                continue;
            }
            if (runtime_popup.open) {
                popup_handle_event(&gs, &config, &renderer, &custom, &event);
                /* A runtime option may change the game mode (for example
                 * COMPLETE OBJECTIVE enters the Captive holomap).  The
                 * popup is only rendered in STATE_GAME; leaving it marked
                 * open here would create an invisible modal that swallowed
                 * every subsequent event until F10/Escape was pressed. */
                if (gs.mode != STATE_GAME)
                    runtime_popup.open = false;
                continue;
            }

            switch (gs.mode) {
                case STATE_MENU: {
                    MenuResult result;
                    if (event.type == SDL_EVENT_MOUSE_MOTION) {
                        float mx, my;
                        if (window_to_canvas(renderer.window, event.motion.x,
                                             event.motion.y, MENU_WIDTH,
                                             MENU_HEIGHT, &mx, &my))
                            start_menu_handle_mouse_motion(&menu, mx, my);
                        result = MENU_RESULT_NONE;
                    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                        event.button.button == SDL_BUTTON_LEFT) {
                        float x, y;
                        result = window_to_canvas(renderer.window, event.button.x,
                                                  event.button.y, MENU_WIDTH,
                                                  MENU_HEIGHT, &x, &y)
                            ? start_menu_handle_click(&menu, x, y)
                            : MENU_RESULT_NONE;
                    } else {
                        result = start_menu_handle_event(&menu, &event);
                    }
                    switch (result) {
                        case MENU_RESULT_START_CAPTIVE:
                            /* Starting a new session must not inherit the
                             * previous dungeon after returning from pause.
                             * Continue uses load_game() below and therefore
                             * deliberately does not take this path. */
                            game_state_init(&gs, GAME_CAPTIVE, 1);
                            combat_init(&creatures);
                            puzzle_init(&puzzles);
                            gs.game_type = GAME_CAPTIVE;
                            apply_menu_config(&config, &menu, &custom);
                            gs.config = config;
                            renderer_set_effects(&renderer, config.bilinear,
                                                 config.scanlines,
                                                 config.crt_curvature,
                                                 config.brightness,
                                                 config.contrast, config.gamma);
                            renderer_apply_display(&renderer, &config);
                            apply_menu_audio(&menu);
                            vfs_free(&vfs);
                            vfs_init(&vfs, config.data_path);
                            if (!validate_data_path(&vfs)) {
                                show_missing_data_dialog(config.data_path);
                                break;
                            }
                            textures_loaded = reload_captive_assets(&atlas, &vfs, &hud_bg);
                            if (!textures_loaded) {
                                show_missing_data_dialog(config.data_path);
                                break;
                            }
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
                            /* A fresh Liberation game must not inherit the
                             * previous Captive/Liberation session. Continue
                             * uses the save-specific path below. */
                            game_state_init(&gs, GAME_LIBERATION, 1);
                            combat_init(&creatures);
                            puzzle_init(&puzzles);
                            apply_menu_config(&config, &menu, &custom);
                            gs.config = config;
                            renderer_set_effects(&renderer, config.bilinear,
                                                 config.scanlines,
                                                 config.crt_curvature,
                                                 config.brightness,
                                                 config.contrast, config.gamma);
                            renderer_apply_display(&renderer, &config);
                            apply_menu_audio(&menu);
                            vfs_free(&vfs);
                            vfs_init(&vfs, config.data_path);
                            liberation_data_close(&liberation_data);
                            if (!liberation_data_open_source(&liberation_data, &vfs,
                                                             (LiberationSource)menu.liberation_source_choice)) {
                                show_missing_liberation_data_dialog(config.data_path);
                                break;
                            }
                            load_liberation_mission_menu();
                            lib_city_generated = false;
                            start_liberation_session(&gs);
                            break;
                        case MENU_RESULT_CONTINUE_CAPTIVE:
                            gs.game_type = GAME_CAPTIVE;
                            apply_menu_config(&config, &menu, &custom);
                            gs.config = config;
                            renderer_set_effects(&renderer, config.bilinear,
                                                 config.scanlines,
                                                 config.crt_curvature,
                                                 config.brightness,
                                                 config.contrast, config.gamma);
                            renderer_apply_display(&renderer, &config);
                            apply_menu_audio(&menu);
                            vfs_free(&vfs);
                            vfs_init(&vfs, config.data_path);
                            if (!validate_data_path(&vfs)) {
                                show_missing_data_dialog(config.data_path);
                                break;
                            }
                            textures_loaded = reload_captive_assets(&atlas, &vfs, &hud_bg);
                            if (!textures_loaded) {
                                show_missing_data_dialog(config.data_path);
                                break;
                            }
                            {
                                char spath[64];
                                bool loaded = false;
                                snprintf(spath, sizeof(spath), "opencaptive_slot%d.sav", quicksave_slot);
                                loaded = load_game(&gs, &creatures, &puzzles, spath);
                                if (!loaded) loaded = load_game(&gs, &creatures, &puzzles, "opencaptive.sav");
                                if (loaded) {
                                    music_play(&music_sys, MUSIC_BASE);
                                    gs.mode = STATE_GAME;
                                } else {
                                    /* The menu may have seen a save file
                                     * that was removed or became invalid
                                     * before loading. Do not let the droid
                                     * configuration continue with a stale
                                     * dungeon from the previous session. */
                                    game_state_init(&gs, GAME_CAPTIVE, 1);
                                    gs.config = config;
                                    combat_init(&creatures);
                                    puzzle_init(&puzzles);
                                    gs.mode = STATE_DROID_CONFIG;
                                    droid_config_cursor = 0;
                                }
                            }
                            break;
                        case MENU_RESULT_CONTINUE_LIBERATION:
                            apply_menu_config(&config, &menu, &custom);
                            gs.config = config;
                            renderer_set_effects(&renderer, config.bilinear,
                                                 config.scanlines,
                                                 config.crt_curvature,
                                                 config.brightness,
                                                 config.contrast, config.gamma);
                            renderer_apply_display(&renderer, &config);
                            apply_menu_audio(&menu);
                            vfs_free(&vfs);
                            vfs_init(&vfs, config.data_path);
                            liberation_data_close(&liberation_data);
                            if (!liberation_data_open(&liberation_data, &vfs)) {
                                show_missing_liberation_data_dialog(config.data_path);
                                break;
                            }
                            load_liberation_mission_menu();
                            {
                                LibSaveData save;
                                if (lib_save_read(&save, "liberation.sav")) {
                                    gs.mission = (int)save.mission;
                                    gs.mission_seed =
                                        ((uint32_t)save.seed_hi << 16) |
                                        save.seed_lo;
                                    lib_city_generated = false;
                                    start_liberation_session(&gs);
                                    restore_liberation_save_state(&gs, &save);
                                } else {
                                    /* Do not continue with a stale session if
                                     * the advertised save disappeared or is
                                     * corrupt between menu scan and load. */
                                    OpenCaptiveConfig preserved_config = gs.config;
                                    game_state_init(&gs, GAME_LIBERATION, 1);
                                    gs.config = preserved_config;
                                    combat_init(&creatures);
                                    puzzle_init(&puzzles);
                                    lib_city_generated = false;
                                    start_liberation_session(&gs);
                                }
                            }
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
                        if (intro_loaded) {
                            anm_free(&intro_anim);
                            intro_loaded = false;
                        }
                        intro_frame = 0;
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
                            } else if (event.key.key == SDLK_RETURN) {
                                if (gs.game_type == GAME_CAPTIVE && gs.num_levels == 0) {
                                    if (custom.replay_playback && replay.seed != 0)
                                        game_state_new_mission_seeded(&gs, gs.mission, replay.seed);
                                    else
                                        game_state_new_mission(&gs, gs.mission);
                                    generate_captive_puzzles(&gs);
                                    generate_captive_encounters(&gs);
                                }
                                gs.mode = STATE_GAME;
                            }
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
                            float canvas_x, canvas_y;
                            if (!window_to_canvas(renderer.window, event.button.x,
                                                  event.button.y,
                                                  LIBERATION_SCREEN_WIDTH,
                                                  LIBERATION_SCREEN_HEIGHT,
                                                  &canvas_x, &canvas_y))
                                break;
                            int x = (int)canvas_x;
                            int y = (int)canvas_y;
                            int local_y = y - LIBERATION_MISSION_MENU_Y;
                            if (x >= 89 && x < 233 &&
                                local_y >= 89 && local_y < liberation_mission_menu_height)
                                liberation_mission_menu_active = false;
                        } else if (lib_mission_briefing &&
                                   event.type == SDL_EVENT_KEY_DOWN &&
                                   event.key.key == SDLK_RETURN) {
                            lib_mission_briefing = false;
                        } else if (liberation_prototype_gameplay_enabled &&
                                   !liberation_intro_active && !liberation_mission_menu_active &&
                                   !lib_mission_briefing && lib_city_generated) {
                            if (lib_in_dungeon) {
                                if (event.type == SDL_EVENT_KEY_DOWN &&
                                    event.key.key == SDLK_ESCAPE) {
                                    lib_in_dungeon = false;
                                    msg_push(_("Left building"), 0xFF44AAFF);
                                } else {
                                    game_handle_input(&gs, &event);
                                    popup_apply_cheats(&gs);
                                    if (gs.generators_destroyed >= gs.generators_total &&
                                        gs.generators_total > 0) {
                                        lib_in_dungeon = false;
                                        if (gs.mission > 0 && gs.mission <= 256)
                                            gs.lib_mission_complete[(gs.mission - 1) / 8] |=
                                                (uint8_t)(1U << ((gs.mission - 1) % 8));
                                        gs.reputation += 20;
                                        if (gs.reputation > 100) gs.reputation = 100;
                                        msg_push(_("Mission complete. Rep +20"), 0xFF44FF44);
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
                                popup_apply_cheats(&gs);
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
                        popup_apply_cheats(&gs);
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
                                        uint8_t item_id;
                                        if (!liberation_runtime_item_id(
                                                gs.lib_inventory[lib_inv_cursor].item_type,
                                                &item_id))
                                            break;
                                        for (int si = 0; si < 10; si++) {
                                            if (d->items[si] == 0) {
                                                d->items[si] = item_id;
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
                                case SDLK_1: case SDLK_2:
                                case SDLK_3: case SDLK_4: {
                                    int droid_idx = (int)(event.key.key - SDLK_1);
                                    gs.selected_droid = droid_idx;
                                    droid_ui_init(&droid_ui, droid_idx);
                                    break;
                                }
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
                                gs.mode = shop_return_mode;
                                shop_return_mode = STATE_GAME;
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
                                generate_captive_puzzles(&gs);
                                generate_captive_encounters(&gs);
                                gs.mode = STATE_GAME;
                                music_play(&music_sys, MUSIC_BASE);
                                break;
                            case SDLK_S:
                                shop_return_mode = STATE_HOLAMAP;
                                gs.mode = STATE_SHOP;
                                shop_init(&shop, &item_db, gs.mission, gs.mission_seed);
                                shop.gold = gs.gold;
                                break;
                            case SDLK_ESCAPE:
                                gs.mode = STATE_MENU;
                                sync_menu_from_config(&menu, &config, &custom,
                                                      music_sys.enabled,
                                                      sound_sys.enabled);
                                music_stop(&music_sys);
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
                        sync_menu_from_config(&menu, &config, &custom,
                                              music_sys.enabled,
                                              sound_sys.enabled);
                        music_stop(&music_sys);
                    }
                    break;
                case STATE_HELP:
                    if (event.type == SDL_EVENT_KEY_DOWN) {
                        gs.mode = STATE_GAME;
                    }
                    break;
                case STATE_CITY_MAP:
                    if (event.type == SDL_EVENT_KEY_DOWN) {
                        gs.mode = STATE_GAME;
                    }
                    break;
                case STATE_PAUSE:
                    if (event.type == SDL_EVENT_MOUSE_MOTION) {
                        int canvas_height = (gs.game_type == GAME_LIBERATION && !lib_in_dungeon)
                            ? LIBERATION_SCREEN_HEIGHT : CAPTIVE_ORIGINAL_HEIGHT;
                        float canvas_x, my;
                        if (!window_to_canvas(renderer.window, event.motion.x,
                                              event.motion.y,
                                              CAPTIVE_ORIGINAL_WIDTH,
                                              canvas_height, &canvas_x, &my))
                            break;
                        for (int i = 0; i < 3; i++) {
                            int iy = 90 + i * 20;
                            if (my >= iy - 4 && my < iy + 14) {
                                pause_cursor = i;
                                break;
                            }
                        }
                    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                               event.button.button == SDL_BUTTON_LEFT) {
                        int canvas_height = (gs.game_type == GAME_LIBERATION && !lib_in_dungeon)
                            ? LIBERATION_SCREEN_HEIGHT : CAPTIVE_ORIGINAL_HEIGHT;
                        float canvas_x, my;
                        if (!window_to_canvas(renderer.window, event.button.x,
                                              event.button.y,
                                              CAPTIVE_ORIGINAL_WIDTH,
                                              canvas_height, &canvas_x, &my))
                            break;
                        int clicked = -1;
                        for (int i = 0; i < 3; i++) {
                            int iy = 90 + i * 20;
                            if (my >= iy - 4 && my < iy + 14) {
                                clicked = i;
                                break;
                            }
                        }
                        if (clicked >= 0) {
                            pause_cursor = clicked;
                            goto pause_activate;
                        }
                    } else if (event.type == SDL_EVENT_KEY_DOWN) {
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
                            pause_activate:
                                if (pause_cursor == 0) {
                                    gs.mode = STATE_GAME;
                                    gs.paused = false;
                                } else if (pause_cursor == 1) {
                                    gs.mode = STATE_MENU;
                                    sync_menu_from_config(&menu, &config, &custom,
                                                          music_sys.enabled,
                                                          sound_sys.enabled);
                                    /* SETTINGS must enter the settings view;
                                     * otherwise it was indistinguishable from
                                     * QUIT and silently discarded the user's
                                     * selection. */
                                    menu.in_settings = true;
                                    menu.settings_cursor = 0;
                                    music_stop(&music_sys);
                                    gs.paused = false;
                                } else if (pause_cursor == 2) {
                                    gs.mode = STATE_MENU;
                                    sync_menu_from_config(&menu, &config, &custom,
                                                          music_sys.enabled,
                                                          sound_sys.enabled);
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
        } else {
            bool liberation_canvas = false;
            if (gs.game_type == GAME_LIBERATION) {
                switch (gs.mode) {
                    case STATE_GAME:
                    case STATE_PAUSE:
                    case STATE_HELP:
                        liberation_canvas = !lib_in_dungeon;
                        break;
                    case STATE_INVENTORY:
                    case STATE_CITY_MAP:
                        liberation_canvas = true;
                        break;
                    default:
                        break;
                }
            }
            if (liberation_canvas) {
                frame_width = LIBERATION_SCREEN_WIDTH;
                frame_height = LIBERATION_SCREEN_HEIGHT;
            } else {
                frame_width = CAPTIVE_ORIGINAL_WIDTH;
                frame_height = CAPTIVE_ORIGINAL_HEIGHT;
            }
        }
        bool renderer_ready = true;
        if (renderer.canvas_width != frame_width || renderer.canvas_height != frame_height)
            renderer_ready = renderer_set_canvas(&renderer, frame_width, frame_height,
                                                 &config);
        if (renderer_ready)
            renderer_ready = renderer_set_upscale(
                &renderer,
                custom.hd_upscale && frame_width <= 640 && frame_height <= 400,
                custom.upscale_factor);
        if (renderer_ready)
            renderer_ready = renderer_set_widescreen(
                &renderer,
                custom.widescreen && frame_width <= 640 && frame_height <= 400,
                custom.widescreen_width);
        if (!renderer_ready) {
            fprintf(stderr, "Could not resize renderer framebuffer: %s\n",
                    SDL_GetError());
            exit_status = 1;
            break;
        }

        memset(framebuffer, 0, (size_t)frame_width * frame_height * sizeof(uint32_t));

        if (gs.mode == STATE_MENU)
            start_menu_update(&menu);

        switch (gs.mode) {
            case STATE_MENU:
                start_menu_render(&menu, framebuffer, MENU_WIDTH, MENU_HEIGHT);
                break;

            case STATE_INTRO:
                if (!intro_loaded || intro_anim.frame_count <= 0) {
                    /* A corrupt or empty ANM must not leave the launcher on
                     * a permanent black intro screen.  Treat it like a
                     * missing intro and continue to droid configuration. */
                    if (intro_loaded) {
                        anm_free(&intro_anim);
                        intro_loaded = false;
                    }
                    music_play(&music_sys, MUSIC_BASE);
                    gs.mode = STATE_DROID_CONFIG;
                    droid_config_cursor = 0;
                    break;
                }
                if (intro_loaded && intro_anim.frame_count > 0) {
                    uint32_t now = SDL_GetTicks();
                    uint32_t elapsed = now - intro_last_tick;
                    if (elapsed >= 100U) {
                        uint32_t advance = elapsed / 100U;
                        uint32_t remaining = (uint32_t)intro_anim.frame_count -
                                             (uint32_t)intro_frame;
                        if (advance >= remaining) {
                            anm_free(&intro_anim);
                            intro_loaded = false;
                            music_play(&music_sys, MUSIC_BASE);
                            gs.mode = STATE_DROID_CONFIG;
                            droid_config_cursor = 0;
                            break;
                        }
                        intro_frame += (int)advance;
                        /* Preserve the original cadence instead of dropping
                         * all elapsed time when a render frame was delayed. */
                        intro_last_tick += advance * 100U;
                    }
                    // Convert indexed frame to ARGB
                    const uint8_t *frame = intro_anim.frames[intro_frame];
                    for (int i = 0; i < CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT; i++) {
                        framebuffer[i] = intro_anim.palette[frame[i]];
                    }
                }
                break;

            case STATE_GAME: {
                /* Cheats stay live while the F10 overlay is open as well;
                 * changing a toggle takes effect on the very next frame. */
                popup_apply_cheats(&gs);
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
                    } else if (liberation_prototype_gameplay_enabled &&
                               lib_mission_briefing && lib_city_generated) {
                        draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                      LIBERATION_SCREEN_HEIGHT, 30,
                                      _("MISSION BRIEFING"), 0xFF44FF44, 2);
                        char line[128];
                        snprintf(line, sizeof(line), _("City: %s"), lib_plot.city_name);
                        draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                      LIBERATION_SCREEN_HEIGHT, 70, line, 0xFFFFFF44, 1);
                        snprintf(line, sizeof(line), _("Find: %s (%s)"),
                                 lib_plot.victim_name, lib_plot.victim_title);
                        draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                      LIBERATION_SCREEN_HEIGHT, 90, line, 0xFFFFAAAA, 1);
                        snprintf(line, sizeof(line), _("Source: %s"), lib_plot.news_source);
                        draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                      LIBERATION_SCREEN_HEIGHT, 110, line, 0xFF8888CC, 1);
                        snprintf(line, sizeof(line), _("%d buildings in city"),
                                 lib_plot.num_buildings);
                        draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                      LIBERATION_SCREEN_HEIGHT, 140, line, 0xFFAAAAAA, 1);
                        draw_centered(framebuffer, LIBERATION_SCREEN_WIDTH,
                                      LIBERATION_SCREEN_HEIGHT, 180,
                                      _("PRESS ENTER TO BEGIN"), 0xFF888888, 1);
                    } else if (liberation_prototype_gameplay_enabled && lib_city_generated) {
                        uint32_t now = SDL_GetTicks();
                        float dt = (now - gs.last_frame_ms) / 1000.0f;
                        if (dt > 0.1f) dt = 0.1f;
                        gs.last_frame_ms = now;
                        /* Liberation uses the shared game tick for its
                         * day/night clock and encounter seed. Captive's
                         * tick is advanced in its game branch below, so
                         * advance it here once per city frame as well. */
                        gs.tick++;
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
                                          _("TAXI"), 0xFFFFFF00, 3);
                        }
                        char pos_str[64];
                        snprintf(pos_str, sizeof(pos_str), _("%s (%d,%d) %s"),
                            lib_plot.city_name[0] ? lib_plot.city_name : lib_buildings.city_name,
                            lib_nav.cell_x, lib_nav.cell_y,
                            city_nav_is_building_entrance(&lib_grid,
                                lib_nav.cell_x, lib_nav.cell_y) ? _("[ENTER]") : "");
                        draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                            LIBERATION_SCREEN_HEIGHT, 2, LIBERATION_SCREEN_HEIGHT - 10,
                            pos_str, 0xFFFFFFFF, 1);
                        if (lib_in_combat) {
                            for (int y = 0; y < LIBERATION_SCREEN_HEIGHT; y++)
                                for (int x = 0; x < LIBERATION_SCREEN_WIDTH; x++)
                                    framebuffer[y * LIBERATION_SCREEN_WIDTH + x] =
                                        (framebuffer[y * LIBERATION_SCREEN_WIDTH + x] >> 1) & 0x7F7F7F;
                            draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                LIBERATION_SCREEN_HEIGHT, 8, 8, _("COMBAT"), 0xFFFF0000, 1);
                            int combat_enemy_count = lib_combat.enemy_count;
                            if (combat_enemy_count < 0) combat_enemy_count = 0;
                            if (combat_enemy_count > LIB_COMBAT_MAX_ENEMIES)
                                combat_enemy_count = LIB_COMBAT_MAX_ENEMIES;
                            for (int ei = 0; ei < combat_enemy_count; ei++) {
                                LibCombatEnemy *e = &lib_combat.enemies[ei];
                                char line[80];
                                snprintf(line, sizeof(line), _("%s%s HP:%d/%d DMG:%d"),
                                    ei == lib_combat.selected_target ? ">" : " ",
                                    e->name, e->hp, e->hp_max, e->damage);
                                uint32_t color = e->alive ? 0xFFFFFFFF : 0xFF666666;
                                draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                    LIBERATION_SCREEN_HEIGHT, 8, 22 + ei * 10, line, color, 1);
                            }
                            draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                LIBERATION_SCREEN_HEIGHT, 8, LIBERATION_SCREEN_HEIGHT - 30,
                                _("1-4:Attack TAB:Target ESC:Flee"), 0xFFCCCCCC, 1);
                            if (lib_combat_is_over(&lib_combat, &gs)) {
                                draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                    LIBERATION_SCREEN_HEIGHT, 8, LIBERATION_SCREEN_HEIGHT / 2,
                                    lib_combat_player_won(&lib_combat) ? _("VICTORY!") : _("DEFEATED"),
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
                                // NPC type indicator (portrait substitute)
                                {
                                    const char *npc_icons[] = {"[?]","[S]","[B]","[C]","[L]","[P]","[R]","[H]","[I]","[!]"};
                                    int ti = lib_interact.type;
                                    if (ti < 0 || ti > 9) ti = 0;
                                    draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                        LIBERATION_SCREEN_HEIGHT,
                                        LIBERATION_SCREEN_WIDTH - 30,
                                        LIBERATION_SCREEN_HEIGHT / 2 + 4,
                                        npc_icons[ti], 0xFFFFAA00, 2);
                                }
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
                                if ((lib_interact.type == INTERACT_SHOP || lib_interact.type == INTERACT_BAR) &&
                                    lib_interact.shop.item_count > 0) {
                                    char shop_info[80];
                                    snprintf(shop_info, sizeof(shop_info),
                                        _("%d items available. Gold: %d"),
                                        lib_interact.shop.item_count, gs.gold);
                                    draw_simple_text(framebuffer, LIBERATION_SCREEN_WIDTH,
                                        LIBERATION_SCREEN_HEIGHT, 8,
                                        LIBERATION_SCREEN_HEIGHT - 12,
                                        shop_info, 0xFFAAAA00, 1);
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
                                      _("VERIFIED LIBERATION PRESENTATION DATA REQUIRED"),
                                      0xFFCCDDEE, 1);
                    }
                } else {
                    /* Captive game tick: energy regen every ~5 seconds */
                    if (custom.replay_playback && gs.game_type == GAME_CAPTIVE &&
                        gs.mode == STATE_GAME) {
                        const ReplayInput *input;
                        while ((input = replay_next(&replay, gs.tick)) != NULL) {
                            SDL_Event replay_event;
                            memset(&replay_event, 0, sizeof(replay_event));
                            replay_event.type = SDL_EVENT_KEY_DOWN;
                            replay_event.key.key = replay_decode_key(input->action);
                            if (replay_event.key.key != SDLK_UNKNOWN)
                                game_handle_input(&gs, &replay_event);
                            popup_apply_cheats(&gs);
                        }
                    }
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
                            snprintf(msg, sizeof(msg), _("Droid %d hit for %d!"),
                                     ti + 1,
                                     creatures.last_attack_damage);
                            msg_push(msg, 0xFFFF4444);
                            damage_flash_ttl = 8;
                            sfx_play(&sfx, SFX_HIT);
                            if (ti >= 0 && ti < 4 && gs.droids[ti].hp <= 0) {
                                char dmsg[64];
                                snprintf(dmsg, sizeof(dmsg), _("Droid %d destroyed!"), ti + 1);
                                msg_push(dmsg, 0xFFFF0000);
                                sfx_play(&sfx, SFX_DEATH);
                                int alive = 0;
                                for (int di = 0; di < 4; di++)
                                    if (gs.droids[di].hp > 0) alive++;
                                if (alive == 0) {
                                    msg_push(_("All droids destroyed! Mission failed."), 0xFFFF0000);
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
                    }
                    CaptiveViewWindow view_window;
                    captive_view_window_build(&gs, &view_window);
                    viewport_render(&view_window, &atlas, framebuffer,
                                    CAPTIVE_ORIGINAL_WIDTH,
                                    CAPTIVE_ORIGINAL_HEIGHT);
                    viewport_render_creatures(&gs, &creatures, &atlas,
                                              framebuffer,
                                              CAPTIVE_ORIGINAL_WIDTH,
                                              CAPTIVE_ORIGINAL_HEIGHT);
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
                                  _("INVENTORY"), 0xFF44AAFF, 2);
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
                                  _("SHARED ITEMS"), 0xFFAAAA44, 1);
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
                              60, _("GAME OVER"), 0xFFFF2222, 3);
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              100, _("ALL DROIDS DESTROYED"), 0xFFAAAAAA, 1);
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              130, _("PRESS ESCAPE"), 0xFF888888, 1);
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
                              40, _("VICTORY!"), 0xFF44FF44, 3);
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              80, _("ALL MISSIONS COMPLETE"), 0xFFFFFF44, 2);
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              120, _("YOU HAVE ESCAPED!"), 0xFFAAAAFF, 1);
                draw_centered(framebuffer, CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT,
                              150, _("PRESS ESCAPE"), 0xFF888888, 1);
                break;

            case STATE_HELP: {
                int pw = (gs.game_type == GAME_LIBERATION && !lib_in_dungeon)
                    ? LIBERATION_SCREEN_WIDTH : CAPTIVE_ORIGINAL_WIDTH;
                int ph = (gs.game_type == GAME_LIBERATION && !lib_in_dungeon)
                    ? LIBERATION_SCREEN_HEIGHT : CAPTIVE_ORIGINAL_HEIGHT;
                memset(framebuffer, 0, (size_t)pw * ph * sizeof(uint32_t));
                draw_centered(framebuffer, pw, ph, 5, _("CONTROLS"), 0xFFFFFF44, 2);
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
                int pw = (gs.game_type == GAME_LIBERATION && !lib_in_dungeon)
                    ? LIBERATION_SCREEN_WIDTH : CAPTIVE_ORIGINAL_WIDTH;
                int ph = (gs.game_type == GAME_LIBERATION && !lib_in_dungeon)
                    ? LIBERATION_SCREEN_HEIGHT : CAPTIVE_ORIGINAL_HEIGHT;
                for (int i = 0; i < pw * ph; i++)
                    framebuffer[i] = (framebuffer[i] & 0xFF000000)
                        | ((framebuffer[i] & 0xFEFEFE) >> 1);
                draw_centered(framebuffer, pw, ph, 40, _("PAUSED"), 0xFFFFFFFF, 3);
                const char *opts[] = {_("RESUME"), _("SETTINGS"), _("QUIT")};
                for (int i = 0; i < 3; i++) {
                    uint32_t c = (i == pause_cursor) ? 0xFFFFFF44 : 0xFF888888;
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%s %s",
                             (i == pause_cursor) ? ">" : " ", opts[i]);
                    draw_centered(framebuffer, pw, ph, 90 + i * 20, buf, c, 1);
                }
                draw_centered(framebuffer, pw, ph, ph - 20,
                              _("ESC: RESUME"), 0xFF666666, 1);
                break;
            }

            case STATE_DROID_CONFIG: {
                int cw = CAPTIVE_ORIGINAL_WIDTH;
                int ch = CAPTIVE_ORIGINAL_HEIGHT;
                for (int i = 0; i < cw * ch; i++)
                    framebuffer[i] = 0xFF000000;
                draw_centered(framebuffer, cw, ch, 10, _("DROID CONFIGURATION"), 0xFF00FF00, 2);
                for (int d = 0; d < 4; d++) {
                    /* Keep the fourth droid's condition row clear of the
                     * bottom action hints. The previous 35-pixel stride put
                     * that row at y=162, directly under the rename hint at
                     * y=165 on the native 320x200 canvas. */
                    int yy = 35 + d * 28;
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
                    draw_simple_text(framebuffer, cw, ch, 10, yy + 10, parts, 0xFF666666, 1);
                }
                if (droid_config_renaming) {
                    char ren[48];
                    snprintf(ren, sizeof(ren), "RENAME: %s_", gs.droids[droid_config_cursor].name);
                    draw_centered(framebuffer, cw, ch, ch - 35, ren, 0xFFFFFF00, 1);
                } else {
                    draw_simple_text(framebuffer, cw, ch, 10, ch - 35,
                                     "R:RENAME  S:SWAP WEAPONS", 0xFF666666, 1);
                }
                draw_centered(framebuffer, cw, ch, ch - 20, _("ENTER: START MISSION"), 0xFF00CC00, 1);
                break;
            }

            case STATE_CITY_MAP: {
                int mw = LIBERATION_SCREEN_WIDTH;
                int mh = LIBERATION_SCREEN_HEIGHT;
                for (int i = 0; i < mw * mh; i++)
                    framebuffer[i] = 0xFF000000;
                draw_centered(framebuffer, mw, mh, 5, _("CITY MAP"), 0xFF00FF00, 2);
                int scale = 3;
                int ox = (mw - 64 * scale) / 2;
                int oy = 30;
                for (int gy = 0; gy < 64; gy++) {
                    for (int gx = 0; gx < 64; gx++) {
                        uint8_t cell = lib_grid.plane0[gy * 64 + gx];
                        uint8_t raw_bid = lib_grid.building_ids[gy * 64 + gx];
                        uint8_t bid = raw_bid & 0x7F;
                        uint32_t col;
                        if (gx == lib_nav.cell_x && gy == lib_nav.cell_y)
                            col = 0xFFFFFF00;
                        else if (raw_bid != 0 && raw_bid != 0xFF && bid != 0)
                            col = 0xFF4444AA;
                        else if (cell == 0x0A)
                            col = 0xFF333333;
                        else if (cell == 0x00)
                            col = 0xFF666666;
                        else
                            col = 0xFF222222;
                        for (int sy = 0; sy < scale; sy++)
                            for (int sx = 0; sx < scale; sx++) {
                                int px = ox + gx * scale + sx;
                                int py = oy + gy * scale + sy;
                                if (px >= 0 && px < mw && py >= 0 && py < mh)
                                    framebuffer[py * mw + px] = col;
                            }
                    }
                }
                draw_centered(framebuffer, mw, mh, mh - 15, "ANY KEY: RETURN", 0xFF666666, 1);
                break;
            }

            default: break;
        }

        if (gs.mode == STATE_GAME) {
            if (gs.current_level >= 0 && gs.current_level < gs.num_levels &&
                gs.num_levels <= MAX_LEVELS &&
                custom.automap)
                automap_mark(&automap_state, gs.current_level, gs.party_x, gs.party_y);

        if (custom.minimap && gs.current_level >= 0 &&
                gs.current_level < gs.num_levels && gs.num_levels <= MAX_LEVELS) {
                const DungeonLevel *mlvl = &gs.levels[gs.current_level];
                minimap_render(framebuffer, frame_width, frame_height,
                               mlvl, gs.party_x, gs.party_y, gs.party_dir,
                               custom.reveal_map ? NULL :
                               (custom.automap ? automap_state.visited : NULL),
                               &custom);
            }

            if (custom.debug_hud)
                debug_hud_render(framebuffer, frame_width, frame_height, &gs, &custom);

            if (custom.replay_record)
                replay.seed = gs.mission_seed;
        }

        if (runtime_popup.open && gs.mode == STATE_GAME)
            popup_render(&gs, &custom, framebuffer, frame_width, frame_height);

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

        /* A capture is a headless one-frame operation.  Do not enter the
         * audio mixer or SDL presentation path after the file is complete:
         * dummy drivers and renderer backends are not required to support a
         * second frame, and the caller expects the process to exit now. */
        if (!running && capture_frame_path) break;

        music_update(&music_sys);
        sfx_update(&sfx);
        sound_mix(&sound_sys);
        renderer_present(&renderer, framebuffer);
        if (config.fps_limit > 0 || custom.speed_control) {
            uint64_t elapsed = SDL_GetTicks() - frame_started;
            double speed = (custom.speed_control && isfinite(custom.game_speed) &&
                            custom.game_speed > 0.0f)
                ? (double)custom.game_speed : 1.0;
            /* Speed control needs a stable reference even when the normal
             * FPS limiter is disabled.  Use the game's standard 60 FPS
             * cadence in that case. */
            int reference_fps = config.fps_limit > 0 ? config.fps_limit : 60;
            uint64_t frame_budget = (uint64_t)(1000.0 /
                ((double)reference_fps * speed));
            if (frame_budget == 0) frame_budget = 1;
            if (elapsed < frame_budget) SDL_Delay((uint32_t)(frame_budget - elapsed));
        }
    }

    vfs_free(&vfs);
    if (custom.replay_record && !replay_save(&replay, replay_output_path)) {
        fprintf(stderr, "Could not write replay: %s\n", replay_output_path);
        exit_status = 1;
    }
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
