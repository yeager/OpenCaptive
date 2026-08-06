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
    uint32_t tiny_pixels[16 * 16];
    start_menu_free(NULL);
    start_menu_render(NULL, NULL, 0, 0);
    start_menu_init(&menu);
    assert(menu.music_enabled && menu.sfx_enabled);

    /* Re-entry preserves the user's path and remains valid for a menu that
     * has already initialized its resources. */
    snprintf(menu.data_path, sizeof(menu.data_path), "/tmp/opencaptive-data");
    start_menu_reinit(&menu);
    assert(strcmp(menu.data_path, "/tmp/opencaptive-data") == 0);
    assert(menu.data_path_cursor == (int)strlen(menu.data_path));

    /* Small targets must not make the fixed-size popups write out of bounds. */
    menu.show_setup_popup = true;
    start_menu_render(&menu, tiny_pixels, 16, 16);
    menu.show_setup_popup = false;
    menu.show_version_popup = true;
    start_menu_render(&menu, tiny_pixels, 16, 16);
    menu.show_version_popup = false;
    menu.in_scanner = true;
    menu.scanner_done = false;
    menu.scanner_total = 1;
    menu.scanner_progress = 1;
    start_menu_render(&menu, tiny_pixels, 16, 16);
    menu.in_scanner = false;

    menu.captive_data_ok = true;
    menu.liberation_data_ok = true;
    menu.captive_source_mask = 7U;
    menu.liberation_source_mask = 7U;
    start_menu_check_data(&menu, "/path/that/does/not/exist");
    assert(!menu.captive_data_ok && !menu.liberation_data_ok);
    assert(menu.captive_source_mask == 0U && menu.liberation_source_mask == 0U);

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

    /* A Captive version popup must return the selected platform. */
    menu.show_version_popup = true;
    menu.version_popup_game = GAME_CAPTIVE;
    menu.version_popup_selection = 1;
    event = key_event(SDLK_RETURN);
    assert(start_menu_handle_event(&menu, &event) == MENU_RESULT_START_CAPTIVE);
    assert(menu.platform == CAPTIVE_PLATFORM_AMIGA);

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
