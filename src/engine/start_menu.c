#include "start_menu.h"
#include "opencaptive.h"
#include <string.h>
#include <stdio.h>

// Simple 5x7 bitmap font for menu text
static const uint8_t font_5x7[][7] = {
    ['A'] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},
    ['B'] = {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    ['C'] = {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
    ['D'] = {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},
    ['E'] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
    ['F'] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
    ['G'] = {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},
    ['H'] = {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    ['I'] = {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
    ['J'] = {0x07,0x02,0x02,0x02,0x02,0x12,0x0C},
    ['K'] = {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
    ['L'] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    ['M'] = {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},
    ['N'] = {0x11,0x11,0x19,0x15,0x13,0x11,0x11},
    ['O'] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
    ['P'] = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    ['Q'] = {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},
    ['R'] = {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    ['S'] = {0x0E,0x11,0x10,0x0E,0x01,0x11,0x0E},
    ['T'] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    ['U'] = {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
    ['V'] = {0x11,0x11,0x11,0x11,0x0A,0x0A,0x04},
    ['W'] = {0x11,0x11,0x11,0x15,0x15,0x1B,0x11},
    ['X'] = {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
    ['Y'] = {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},
    ['Z'] = {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
    ['0'] = {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
    ['1'] = {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    ['2'] = {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F},
    ['3'] = {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E},
    ['4'] = {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
    ['5'] = {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    ['6'] = {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
    ['7'] = {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    ['8'] = {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    ['9'] = {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
    [':'] = {0x00,0x04,0x04,0x00,0x04,0x04,0x00},
    [' '] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['-'] = {0x00,0x00,0x00,0x1F,0x00,0x00,0x00},
    ['.'] = {0x00,0x00,0x00,0x00,0x00,0x00,0x04},
    ['('] = {0x02,0x04,0x08,0x08,0x08,0x04,0x02},
    [')'] = {0x08,0x04,0x02,0x02,0x02,0x04,0x08},
};

static void draw_char(uint32_t *pixels, int pw, int ph,
                      int x, int y, char c, uint32_t color, int scale) {
    if (c < 0 || (unsigned char)c >= sizeof(font_5x7)/sizeof(font_5x7[0])) return;
    const uint8_t *glyph = font_5x7[(unsigned char)c];
    for (int gy = 0; gy < 7; gy++) {
        for (int gx = 0; gx < 5; gx++) {
            if (glyph[gy] & (0x10 >> gx)) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        int px = x + gx * scale + sx;
                        int py = y + gy * scale + sy;
                        if (px >= 0 && px < pw && py >= 0 && py < ph)
                            pixels[py * pw + px] = color;
                    }
                }
            }
        }
    }
}

static void draw_text(uint32_t *pixels, int pw, int ph,
                      int x, int y, const char *text, uint32_t color, int scale) {
    for (int i = 0; text[i]; i++) {
        draw_char(pixels, pw, ph, x + i * 6 * scale, y, text[i], color, scale);
    }
}

static void draw_text_centered(uint32_t *pixels, int pw, int ph,
                               int y, const char *text, uint32_t color, int scale) {
    int len = (int)strlen(text);
    int tw = len * 6 * scale;
    draw_text(pixels, pw, ph, (pw - tw) / 2, y, text, color, scale);
}

static void draw_rect(uint32_t *pixels, int pw, int ph,
                      int x, int y, int w, int h, uint32_t color) {
    for (int ry = y; ry < y + h && ry < ph; ry++) {
        if (ry < 0) continue;
        for (int rx = x; rx < x + w && rx < pw; rx++) {
            if (rx < 0) continue;
            pixels[ry * pw + rx] = color;
        }
    }
}

static void draw_border(uint32_t *pixels, int pw, int ph,
                        int x, int y, int w, int h, uint32_t color, int t) {
    draw_rect(pixels, pw, ph, x, y, w, t, color);
    draw_rect(pixels, pw, ph, x, y + h - t, w, t, color);
    draw_rect(pixels, pw, ph, x, y, t, h, color);
    draw_rect(pixels, pw, ph, x + w - t, y, t, h, color);
}

void start_menu_init(StartMenu *menu) {
    char saved_path[512];
    memcpy(saved_path, menu->data_path, sizeof(saved_path));
    int saved_cursor = menu->data_path_cursor;
    memset(menu, 0, sizeof(*menu));
    memcpy(menu->data_path, saved_path, sizeof(menu->data_path));
    menu->data_path_cursor = saved_cursor ? saved_cursor : (int)strlen(menu->data_path);
    menu->num_items = 4;
    menu->platform = CAPTIVE_PLATFORM_DOS;
    menu->music_enabled = true;
    menu->sfx_enabled = true;
    menu->scale_factor = 3;
    menu->vsync = true;
    menu->integer_scaling = true;
    menu->brightness = 50;
    menu->contrast = 50;
    menu->fps_limit = 60;
}

MenuResult start_menu_handle_event(StartMenu *menu, const SDL_Event *event) {
    // Handle text input for data path editing
    if (menu->data_path_editing) {
        if (event->type == SDL_EVENT_TEXT_INPUT) {
            int len = (int)strlen(menu->data_path);
            const char *text = event->text.text;
            int tlen = (int)strlen(text);
            if (len + tlen < 510) {
                memmove(&menu->data_path[menu->data_path_cursor + tlen],
                        &menu->data_path[menu->data_path_cursor],
                        len - menu->data_path_cursor + 1);
                memcpy(&menu->data_path[menu->data_path_cursor], text, tlen);
                menu->data_path_cursor += tlen;
            }
            return MENU_RESULT_NONE;
        }
        if (event->type != SDL_EVENT_KEY_DOWN) return MENU_RESULT_NONE;
        switch (event->key.key) {
            case SDLK_BACKSPACE:
                if (menu->data_path_cursor > 0) {
                    int len = (int)strlen(menu->data_path);
                    memmove(&menu->data_path[menu->data_path_cursor - 1],
                            &menu->data_path[menu->data_path_cursor],
                            len - menu->data_path_cursor + 1);
                    menu->data_path_cursor--;
                }
                break;
            case SDLK_DELETE: {
                int len = (int)strlen(menu->data_path);
                if (menu->data_path_cursor < len) {
                    memmove(&menu->data_path[menu->data_path_cursor],
                            &menu->data_path[menu->data_path_cursor + 1],
                            len - menu->data_path_cursor);
                }
                break;
            }
            case SDLK_LEFT:
                if (menu->data_path_cursor > 0) menu->data_path_cursor--;
                break;
            case SDLK_RIGHT:
                if (menu->data_path_cursor < (int)strlen(menu->data_path))
                    menu->data_path_cursor++;
                break;
            case SDLK_HOME:
                menu->data_path_cursor = 0;
                break;
            case SDLK_END:
                menu->data_path_cursor = (int)strlen(menu->data_path);
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
            case SDLK_ESCAPE:
                menu->data_path_editing = false;
                SDL_StopTextInput(NULL);
                break;
            default: break;
        }
        return MENU_RESULT_NONE;
    }

    if (event->type != SDL_EVENT_KEY_DOWN) return MENU_RESULT_NONE;

    if (menu->in_settings) {
        #define SETTINGS_COUNT 14
        switch (event->key.key) {
            case SDLK_UP:
                if (menu->settings_cursor > 0) menu->settings_cursor--;
                break;
            case SDLK_DOWN:
                if (menu->settings_cursor < SETTINGS_COUNT - 1) menu->settings_cursor++;
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
            case SDLK_LEFT:
            case SDLK_RIGHT:
                switch (menu->settings_cursor) {
                    case 0: menu->enhanced_mode = !menu->enhanced_mode; break;
                    case 1: menu->scanlines = !menu->scanlines; break;
                    case 2: menu->crt_curvature = !menu->crt_curvature; break;
                    case 3: menu->bilinear = !menu->bilinear; break;
                    case 4: menu->integer_scaling = !menu->integer_scaling; break;
                    case 5:
                        if (event->key.key == SDLK_RIGHT && menu->scale_factor < 5)
                            menu->scale_factor++;
                        else if (event->key.key == SDLK_LEFT && menu->scale_factor > 1)
                            menu->scale_factor--;
                        break;
                    case 6: menu->fullscreen = !menu->fullscreen; break;
                    case 7: menu->vsync = !menu->vsync; break;
                    case 8: {
                        int fps_vals[] = {0, 30, 60, 120};
                        int cur = 2;
                        for (int i = 0; i < 4; i++)
                            if (fps_vals[i] == menu->fps_limit) cur = i;
                        if (event->key.key == SDLK_RIGHT) cur = (cur + 1) % 4;
                        else if (event->key.key == SDLK_LEFT) cur = (cur + 3) % 4;
                        menu->fps_limit = fps_vals[cur];
                        break;
                    }
                    case 9:
                        if (event->key.key == SDLK_RIGHT && menu->brightness < 100)
                            menu->brightness += 5;
                        else if (event->key.key == SDLK_LEFT && menu->brightness > 0)
                            menu->brightness -= 5;
                        break;
                    case 10:
                        if (event->key.key == SDLK_RIGHT && menu->contrast < 100)
                            menu->contrast += 5;
                        else if (event->key.key == SDLK_LEFT && menu->contrast > 0)
                            menu->contrast -= 5;
                        break;
                    case 11: menu->music_enabled = !menu->music_enabled; break;
                    case 12:
                        if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
                            menu->data_path_editing = true;
                            menu->data_path_cursor = (int)strlen(menu->data_path);
                            SDL_StartTextInput(NULL);
                        }
                        break;
                    case 13: menu->in_settings = false; break;
                }
                break;
            case SDLK_ESCAPE:
                menu->in_settings = false;
                break;
        }
        // Keep cursor visible by scrolling
        int visible = 8;
        if (menu->settings_cursor < menu->settings_scroll)
            menu->settings_scroll = menu->settings_cursor;
        if (menu->settings_cursor >= menu->settings_scroll + visible)
            menu->settings_scroll = menu->settings_cursor - visible + 1;
        return MENU_RESULT_NONE;
    }

    switch (event->key.key) {
        case SDLK_UP:
            menu->selected_item = (menu->selected_item + menu->num_items - 1) % menu->num_items;
            break;
        case SDLK_DOWN:
            menu->selected_item = (menu->selected_item + 1) % menu->num_items;
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            switch (menu->selected_item) {
                case 0: return MENU_RESULT_START_CAPTIVE;
                case 1: return MENU_RESULT_START_LIBERATION;
                case 2: menu->in_settings = true; menu->settings_cursor = 0; break;
                case 3: return MENU_RESULT_QUIT;
            }
            break;
        case SDLK_ESCAPE:
            return MENU_RESULT_QUIT;
    }
    return MENU_RESULT_NONE;
}

static void render_settings(StartMenu *menu, uint32_t *pixels, int width, int height) {
    draw_text_centered(pixels, width, height, 8, "OPENCAPTIVE", 0xFFFF8800, 2);
    draw_text_centered(pixels, width, height, 26, "BY DANIEL NYLANDER", 0xFF666688, 1);
    draw_text_centered(pixels, width, height, 38, "SETTINGS", 0xFF888888, 1);

    const char *labels[] = {
        "RENDERER:",
        "SCANLINES:",
        "CRT CURVE:",
        "BILINEAR:",
        "INT SCALE:",
        "SCALE:",
        "FULLSCREEN:",
        "VSYNC:",
        "FPS LIMIT:",
        "BRIGHTNESS:",
        "CONTRAST:",
        "MUSIC:",
        "DATA PATH:",
        "BACK",
    };
    char values[SETTINGS_COUNT][20];
    snprintf(values[0], 20, "%s", menu->enhanced_mode ? "ENHANCED" : "ORIGINAL");
    snprintf(values[1], 20, "%s", menu->scanlines ? "ON" : "OFF");
    snprintf(values[2], 20, "%s", menu->crt_curvature ? "ON" : "OFF");
    snprintf(values[3], 20, "%s", menu->bilinear ? "ON" : "OFF");
    snprintf(values[4], 20, "%s", menu->integer_scaling ? "ON" : "OFF");
    snprintf(values[5], 20, "%dX", menu->scale_factor);
    snprintf(values[6], 20, "%s", menu->fullscreen ? "ON" : "OFF");
    snprintf(values[7], 20, "%s", menu->vsync ? "ON" : "OFF");
    snprintf(values[8], 20, "%s", menu->fps_limit == 0 ? "UNLIMITED" :
             (menu->fps_limit == 30 ? "30" : (menu->fps_limit == 60 ? "60" : "120")));
    snprintf(values[9], 20, "%d%%", menu->brightness);
    snprintf(values[10], 20, "%d%%", menu->contrast);
    snprintf(values[11], 20, "%s", menu->music_enabled ? "ON" : "OFF");
    values[12][0] = '\0';
    values[13][0] = '\0';

    int menu_y = 50;
    int item_h = 14;
    int visible = 8;

    for (int vi = 0; vi < visible && vi + menu->settings_scroll < SETTINGS_COUNT; vi++) {
        int i = vi + menu->settings_scroll;
        int y = menu_y + vi * item_h;
        bool sel = (i == menu->settings_cursor);

        if (sel) {
            draw_rect(pixels, width, height, 30, y - 1, width - 60, item_h - 2, 0xFF333366);
            int pulse = (menu->anim_tick / 8) % 2;
            draw_text(pixels, width, height, 32, y, ">", pulse ? 0xFFFFFF00 : 0xFFFF8800, 1);
        }

        uint32_t color = sel ? 0xFFFFFFFF : 0xFFAAAAAA;
        draw_text(pixels, width, height, 42, y, labels[i], color, 1);
        if (values[i][0]) {
            uint32_t val_col = sel ? 0xFFFFFF00 : 0xFF88AA88;
            draw_text(pixels, width, height, 180, y, values[i], val_col, 1);
        }
    }

    // Scroll indicators
    if (menu->settings_scroll > 0)
        draw_text_centered(pixels, width, height, menu_y - 10, "...", 0xFF555555, 1);
    if (menu->settings_scroll + visible < SETTINGS_COUNT)
        draw_text_centered(pixels, width, height, menu_y + visible * item_h, "...", 0xFF555555, 1);

    // Data path display
    if (menu->settings_cursor == 12) {
        int path_y = menu_y + visible * item_h + 10;
        draw_rect(pixels, width, height, 30, path_y, width - 60, 12, 0xFF222244);
        const char *dp = menu->data_path;
        int dp_len = (int)strlen(dp);
        int max_chars = (width - 70) / 6;
        int start = 0;
        if (dp_len > max_chars) start = dp_len - max_chars;
        draw_text(pixels, width, height, 35, path_y + 2,
                  dp + start, menu->data_path_editing ? 0xFF44FF44 : 0xFFAAAACC, 1);
        if (menu->data_path_editing) {
            int cx = 35 + (menu->data_path_cursor - start) * 6;
            if ((menu->anim_tick / 6) % 2 == 0 && cx < width - 35)
                draw_rect(pixels, width, height, cx, path_y + 2, 1, 7, 0xFFFFFFFF);
        }
    }

    if (menu->data_path_editing) {
        draw_text_centered(pixels, width, height, height - 16,
                           "TYPE PATH  ENTER: CONFIRM  ESC: CANCEL",
                           0xFF555555, 1);
    } else {
        draw_text_centered(pixels, width, height, height - 16,
                           "UP-DOWN: SELECT  LEFT-RIGHT: ADJUST  ESC: BACK",
                           0xFF555555, 1);
    }
    draw_border(pixels, width, height, 5, 5, width - 10, height - 10, 0xFF444488, 1);
}

void start_menu_render(StartMenu *menu, uint32_t *pixels, int width, int height) {
    menu->anim_tick++;
    memset(pixels, 0, width * height * sizeof(uint32_t));

    if (menu->in_settings) {
        render_settings(menu, pixels, width, height);
        return;
    }

    // Title
    draw_text_centered(pixels, width, height, 15, "OPENCAPTIVE", 0xFFFF8800, 3);
    draw_text_centered(pixels, width, height, 42, "BY DANIEL NYLANDER", 0xFF666688, 1);
    draw_text_centered(pixels, width, height, 55, "SELECT GAME", 0xFF888888, 1);

    const char *items[] = {
        "CAPTIVE (1990)",
        "LIBERATION: CAPTIVE 2",
        "SETTINGS",
        "QUIT",
    };

    int menu_y = 80;
    int item_h = 20;

    for (int i = 0; i < menu->num_items; i++) {
        int y = menu_y + i * item_h;
        bool selected = (i == menu->selected_item);

        if (selected) {
            draw_rect(pixels, width, height, 40, y - 2, width - 80, item_h - 2, 0xFF333366);
            int pulse = (menu->anim_tick / 8) % 2;
            uint32_t arrow_col = pulse ? 0xFFFFFF00 : 0xFFFF8800;
            draw_text(pixels, width, height, 44, y, ">", arrow_col, 2);
        }

        uint32_t color = selected ? 0xFFFFFFFF : 0xFFAAAAAA;
        draw_text(pixels, width, height, 65, y, items[i], color, 2);
    }

    draw_text_centered(pixels, width, height, height - 20,
                       "UP-DOWN: SELECT  ENTER: START  ESC: QUIT",
                       0xFF555555, 1);
    draw_border(pixels, width, height, 5, 5, width - 10, height - 10, 0xFF444488, 1);
    char ver[32];
    snprintf(ver, sizeof(ver), "V%d.%d.%d",
             OPENCAPTIVE_VERSION_MAJOR, OPENCAPTIVE_VERSION_MINOR, OPENCAPTIVE_VERSION_PATCH);
    draw_text(pixels, width, height, 10, height - 12, ver, 0xFF333333, 1);
}
