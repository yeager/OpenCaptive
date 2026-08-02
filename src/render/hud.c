#include "hud.h"
#include "opencaptive.h"
#include "xp_level.h"
#include <string.h>
#include <stdio.h>

static const int dir_dx[] = { 0, 1, 0, -1 };
static const int dir_dy[] = { -1, 0, 1, 0 };

// Minimal 4x6 digit font for HUD numbers
static const uint8_t digit_4x6[][6] = {
    [0] = {0x6,0x9,0x9,0x9,0x9,0x6},
    [1] = {0x2,0x6,0x2,0x2,0x2,0x7},
    [2] = {0x6,0x9,0x2,0x4,0x8,0xF},
    [3] = {0xE,0x1,0x6,0x1,0x1,0xE},
    [4] = {0x9,0x9,0xF,0x1,0x1,0x1},
    [5] = {0xF,0x8,0xE,0x1,0x1,0xE},
    [6] = {0x6,0x8,0xE,0x9,0x9,0x6},
    [7] = {0xF,0x1,0x2,0x4,0x4,0x4},
    [8] = {0x6,0x9,0x6,0x9,0x9,0x6},
    [9] = {0x6,0x9,0x7,0x1,0x1,0x6},
};

static void put_pixel(uint32_t *pixels, int w, int h, int x, int y, uint32_t c) {
    if (x >= 0 && x < w && y >= 0 && y < h)
        pixels[y * w + x] = c;
}

static void fill_rect(uint32_t *pixels, int w, int h,
                      int rx, int ry, int rw, int rh, uint32_t c) {
    for (int y = ry; y < ry + rh && y < h; y++) {
        if (y < 0) continue;
        for (int x = rx; x < rx + rw && x < w; x++) {
            if (x < 0) continue;
            pixels[y * w + x] = c;
        }
    }
}

static void draw_digit(uint32_t *pixels, int w, int h,
                       int x, int y, int digit, uint32_t color) {
    if (digit < 0 || digit > 9) return;
    for (int gy = 0; gy < 6; gy++) {
        for (int gx = 0; gx < 4; gx++) {
            if (digit_4x6[digit][gy] & (0x8 >> gx)) {
                put_pixel(pixels, w, h, x + gx, y + gy, color);
            }
        }
    }
}

static void draw_number(uint32_t *pixels, int w, int h,
                        int x, int y, int value, uint32_t color) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    for (int i = 0; buf[i]; i++) {
        draw_digit(pixels, w, h, x + i * 5, y, buf[i] - '0', color);
    }
}

static void draw_hp_bar(uint32_t *pixels, int w, int h,
                        int x, int y, int bar_w, int bar_h,
                        int current, int maximum) {
    // Background
    fill_rect(pixels, w, h, x, y, bar_w, bar_h, 0xFF222222);

    // Fill
    int fill_w = (current * (bar_w - 2)) / maximum;
    uint32_t fill_color;
    int pct = (current * 100) / maximum;
    if (pct > 60) fill_color = 0xFF00AA00;
    else if (pct > 30) fill_color = 0xFFAAAA00;
    else fill_color = 0xFFAA0000;

    fill_rect(pixels, w, h, x + 1, y + 1, fill_w, bar_h - 2, fill_color);

    // Border
    for (int bx = x; bx < x + bar_w; bx++) {
        put_pixel(pixels, w, h, bx, y, 0xFF555555);
        put_pixel(pixels, w, h, bx, y + bar_h - 1, 0xFF555555);
    }
    for (int by = y; by < y + bar_h; by++) {
        put_pixel(pixels, w, h, x, by, 0xFF555555);
        put_pixel(pixels, w, h, x + bar_w - 1, by, 0xFF555555);
    }
}

static void draw_minimap(const GameState *gs, uint32_t *pixels, int w, int h,
                         int ox, int oy, int mw, int mh) {
    const DungeonLevel *lvl = &gs->levels[gs->current_level];

    fill_rect(pixels, w, h, ox, oy, mw, mh, 0xFF111111);

    // Show 16x8 area centered on party
    int view_cells_x = 16;
    int view_cells_y = 8;
    int cell_px = mw / view_cells_x;
    int cell_py = mh / view_cells_y;

    int start_x = gs->party_x - view_cells_x / 2;
    int start_y = gs->party_y - view_cells_y / 2;

    for (int cy = 0; cy < view_cells_y; cy++) {
        for (int cx = 0; cx < view_cells_x; cx++) {
            int mx = start_x + cx;
            int my = start_y + cy;
            int px = ox + cx * cell_px;
            int py = oy + cy * cell_py;

            if (mx < 0 || mx >= MAP_WIDTH || my < 0 || my >= MAP_HEIGHT) continue;

            CellType cell = lvl->cells[my][mx].type;
            uint32_t color = 0xFF111111;
            switch (cell) {
                case CELL_FLOOR:       color = 0xFF333333; break;
                case CELL_DOOR:        color = 0xFF3333AA; break;
                case CELL_DOOR_LOCKED: color = 0xFFAA3333; break;
                case CELL_STAIRS_UP:
                case CELL_STAIRS_DOWN: color = 0xFF33AA33; break;
                case CELL_GENERATOR:   color = 0xFFFF4444; break;
                case CELL_SHOP:        color = 0xFFAAAA33; break;
                case CELL_TELEPORTER:  color = 0xFF9933AA; break;
                default: break;
            }
            fill_rect(pixels, w, h, px, py, cell_px, cell_py, color);
        }
    }

    // Party marker
    int ppx = ox + (gs->party_x - start_x) * cell_px;
    int ppy = oy + (gs->party_y - start_y) * cell_py;
    fill_rect(pixels, w, h, ppx, ppy, cell_px, cell_py, 0xFFFFFFFF);

    // Border
    for (int bx = ox; bx < ox + mw; bx++) {
        put_pixel(pixels, w, h, bx, oy, 0xFF555555);
        put_pixel(pixels, w, h, bx, oy + mh - 1, 0xFF555555);
    }
    for (int by = oy; by < oy + mh; by++) {
        put_pixel(pixels, w, h, ox, by, 0xFF555555);
        put_pixel(pixels, w, h, ox + mw - 1, by, 0xFF555555);
    }
}

void hud_render(const GameState *gs, uint32_t *pixels, int width, int height) {
    // HUD area: below viewport (y >= CAPTIVE_VIEWPORT_Y + CAPTIVE_VIEWPORT_HEIGHT)
    int hud_y = CAPTIVE_VIEWPORT_Y + CAPTIVE_VIEWPORT_HEIGHT + 4;

    // Viewport border
    for (int x = CAPTIVE_VIEWPORT_X - 1; x <= CAPTIVE_VIEWPORT_X + CAPTIVE_VIEWPORT_WIDTH; x++) {
        put_pixel(pixels, width, height, x, CAPTIVE_VIEWPORT_Y - 1, 0xFF444488);
        put_pixel(pixels, width, height, x, CAPTIVE_VIEWPORT_Y + CAPTIVE_VIEWPORT_HEIGHT, 0xFF444488);
    }
    for (int y = CAPTIVE_VIEWPORT_Y - 1; y <= CAPTIVE_VIEWPORT_Y + CAPTIVE_VIEWPORT_HEIGHT; y++) {
        put_pixel(pixels, width, height, CAPTIVE_VIEWPORT_X - 1, y, 0xFF444488);
        put_pixel(pixels, width, height, CAPTIVE_VIEWPORT_X + CAPTIVE_VIEWPORT_WIDTH, y, 0xFF444488);
    }

    // Droid status panels (4 droids, side by side)
    int panel_w = 72;
    int panel_h = 40;
    for (int i = 0; i < 4; i++) {
        int px = 8 + i * (panel_w + 4);
        int py = hud_y;

        bool selected = (i == gs->selected_droid);
        uint32_t border = selected ? 0xFFFF8800 : 0xFF444444;

        // Panel background
        fill_rect(pixels, width, height, px, py, panel_w, panel_h, 0xFF111122);

        // Border
        for (int bx = px; bx < px + panel_w; bx++) {
            put_pixel(pixels, width, height, bx, py, border);
            put_pixel(pixels, width, height, bx, py + panel_h - 1, border);
        }
        for (int by = py; by < py + panel_h; by++) {
            put_pixel(pixels, width, height, px, by, border);
            put_pixel(pixels, width, height, px + panel_w - 1, by, border);
        }

        // HP bar
        draw_hp_bar(pixels, width, height,
                    px + 2, py + 4, panel_w - 4, 6,
                    gs->droids[i].hp, gs->droids[i].hp_max);

        // Energy bar
        draw_hp_bar(pixels, width, height,
                    px + 2, py + 12, panel_w - 4, 6,
                    gs->droids[i].energy, gs->droids[i].energy_max);

        // HP number
        draw_number(pixels, width, height, px + 4, py + 20,
                    gs->droids[i].hp, 0xFF00AA00);

        // Level
        draw_number(pixels, width, height, px + 40, py + 20,
                    xp_to_display_level(gs->droids[i].xp), 0xFFAAAA00);

        // Droid number
        draw_number(pixels, width, height, px + 4, py + 30,
                    i + 1, 0xFF888888);
    }

    // Compass / direction indicator (right side)
    int compass_x = 295;
    int compass_y = hud_y + 5;
    fill_rect(pixels, width, height, compass_x, compass_y, 20, 20, 0xFF222244);
    // Direction letter would go here but we don't have text drawing in hud.c
    // Instead draw a directional indicator
    int cx = compass_x + 10;
    int cy = compass_y + 10;
    int arrow_dx = dir_dx[gs->party_dir] * 6;
    int arrow_dy = dir_dy[gs->party_dir] * 6;
    for (int i = -3; i <= 3; i++) {
        put_pixel(pixels, width, height, cx + arrow_dx + (arrow_dy ? 0 : i),
                  cy + arrow_dy + (arrow_dx ? 0 : i), 0xFFFFFF00);
        put_pixel(pixels, width, height, cx + arrow_dx / 2, cy + arrow_dy / 2, 0xFFFFFF00);
    }

    // Minimap (right side, below compass)
    if (gs->map_overlay) {
        draw_minimap(gs, pixels, width, height, 0, 0, width, height);
    } else {
        draw_minimap(gs, pixels, width, height, 4, hud_y, 30, 30);
    }

    // Level and gold display
    draw_number(pixels, width, height, 8, height - 8, gs->current_level + 1, 0xFF555555);
    draw_number(pixels, width, height, width - 40, height - 8, gs->gold, 0xFFFFAA00);
}
