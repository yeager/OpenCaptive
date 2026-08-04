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
    menu.settings_cursor = 15; /* SFX */
    SDL_Event event = key_event(SDLK_RETURN);
    assert(start_menu_handle_event(&menu, &event) == MENU_RESULT_NONE);
    assert(!menu.sfx_enabled);
    assert(menu.music_enabled);

    menu.settings_cursor = 23; /* Back */
    event = key_event(SDLK_RETURN);
    assert(start_menu_handle_event(&menu, &event) == MENU_RESULT_NONE);
    assert(!menu.in_settings);

    /* Keyboard navigation: item 0 = Captive card */
    menu.selected_item = 0;
    event = key_event(SDLK_RETURN);
    assert(start_menu_handle_event(&menu, &event) == MENU_RESULT_START_CAPTIVE);

    /* item 1 = Liberation card */
    menu.selected_item = 1;
    event = key_event(SDLK_RETURN);
    assert(start_menu_handle_event(&menu, &event) == MENU_RESULT_START_LIBERATION);

    /* item 4 = Settings */
    menu.selected_item = 4;
    event = key_event(SDLK_RETURN);
    assert(start_menu_handle_event(&menu, &event) == MENU_RESULT_NONE);
    assert(menu.in_settings);
    menu.in_settings = false;

    /* item 5 = About */
    menu.selected_item = 5;
    event = key_event(SDLK_RETURN);
    assert(start_menu_handle_event(&menu, &event) == MENU_RESULT_NONE);
    assert(menu.in_about);
    event = key_event(SDLK_ESCAPE);
    start_menu_handle_event(&menu, &event);
    assert(!menu.in_about);

    /* item 6 = Controls */
    menu.selected_item = 6;
    event = key_event(SDLK_RETURN);
    assert(start_menu_handle_event(&menu, &event) == MENU_RESULT_NONE);
    assert(menu.in_controls);
    event = key_event(SDLK_ESCAPE);
    start_menu_handle_event(&menu, &event);
    assert(!menu.in_controls);

    /* item 7 = Quit */
    menu.selected_item = 7;
    event = key_event(SDLK_RETURN);
    assert(start_menu_handle_event(&menu, &event) == MENU_RESULT_QUIT);

    /* Continue only works when save exists */
    menu.selected_item = 2;
    menu.captive_save_exists = false;
    event = key_event(SDLK_RETURN);
    assert(start_menu_handle_event(&menu, &event) == MENU_RESULT_NONE);
    menu.captive_save_exists = true;
    assert(start_menu_handle_event(&menu, &event) == MENU_RESULT_CONTINUE_CAPTIVE);

    puts("All start menu tests passed");
    return 0;
}
