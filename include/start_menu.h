#ifndef START_MENU_H
#define START_MENU_H

#include "game_state.h"
#include "renderer.h"
#include <SDL3/SDL.h>

typedef enum {
    MENU_RESULT_NONE,
    MENU_RESULT_START_CAPTIVE,
    MENU_RESULT_START_LIBERATION,
    MENU_RESULT_QUIT,
} MenuResult;

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
} StartMenu;

void start_menu_init(StartMenu *menu);
MenuResult start_menu_handle_event(StartMenu *menu, const SDL_Event *event);
void start_menu_render(StartMenu *menu, uint32_t *pixels, int width, int height);

#endif
