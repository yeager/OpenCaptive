#ifndef START_MENU_H
#define START_MENU_H

#include "game_state.h"
#include "renderer.h"
#include "data_vfs.h"
#include <SDL3/SDL.h>

#define MENU_WIDTH  960
#define MENU_HEIGHT 600

typedef enum {
    MENU_RESULT_NONE,
    MENU_RESULT_START_CAPTIVE,
    MENU_RESULT_START_LIBERATION,
    MENU_RESULT_CONTINUE_CAPTIVE,
    MENU_RESULT_CONTINUE_LIBERATION,
    MENU_RESULT_QUIT,
} MenuResult;

typedef struct TTF_Font TTF_Font;

typedef struct {
    int selected_item;    // 0=Captive, 1=Liberation, 2=Settings, 3=Quit
    int num_items;
    bool enhanced_mode;
    bool music_enabled;
    bool sfx_enabled;
    int scale_factor;     // 1-5
    bool scale_custom;    // true when SCALE was explicitly selected
    bool anim_clock_initialized;
    uint32_t anim_last_ms;
    uint32_t anim_accumulator_ms;
    bool fullscreen;
    bool vsync;
    bool scanlines;
    bool crt_curvature;
    bool bilinear;
    bool integer_scaling;
    int fps_limit;        // 0,30,60,120
    int brightness;       // 0-100
    int contrast;         // 0-100
    CaptivePlatform platform;
    uint32_t anim_tick;
    bool in_settings;
    int settings_cursor;
    int settings_scroll;
    char data_path[512];
    bool data_path_editing;
    int data_path_cursor;
    int lang_index;
    int renderer_backend;  // 0=auto, 1=gpu, 2=software
    int window_size;       // 0=960x600, 1=1280x800, 2=1600x1000, 3=1920x1200
    int gamma;             // 0-100
    int master_volume;     // 0-100
    int audio_sample_rate; // 0=22050, 1=44100, 2=48000
    bool audio_reverb;
    int game_speed;        // 0=0.5x, 1=1x, 2=2x
    int mouse_sensitivity; // 1-10
    bool captive_data_ok;
    bool liberation_data_ok;
    bool captive_save_exists;
    /* Most recently modified numbered Captive save slot, or -1 when only
     * the legacy opencaptive.sav file is available. */
    int captive_save_slot;
    bool liberation_save_exists;
    bool in_about;
    bool in_controls;
    bool in_scanner;
    int scanner_captive_found;
    int scanner_captive_total;
    int scanner_liberation_found;
    int scanner_liberation_total;
    int scanner_zip_count;
    int scanner_progress;
    int scanner_total;
    int scanner_phase;
    bool scanner_done;
    bool scanner_background;
    bool scanner_vfs_ready;
    size_t scanner_captive_index;
    DataVFS scanner_vfs;
    bool show_setup_popup;
    bool show_version_popup;
    int version_popup_game;
    int version_popup_selection;
    unsigned captive_source_mask;
    CaptivePlatform captive_source_choice;
    unsigned liberation_source_mask;
    int liberation_source_choice;
    uint32_t *logo_img;
    int logo_img_w, logo_img_h;
    uint32_t *captive_img;
    int captive_img_w, captive_img_h;
    uint32_t *liberation_img;
    int liberation_img_w, liberation_img_h;
    TTF_Font *font_title;
    TTF_Font *font_body;
    TTF_Font *font_small;
    bool ttf_ready;
} StartMenu;

void start_menu_init(StartMenu *menu);
/* Re-enter the menu while retaining loaded artwork, fonts, and data path. */
void start_menu_reinit(StartMenu *menu);
void start_menu_free(StartMenu *menu);
void start_menu_check_data(StartMenu *menu, const char *data_path);
/* Start a cached, frame-bounded scan.  Explicit scans show progress; a
 * background scan keeps the start menu responsive while refreshing status. */
void start_menu_start_scan(StartMenu *menu, const char *data_path,
                           bool show_progress);
void start_menu_check_saves(StartMenu *menu);
/* Advance one bounded scanner operation without blocking menu rendering. */
void start_menu_update(StartMenu *menu);
MenuResult start_menu_handle_event(StartMenu *menu, const SDL_Event *event);
MenuResult start_menu_handle_click(StartMenu *menu, float x, float y);
void start_menu_handle_mouse_motion(StartMenu *menu, float x, float y);
void start_menu_render(StartMenu *menu, uint32_t *pixels, int width, int height);

#endif
