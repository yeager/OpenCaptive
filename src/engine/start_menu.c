#include "start_menu.h"
#include "opencaptive.h"
#include "i18n.h"
#include <string.h>
#include <stdio.h>

static const char *const lang_codes[] = {
    "en", "sv", "cs", "da", "de", "es", "fi", "fr", "hu", "it",
    "ja", "ko", "nl", "no", "pl", "pt", "ro", "ru", "zh",
};
static const char *const lang_labels[] = {
    "ENGLISH", "SVENSKA", "CESKY", "DANSK", "DEUTSCH",
    "ESPANOL", "SUOMI", "FRANCAIS", "MAGYAR", "ITALIANO",
    "NIHONGO", "HANGUGEO", "NEDERLANDS", "NORSK", "POLSKI",
    "PORTUGUES", "ROMANA", "RUSSKIJ", "ZHONGWEN",
};
#define LANG_COUNT 19

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

    const char *cur_lang = i18n_get_lang();
    menu->lang_index = 0;
    for (int i = 0; i < LANG_COUNT; i++) {
        if (strcmp(lang_codes[i], cur_lang) == 0) {
            menu->lang_index = i;
            break;
        }
    }
}

MenuResult start_menu_handle_click(StartMenu *menu, float x, float y) {
    if (!menu || menu->in_settings) return MENU_RESULT_NONE;

    /* Card layout: two game cards on top row, settings/quit on bottom.
     * Coordinates must match start_menu_render(). */
    const int card_w = 130, card_h = 90, card_y = 38, gap = 10;
    const int total_w = card_w * 2 + gap;
    const int card_x0 = (320 - total_w) / 2;
    const int card_x1 = card_x0 + card_w + gap;
    const int bottom_y = card_y + card_h + 25;

    int item = -1;
    if (y >= card_y && y < card_y + card_h) {
        if (x >= card_x0 && x < card_x0 + card_w) item = 0;
        else if (x >= card_x1 && x < card_x1 + card_w) item = 1;
    } else if (y >= bottom_y && y < bottom_y + 12) {
        if (x >= card_x0 && x < card_x0 + card_w) item = 2;
        else if (x >= card_x1 && x < card_x1 + card_w) item = 3;
    }
    if (item < 0) return MENU_RESULT_NONE;
    menu->selected_item = item;
    switch (item) {
        case 0: return MENU_RESULT_START_CAPTIVE;
        case 1: return MENU_RESULT_START_LIBERATION;
        case 2: menu->in_settings = true; menu->settings_cursor = 0; break;
        case 3: return MENU_RESULT_QUIT;
    }
    return MENU_RESULT_NONE;
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
        #define SETTINGS_COUNT 16
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
                    /* Captive's original viewport composition is pending
                     * recovery.  Do not expose the retired generated corridor
                     * as a graphics option when verified media is present. */
                    case 0: menu->enhanced_mode = false; break;
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
                    case 12: menu->sfx_enabled = !menu->sfx_enabled; break;
                    case 13:
                        if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
                            menu->data_path_editing = true;
                            menu->data_path_cursor = (int)strlen(menu->data_path);
                            SDL_StartTextInput(NULL);
                        }
                        break;
                    case 14:
                        if (event->key.key == SDLK_RIGHT)
                            menu->lang_index = (menu->lang_index + 1) % LANG_COUNT;
                        else if (event->key.key == SDLK_LEFT)
                            menu->lang_index = (menu->lang_index + LANG_COUNT - 1) % LANG_COUNT;
                        i18n_init(lang_codes[menu->lang_index]);
                        break;
                    case 15: menu->in_settings = false; break;
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
            if (menu->selected_item >= 2) menu->selected_item -= 2;
            break;
        case SDLK_DOWN:
            if (menu->selected_item < 2) menu->selected_item += 2;
            break;
        case SDLK_LEFT:
            if (menu->selected_item == 1) menu->selected_item = 0;
            else if (menu->selected_item == 3) menu->selected_item = 2;
            break;
        case SDLK_RIGHT:
            if (menu->selected_item == 0) menu->selected_item = 1;
            else if (menu->selected_item == 2) menu->selected_item = 3;
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
    draw_text_centered(pixels, width, height, 8, _("OPENCAPTIVE"), 0xFFFF8800, 2);
    draw_text_centered(pixels, width, height, 26, _("BY DANIEL NYLANDER"), 0xFF666688, 1);
    draw_text_centered(pixels, width, height, 38, _("SETTINGS"), 0xFF888888, 1);

    const char *labels[] = {
        _("RENDERER:"),
        _("SCANLINES:"),
        _("CRT CURVE:"),
        _("BILINEAR:"),
        _("INT SCALE:"),
        _("SCALE:"),
        _("FULLSCREEN:"),
        _("VSYNC:"),
        _("FPS LIMIT:"),
        _("BRIGHTNESS:"),
        _("CONTRAST:"),
        _("MUSIC:"),
        _("SFX:"),
        _("DATA PATH:"),
        _("LANGUAGE:"),
        _("BACK"),
    };
    char values[SETTINGS_COUNT][20];
    snprintf(values[0], 20, "PENDING");
    snprintf(values[1], 20, "%s", menu->scanlines ? _("ON") : _("OFF"));
    snprintf(values[2], 20, "%s", menu->crt_curvature ? _("ON") : _("OFF"));
    snprintf(values[3], 20, "%s", menu->bilinear ? _("ON") : _("OFF"));
    snprintf(values[4], 20, "%s", menu->integer_scaling ? _("ON") : _("OFF"));
    snprintf(values[5], 20, "%dX", menu->scale_factor);
    snprintf(values[6], 20, "%s", menu->fullscreen ? _("ON") : _("OFF"));
    snprintf(values[7], 20, "%s", menu->vsync ? _("ON") : _("OFF"));
    snprintf(values[8], 20, "%s", menu->fps_limit == 0 ? _("UNLIMITED") :
             (menu->fps_limit == 30 ? "30" : (menu->fps_limit == 60 ? "60" : "120")));
    snprintf(values[9], 20, "%d%%", menu->brightness);
    snprintf(values[10], 20, "%d%%", menu->contrast);
    snprintf(values[11], 20, "%s", menu->music_enabled ? _("ON") : _("OFF"));
    snprintf(values[12], 20, "%s", menu->sfx_enabled ? _("ON") : _("OFF"));
    values[13][0] = '\0';
    snprintf(values[14], 20, "%s", lang_labels[menu->lang_index]);
    values[15][0] = '\0';

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
    if (menu->settings_cursor == 13) {
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
                           _("TYPE PATH  ENTER: CONFIRM  ESC: CANCEL"),
                           0xFF555555, 1);
    } else {
        draw_text_centered(pixels, width, height, height - 16,
                           _("UP-DOWN: SELECT  LEFT-RIGHT: ADJUST  ESC: BACK"),
                           0xFF555555, 1);
    }
    draw_border(pixels, width, height, 5, 5, width - 10, height - 10, 0xFF444488, 1);
}

static uint32_t hash_pixel(int x, int y, int seed) {
    uint32_t h = (uint32_t)(x * 374761393 + y * 668265263 + seed * 1274126177);
    h = (h ^ (h >> 13)) * 1103515245;
    return h ^ (h >> 16);
}

static void draw_captive_card(uint32_t *pixels, int pw, int ph,
                              int cx, int cy, int cw, int ch,
                              bool selected, uint32_t tick) {
    uint32_t wall_dark  = 0xFF2A1A3A;
    uint32_t wall_mid   = 0xFF3D2D50;
    uint32_t wall_light = 0xFF504068;
    uint32_t floor_col  = 0xFF1A1020;
    uint32_t torch_col  = 0xFFFF8800;

    for (int y = 0; y < ch; y++) {
        for (int x = 0; x < cw; x++) {
            int px = cx + x, py = cy + y;
            if (px < 0 || px >= pw || py < 0 || py >= ph) continue;

            uint32_t h = hash_pixel(x, y, 42);
            uint32_t col;

            if (y < 3 || y >= ch - 3 || x < 3 || x >= cw - 3) {
                col = wall_light;
            } else if (y < ch / 3) {
                col = (h % 5 == 0) ? wall_light : wall_mid;
            } else if (y > ch * 2 / 3) {
                col = floor_col;
                if ((x % 16) < 2 || (y % 8) < 1) col = wall_dark;
            } else {
                col = wall_dark;
                if ((x + 2) % 24 < 4 && y == ch / 2)
                    col = (h % 3 == 0) ? wall_light : wall_mid;
            }

            if (x > cw / 3 && x < cw * 2 / 3 && y > ch / 4 && y < ch * 3 / 4) {
                int corridor_shade = (int)(y - ch / 4) * 2;
                uint32_t r = ((col >> 16) & 0xFF);
                uint32_t g = ((col >> 8) & 0xFF);
                uint32_t b = (col & 0xFF);
                r = r > (uint32_t)corridor_shade ? r - corridor_shade : 0;
                g = g > (uint32_t)corridor_shade ? g - corridor_shade : 0;
                b = b > (uint32_t)corridor_shade ? b - corridor_shade : 0;
                col = 0xFF000000 | (r << 16) | (g << 8) | b;
            }

            int torch_x1 = cw / 4, torch_x2 = cw * 3 / 4;
            int torch_y = ch / 2 - 5;
            int flicker = (int)((tick / 4 + h) % 4);
            if (((x > torch_x1 - 3 && x < torch_x1 + 3) ||
                 (x > torch_x2 - 3 && x < torch_x2 + 3)) &&
                y > torch_y - 2 - flicker && y < torch_y + 3) {
                col = torch_col;
            }

            pixels[py * pw + px] = col;
        }
    }

    if (selected) {
        uint32_t sel_col = ((tick / 8) % 2) ? 0xFFFF8800 : 0xFFFFAA44;
        draw_border(pixels, pw, ph, cx, cy, cw, ch, sel_col, 2);
    } else {
        draw_border(pixels, pw, ph, cx, cy, cw, ch, 0xFF444466, 1);
    }
}

static void draw_liberation_card(uint32_t *pixels, int pw, int ph,
                                 int cx, int cy, int cw, int ch,
                                 bool selected, uint32_t tick) {
    uint32_t sky_top    = 0xFF0A0A2A;
    uint32_t sky_bot    = 0xFF1A1A4A;
    uint32_t bldg_dark  = 0xFF222244;
    uint32_t bldg_mid   = 0xFF333366;
    uint32_t bldg_light = 0xFF444488;
    uint32_t window_on  = 0xFFFFDD66;
    uint32_t window_off = 0xFF111133;
    uint32_t road_col   = 0xFF181828;

    for (int y = 0; y < ch; y++) {
        for (int x = 0; x < cw; x++) {
            int px = cx + x, py = cy + y;
            if (px < 0 || px >= pw || py < 0 || py >= ph) continue;

            uint32_t h = hash_pixel(x, y, 99);
            uint32_t col;

            int horizon = ch * 3 / 5;

            if (y < horizon) {
                uint32_t r1 = (sky_top >> 16) & 0xFF, g1 = (sky_top >> 8) & 0xFF, b1 = sky_top & 0xFF;
                uint32_t r2 = (sky_bot >> 16) & 0xFF, g2 = (sky_bot >> 8) & 0xFF, b2 = sky_bot & 0xFF;
                int t = y * 256 / horizon;
                col = 0xFF000000 |
                    (((r1 * (256 - t) + r2 * t) >> 8) << 16) |
                    (((g1 * (256 - t) + g2 * t) >> 8) << 8) |
                    ((b1 * (256 - t) + b2 * t) >> 8);

                int bx = x % 28;
                int building_height = (int)(10 + (h % 25));
                if (y > horizon - building_height && bx > 2 && bx < 26) {
                    col = (h % 3 == 0) ? bldg_light :
                          (h % 3 == 1) ? bldg_mid : bldg_dark;
                    if ((bx - 5) % 6 < 3 && (y - (horizon - building_height)) % 5 > 1) {
                        uint32_t wh = hash_pixel(x / 6, y / 5, 77);
                        int blink = (int)((tick / 30 + wh) % 8);
                        col = (blink < 3) ? window_on : window_off;
                    }
                }

                if (h % 200 == 0 && y < horizon / 2) col = 0xFFCCCCDD;
            } else {
                col = road_col;
                if ((y - horizon) < 2) col = bldg_dark;
                if ((x % 32) < 2 && (y - horizon) > 5)
                    col = 0xFF333344;
            }

            pixels[py * pw + px] = col;
        }
    }

    if (selected) {
        uint32_t sel_col = ((tick / 8) % 2) ? 0xFF4488FF : 0xFF66AAFF;
        draw_border(pixels, pw, ph, cx, cy, cw, ch, sel_col, 2);
    } else {
        draw_border(pixels, pw, ph, cx, cy, cw, ch, 0xFF444466, 1);
    }
}

void start_menu_render(StartMenu *menu, uint32_t *pixels, int width, int height) {
    menu->anim_tick++;
    memset(pixels, 0, width * height * sizeof(uint32_t));

    if (menu->in_settings) {
        render_settings(menu, pixels, width, height);
        return;
    }

    draw_text_centered(pixels, width, height, 6, _("OPENCAPTIVE"), 0xFFFF8800, 2);
    draw_text_centered(pixels, width, height, 22, _("BY DANIEL NYLANDER"), 0xFF666688, 1);

    int card_w = 130;
    int card_h = 90;
    int card_y = 38;
    int gap = 10;
    int total_w = card_w * 2 + gap;
    int card_x0 = (width - total_w) / 2;
    int card_x1 = card_x0 + card_w + gap;

    draw_captive_card(pixels, width, height,
                      card_x0, card_y, card_w, card_h,
                      menu->selected_item == 0, menu->anim_tick);
    draw_liberation_card(pixels, width, height,
                         card_x1, card_y, card_w, card_h,
                         menu->selected_item == 1, menu->anim_tick);

    draw_text_centered(pixels, width, height,
                       card_y + card_h + 3, "CAPTIVE", 0xFFCCCCCC, 1);
    draw_text(pixels, width, height,
              card_x1 + (card_w - 11 * 6) / 2, card_y + card_h + 3,
              "LIBERATION", 0xFFCCCCCC, 1);

    draw_text_centered(pixels, width, height,
                       card_y + card_h + 12, "1990",
                       0xFF888888, 1);
    draw_text(pixels, width, height,
              card_x1 + (card_w - 4 * 6) / 2, card_y + card_h + 12,
              "1993", 0xFF888888, 1);

    int bottom_y = card_y + card_h + 25;
    int btn_items = 2;
    const char *bottom_labels[] = { _("SETTINGS"), _("QUIT") };
    int bottom_idx[] = { 2, 3 };

    for (int i = 0; i < btn_items; i++) {
        int bx = card_x0 + i * (card_w + gap);
        int by = bottom_y;
        bool sel = (menu->selected_item == bottom_idx[i]);

        if (sel) {
            draw_rect(pixels, width, height, bx, by - 1, card_w, 12, 0xFF333366);
            uint32_t ac = ((menu->anim_tick / 8) % 2) ? 0xFFFFFF00 : 0xFFFF8800;
            draw_text(pixels, width, height, bx + 2, by, ">", ac, 1);
        }
        uint32_t col = sel ? 0xFFFFFFFF : 0xFFAAAAAA;
        int tw = (int)strlen(bottom_labels[i]) * 6;
        draw_text(pixels, width, height, bx + (card_w - tw) / 2, by,
                  bottom_labels[i], col, 1);
    }

    draw_text_centered(pixels, width, height, height - 10,
                       _("UP-DOWN: SELECT  ENTER: START  ESC: QUIT"),
                       0xFF444444, 1);
    draw_border(pixels, width, height, 5, 5, width - 10, height - 10, 0xFF444488, 1);
    char ver[32];
    snprintf(ver, sizeof(ver), "V%d.%d.%d",
             OPENCAPTIVE_VERSION_MAJOR, OPENCAPTIVE_VERSION_MINOR, OPENCAPTIVE_VERSION_PATCH);
    draw_text(pixels, width, height, 10, height - 12, ver, 0xFF333333, 1);
}
