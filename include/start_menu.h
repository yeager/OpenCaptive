#ifndef START_MENU_H
#define START_MENU_H

#include "game_state.h"
#include "renderer.h"
#include <SDL3/SDL.h>

#define MENU_WIDTH  960
#define MENU_HEIGHT 600

typedef enum {
    MENU_RESULT_NONE,
    MENU_RESULT_START_CAPTIVE,
    MENU_RESULT_START_LIBERATION,
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
void start_menu_free(StartMenu *menu);
MenuResult start_menu_handle_event(StartMenu *menu, const SDL_Event *event);
MenuResult start_menu_handle_click(StartMenu *menu, float x, float y);
void start_menu_render(StartMenu *menu, uint32_t *pixels, int width, int height);

#endif
