#include "start_menu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static SDL_Event key_event(SDL_Keycode key) {
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = key;
    return event;
}

int main(void) {
    StartMenu menu = {0};
    start_menu_init(&menu);
    assert(menu.music_enabled && menu.sfx_enabled);

    menu.in_settings = true;
    menu.settings_cursor = 12; /* SFX */
    SDL_Event event = key_event(SDLK_RETURN);
    assert(start_menu_handle_event(&menu, &event) == MENU_RESULT_NONE);
    assert(!menu.sfx_enabled);
    assert(menu.music_enabled);

    menu.settings_cursor = 14; /* Back */
    event = key_event(SDLK_RETURN);
    assert(start_menu_handle_event(&menu, &event) == MENU_RESULT_NONE);
    assert(!menu.in_settings);

    assert(start_menu_handle_click(&menu, 100.0f, 85.0f) ==
           MENU_RESULT_START_CAPTIVE);
    assert(start_menu_handle_click(&menu, 100.0f, 105.0f) ==
           MENU_RESULT_START_LIBERATION);
    assert(start_menu_handle_click(&menu, 100.0f, 125.0f) == MENU_RESULT_NONE);
    assert(menu.in_settings);

    puts("All start menu tests passed");
    return 0;
}
