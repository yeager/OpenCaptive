#include "start_menu.h"
#include <string.h>

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
    memset(menu, 0, sizeof(*menu));
    menu->num_items = 4;
    menu->platform = CAPTIVE_PLATFORM_DOS;
}

MenuResult start_menu_handle_event(StartMenu *menu, const SDL_Event *event) {
    if (event->type == SDL_EVENT_KEY_DOWN) {
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
                    case 3: return MENU_RESULT_QUIT;
                }
                break;
            case SDLK_ESCAPE:
                return MENU_RESULT_QUIT;
        }
    }
    return MENU_RESULT_NONE;
}

void start_menu_render(StartMenu *menu, uint32_t *pixels, int width, int height) {
    menu->anim_tick++;

    // Black background
    memset(pixels, 0, width * height * sizeof(uint32_t));

    // Title
    uint32_t title_color = 0xFFFF8800; // orange, like Captive's text
    draw_text_centered(pixels, width, height, 20, "OPENCAPTIVE", title_color, 3);

    // Subtitle
    uint32_t sub_color = 0xFF888888;
    draw_text_centered(pixels, width, height, 50, "SELECT GAME", sub_color, 1);

    // Menu items
    const char *items[] = {
        "CAPTIVE (1990)",
        "LIBERATION: CAPTIVE 2",
        "SETTINGS",
        "QUIT",
    };

    int menu_y = 75;
    int item_h = 20;

    for (int i = 0; i < menu->num_items; i++) {
        int y = menu_y + i * item_h;
        bool selected = (i == menu->selected_item);

        if (selected) {
            // Highlight bar
            uint32_t hl = 0xFF333366;
            draw_rect(pixels, width, height, 40, y - 2, width - 80, item_h - 2, hl);

            // Pulsing selection indicator
            int pulse = (menu->anim_tick / 8) % 2;
            uint32_t arrow_col = pulse ? 0xFFFFFF00 : 0xFFFF8800;
            draw_text(pixels, width, height, 44, y, ">", arrow_col, 2);
        }

        uint32_t color = selected ? 0xFFFFFFFF : 0xFFAAAAAA;
        draw_text(pixels, width, height, 65, y, items[i], color, 2);
    }

    // Bottom info
    draw_text_centered(pixels, width, height, height - 20,
                       "UP-DOWN: SELECT  ENTER: START  ESC: QUIT",
                       0xFF555555, 1);

    // Decorative border
    draw_border(pixels, width, height, 5, 5, width - 10, height - 10,
                0xFF444488, 1);

    // Version
    draw_text(pixels, width, height, 10, height - 12, "V0.1.0", 0xFF333333, 1);
}
