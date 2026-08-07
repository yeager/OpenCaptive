#ifndef CUSTOM_FEATURES_H
#define CUSTOM_FEATURES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool hd_upscale;
    int  upscale_factor;        // 2 = 2x, 3 = 3x, 4 = 4x

    bool widescreen;
    int  widescreen_width;      // viewport width override (0 = original)

    bool quicksave;             // F5/F9 multi-slot quicksave

    bool minimap;               // persistent minimap overlay
    float minimap_opacity;      // 0.0 - 1.0
    int  minimap_size;          // pixels, side length

    bool speed_control;
    float game_speed;           // 1.0 = normal, 0.5 = half, 2.0 = double
    bool fast_travel;

    bool mouse_look;            // FPS-style mouse control
    float mouse_sensitivity;    // 1.0 = default

    bool debug_hud;             // show cell info, PRNG, creature stats

    bool audio_reverb;
    float reverb_amount;        // 0.0 - 1.0
    bool hq_midi;               // filtered high-quality MIDI output
    int  audio_sample_rate;     // 22050, 44100, 48000

    bool automap;               // reveal visited cells permanently
    bool reveal_map;            // cheat: show every cell on the minimap
    bool cross_save;            // portable save format

    bool replay_record;
    bool replay_playback;

    bool texture_filter;        // bilinear on wall textures
    bool dynamic_lighting;      // per-face shading based on distance/normal
} CustomFeatures;

void custom_features_defaults(CustomFeatures *f);
bool custom_features_load(CustomFeatures *f, const char *path);
bool custom_features_save(const CustomFeatures *f, const char *path);

// --- HD Upscale (xBRZ) ---

void upscale_xbrz_2x(const uint32_t *src, int src_w, int src_h,
                      uint32_t *dst);
void upscale_xbrz_3x(const uint32_t *src, int src_w, int src_h,
                      uint32_t *dst);
void upscale_xbrz_4x(const uint32_t *src, int src_w, int src_h,
                      uint32_t *dst);

// --- Minimap ---

void minimap_render(uint32_t *fb, int fb_w, int fb_h,
                    const void *level, int party_x, int party_y,
                    int party_dir, const bool *visited,
                    const CustomFeatures *feat);

// --- Debug HUD ---

void debug_hud_render(uint32_t *fb, int fb_w, int fb_h,
                      const void *game_state, const CustomFeatures *feat);

// --- Automap ---

#define AUTOMAP_MAX_CELLS (64 * 32 * 15)

typedef struct {
    bool visited[AUTOMAP_MAX_CELLS];
} Automap;

void automap_init(Automap *am);
void automap_mark(Automap *am, int level, int x, int y);
bool automap_is_visited(const Automap *am, int level, int x, int y);

// --- Audio Reverb ---

void reverb_process(int16_t *buffer, int num_samples, float amount);
void reverb_reset(void);

// --- Replay ---

#define REPLAY_MAX_INPUTS 65536

typedef struct {
    uint32_t tick;
    uint8_t  action;
    uint8_t  param;
} ReplayInput;

typedef struct {
    ReplayInput inputs[REPLAY_MAX_INPUTS];
    int         count;
    int         playback_pos;
    uint32_t    seed;
    bool        recording;
    bool        playing;
} ReplaySystem;

void replay_init(ReplaySystem *rs);
void replay_record_input(ReplaySystem *rs, uint32_t tick, uint8_t action, uint8_t param);
bool replay_save(const ReplaySystem *rs, const char *path);
bool replay_load(ReplaySystem *rs, const char *path);
const ReplayInput *replay_next(ReplaySystem *rs, uint32_t tick);

// --- Dynamic Lighting ---

uint32_t lighting_shade_pixel(uint32_t color, float intensity);
float lighting_compute_intensity(int distance, int normal_dot);

// --- Cross-save ---

bool cross_save_export(const void *game_state, const char *path);
bool cross_save_import(void *game_state, const char *path);

#endif
