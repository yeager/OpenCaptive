#include "terminal.h"
#include <string.h>
#include <stdio.h>

static void put_px(uint32_t *p, int w, int h, int x, int y, uint32_t c) {
    if (x >= 0 && x < w && y >= 0 && y < h) p[y * w + x] = c;
}

static void fill(uint32_t *p, int w, int h, int rx, int ry, int rw, int rh, uint32_t c) {
    for (int y = ry; y < ry + rh && y < h; y++) {
        if (y < 0) continue;
        for (int x = rx; x < rx + rw && x < w; x++) {
            if (x < 0) continue;
            p[y * w + x] = c;
        }
    }
}

static const uint8_t t_font[][5] = {
    ['A'] = {0x7C,0x12,0x12,0x12,0x7C}, ['B'] = {0x7E,0x4A,0x4A,0x4A,0x34},
    ['C'] = {0x3C,0x42,0x42,0x42,0x24}, ['D'] = {0x7E,0x42,0x42,0x42,0x3C},
    ['E'] = {0x7E,0x4A,0x4A,0x4A,0x42}, ['F'] = {0x7E,0x0A,0x0A,0x0A,0x02},
    ['G'] = {0x3C,0x42,0x52,0x52,0x34}, ['H'] = {0x7E,0x08,0x08,0x08,0x7E},
    ['I'] = {0x42,0x42,0x7E,0x42,0x42}, ['K'] = {0x7E,0x08,0x14,0x22,0x40},
    ['L'] = {0x7E,0x40,0x40,0x40,0x40}, ['M'] = {0x7E,0x04,0x08,0x04,0x7E},
    ['N'] = {0x7E,0x04,0x08,0x10,0x7E}, ['O'] = {0x3C,0x42,0x42,0x42,0x3C},
    ['P'] = {0x7E,0x12,0x12,0x12,0x0C}, ['R'] = {0x7E,0x12,0x32,0x52,0x0C},
    ['S'] = {0x24,0x4A,0x4A,0x4A,0x30}, ['T'] = {0x02,0x02,0x7E,0x02,0x02},
    ['U'] = {0x3E,0x40,0x40,0x40,0x3E}, ['V'] = {0x1E,0x20,0x40,0x20,0x1E},
    ['W'] = {0x3E,0x40,0x30,0x40,0x3E}, ['X'] = {0x42,0x24,0x18,0x24,0x42},
    ['Y'] = {0x06,0x08,0x70,0x08,0x06},
    ['0'] = {0x3C,0x46,0x4A,0x52,0x3C}, ['1'] = {0x00,0x44,0x7E,0x40,0x00},
    ['2'] = {0x64,0x52,0x52,0x52,0x4C}, ['3'] = {0x22,0x42,0x4A,0x4A,0x36},
    ['4'] = {0x1E,0x10,0x10,0x7E,0x10}, ['5'] = {0x2E,0x4A,0x4A,0x4A,0x32},
    ['6'] = {0x3C,0x4A,0x4A,0x4A,0x30}, ['7'] = {0x02,0x62,0x12,0x0A,0x06},
    ['8'] = {0x34,0x4A,0x4A,0x4A,0x34}, ['9'] = {0x0C,0x52,0x52,0x52,0x3C},
    [' '] = {0,0,0,0,0}, [':'] = {0x00,0x28,0x00,0x00,0x00},
    ['/'] = {0x40,0x20,0x10,0x08,0x04}, ['-'] = {0x08,0x08,0x08,0x00,0x00},
    ['.'] = {0x00,0x40,0x00,0x00,0x00}, ['!'] = {0x00,0x00,0x5E,0x00,0x00},
};

static void draw_t(uint32_t *fb, int pw, int ph,
                   int x, int y, const char *text, uint32_t color) {
    for (int i = 0; text[i]; i++) {
        unsigned char ch = (unsigned char)text[i];
        if (ch >= sizeof(t_font)/sizeof(t_font[0])) continue;
        const uint8_t *glyph = t_font[ch];
        for (int col = 0; col < 5; col++) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 7; row++) {
                if (bits & (1 << row))
                    put_px(fb, pw, ph, x + i * 6 + col, y + row, color);
            }
        }
    }
}

void terminal_init(TerminalState *ts, int level) {
    memset(ts, 0, sizeof(*ts));
    ts->page = TERM_MAIN;
    ts->level = level;
    ts->active = true;
}

static void render_main(const TerminalState *ts, const GameState *gs,
                        uint32_t *fb, int w, int h, int px, int py) {
    (void)gs;
    const char *options[] = {"1. MAP", "2. STATUS", "3. MISSION INFO", "4. EXIT"};
    for (int i = 0; i < 4; i++) {
        uint32_t col = (i == ts->cursor) ? 0xFF00FF00 : 0xFF00AA00;
        if (i == ts->cursor) fill(fb, w, h, px + 10, py + 24 + i * 12 - 1, 120, 10, 0xFF002200);
        draw_t(fb, w, h, px + 14, py + 24 + i * 12, options[i], col);
    }
}

static void render_map(const TerminalState *ts, const GameState *gs,
                       uint32_t *fb, int w, int h, int px, int py) {
    (void)ts;
    draw_t(fb, w, h, px + 14, py + 24, "LEVEL MAP", 0xFF00FF00);

    const DungeonLevel *lvl = &gs->levels[gs->current_level];
    int scale = 2;
    int ox = px + 20, oy = py + 36;

    for (int my = 0; my < MAP_HEIGHT && my < 60; my++) {
        for (int mx = 0; mx < MAP_WIDTH && mx < 60; mx++) {
            uint32_t c = 0;
            switch (lvl->cells[my][mx].type) {
                case CELL_WALL: c = 0xFF333333; break;
                case CELL_FLOOR: c = 0xFF005500; break;
                case CELL_DOOR: case CELL_DOOR_LOCKED: c = 0xFF0055AA; break;
                case CELL_STAIRS_UP: case CELL_STAIRS_DOWN: c = 0xFF00AA00; break;
                case CELL_GENERATOR: c = 0xFFAA0000; break;
                case CELL_SHOP: c = 0xFFAAAA00; break;
                default: break;
            }
            if (c) {
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        put_px(fb, w, h, ox + mx * scale + sx, oy + my * scale + sy, c);
            }
        }
    }
    // Party position
    for (int sy = 0; sy < scale; sy++)
        for (int sx = 0; sx < scale; sx++)
            put_px(fb, w, h, ox + gs->party_x * scale + sx,
                   oy + gs->party_y * scale + sy, 0xFFFFFFFF);
}

static void render_status(const TerminalState *ts, const GameState *gs,
                          uint32_t *fb, int w, int h, int px, int py) {
    (void)ts;
    draw_t(fb, w, h, px + 14, py + 24, "DROID STATUS", 0xFF00FF00);

    char buf[64];
    for (int i = 0; i < 4; i++) {
        const Droid *d = &gs->droids[i];
        snprintf(buf, sizeof(buf), "%s HP:%d/%d EN:%d/%d",
                 d->name, d->hp, d->hp_max, d->energy, d->energy_max);
        draw_t(fb, w, h, px + 14, py + 38 + i * 12, buf, 0xFF00CC00);
    }
}

static void render_mission(const TerminalState *ts, const GameState *gs,
                           uint32_t *fb, int w, int h, int px, int py) {
    (void)ts;
    char buf[64];
    draw_t(fb, w, h, px + 14, py + 24, "MISSION BRIEFING", 0xFF00FF00);

    snprintf(buf, sizeof(buf), "MISSION: %d", gs->mission);
    draw_t(fb, w, h, px + 14, py + 40, buf, 0xFF00CC00);

    snprintf(buf, sizeof(buf), "LEVEL: %d/%d", gs->current_level + 1, gs->num_levels);
    draw_t(fb, w, h, px + 14, py + 52, buf, 0xFF00CC00);

    snprintf(buf, sizeof(buf), "GENERATORS: %d/%d",
             gs->generators_destroyed, gs->generators_total);
    draw_t(fb, w, h, px + 14, py + 64, buf, 0xFF00CC00);

    draw_t(fb, w, h, px + 14, py + 84, "DESTROY ALL GENERATORS", 0xFF00AA00);
    draw_t(fb, w, h, px + 14, py + 96, "TO COMPLETE THE MISSION", 0xFF00AA00);
}

void terminal_render(const TerminalState *ts, const GameState *gs,
                     uint32_t *pixels, int width, int height) {
    // Dark background
    for (int i = 0; i < width * height; i++)
        pixels[i] = (pixels[i] & 0xFF000000) | ((pixels[i] & 0xFCFCFC) >> 2);

    int px = 30, py = 15, pw = 260, ph = 170;
    fill(pixels, width, height, px, py, pw, ph, 0xFF001100);

    // CRT scanline effect
    for (int y = py; y < py + ph; y += 2)
        for (int x = px; x < px + pw; x++)
            pixels[y * width + x] = (pixels[y * width + x] & 0xFF000000) |
                ((pixels[y * width + x] & 0xFEFEFE) >> 1);

    // Border
    for (int x = px; x < px + pw; x++) {
        put_px(pixels, width, height, x, py, 0xFF00FF00);
        put_px(pixels, width, height, x, py + ph - 1, 0xFF00FF00);
    }
    for (int y = py; y < py + ph; y++) {
        put_px(pixels, width, height, px, y, 0xFF00FF00);
        put_px(pixels, width, height, px + pw - 1, y, 0xFF00FF00);
    }

    draw_t(pixels, width, height, px + 14, py + 6, "TERMINAL SYSTEM V2.1", 0xFF00FF00);

    switch (ts->page) {
        case TERM_MAIN:    render_main(ts, gs, pixels, width, height, px, py); break;
        case TERM_MAP:     render_map(ts, gs, pixels, width, height, px, py); break;
        case TERM_STATUS:  render_status(ts, gs, pixels, width, height, px, py); break;
        case TERM_MISSION: render_mission(ts, gs, pixels, width, height, px, py); break;
    }

    draw_t(pixels, width, height, px + 14, py + ph - 12, "ESC:BACK", 0xFF006600);
}

bool terminal_handle_key(TerminalState *ts, int key) {
    if (ts->page == TERM_MAIN) {
        switch (key) {
            case 0x50: if (ts->cursor > 0) ts->cursor--; return true;
            case 0x51: if (ts->cursor < 3) ts->cursor++; return true;
            case 0x0D:
                switch (ts->cursor) {
                    case 0: ts->page = TERM_MAP; break;
                    case 1: ts->page = TERM_STATUS; break;
                    case 2: ts->page = TERM_MISSION; break;
                    case 3: ts->active = false; break;
                }
                return true;
        }
    } else {
        if (key == 0x1B) { ts->page = TERM_MAIN; return true; }
    }
    return false;
}
