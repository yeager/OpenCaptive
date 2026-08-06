#include "custom_features.h"
#include "game_state.h"
#include <string.h>
#include <stdio.h>

static void draw_char(uint32_t *fb, int fb_w, int fb_h,
                      int x, int y, char ch, uint32_t color) {
    static const uint8_t font4x6[][6] = {
        ['0'] = {0x6,0x9,0xB,0xD,0x9,0x6},
        ['1'] = {0x2,0x6,0x2,0x2,0x2,0x7},
        ['2'] = {0x6,0x9,0x2,0x4,0x8,0xF},
        ['3'] = {0xE,0x1,0x6,0x1,0x1,0xE},
        ['4'] = {0x9,0x9,0xF,0x1,0x1,0x1},
        ['5'] = {0xF,0x8,0xE,0x1,0x1,0xE},
        ['6'] = {0x6,0x8,0xE,0x9,0x9,0x6},
        ['7'] = {0xF,0x1,0x2,0x4,0x4,0x4},
        ['8'] = {0x6,0x9,0x6,0x9,0x9,0x6},
        ['9'] = {0x6,0x9,0x7,0x1,0x1,0x6},
        ['A'] = {0x6,0x9,0xF,0x9,0x9,0x9},
        ['B'] = {0xE,0x9,0xE,0x9,0x9,0xE},
        ['C'] = {0x6,0x9,0x8,0x8,0x9,0x6},
        ['D'] = {0xE,0x9,0x9,0x9,0x9,0xE},
        ['E'] = {0xF,0x8,0xE,0x8,0x8,0xF},
        ['F'] = {0xF,0x8,0xE,0x8,0x8,0x8},
        ['G'] = {0x6,0x9,0x8,0xB,0x9,0x6},
        ['H'] = {0x9,0x9,0xF,0x9,0x9,0x9},
        ['I'] = {0x7,0x2,0x2,0x2,0x2,0x7},
        ['L'] = {0x8,0x8,0x8,0x8,0x8,0xF},
        ['M'] = {0x9,0xF,0xF,0x9,0x9,0x9},
        ['N'] = {0x9,0xD,0xB,0x9,0x9,0x9},
        ['O'] = {0x6,0x9,0x9,0x9,0x9,0x6},
        ['P'] = {0xE,0x9,0xE,0x8,0x8,0x8},
        ['R'] = {0xE,0x9,0xE,0xA,0x9,0x9},
        ['S'] = {0x6,0x8,0x6,0x1,0x1,0x6},
        ['T'] = {0x7,0x2,0x2,0x2,0x2,0x2},
        ['V'] = {0x9,0x9,0x9,0x9,0x6,0x6},
        ['W'] = {0x9,0x9,0x9,0xF,0xF,0x9},
        ['X'] = {0x9,0x9,0x6,0x6,0x9,0x9},
        ['Y'] = {0x5,0x5,0x2,0x2,0x2,0x2},
        [':'] = {0x0,0x2,0x0,0x0,0x2,0x0},
        [','] = {0x0,0x0,0x0,0x0,0x2,0x4},
        [' '] = {0x0,0x0,0x0,0x0,0x0,0x0},
        ['/'] = {0x1,0x1,0x2,0x4,0x8,0x8},
        ['.'] = {0x0,0x0,0x0,0x0,0x0,0x2},
        ['-'] = {0x0,0x0,0xF,0x0,0x0,0x0},
        ['='] = {0x0,0xF,0x0,0xF,0x0,0x0},
        ['('] = {0x2,0x4,0x4,0x4,0x4,0x2},
        [')'] = {0x4,0x2,0x2,0x2,0x2,0x4},
    };

    unsigned uch = (unsigned char)ch;
    if (uch >= sizeof(font4x6) / sizeof(font4x6[0])) return;

    for (int row = 0; row < 6; row++) {
        uint8_t bits = font4x6[uch][row];
        for (int col = 0; col < 4; col++) {
            if (bits & (0x8 >> col)) {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < fb_w && py >= 0 && py < fb_h)
                    fb[py * fb_w + px] = color;
            }
        }
    }
}

static void draw_string(uint32_t *fb, int fb_w, int fb_h,
                         int x, int y, const char *str, uint32_t color) {
    while (*str) {
        draw_char(fb, fb_w, fb_h, x, y, *str, color);
        x += 5;
        str++;
    }
}

void debug_hud_render(uint32_t *fb, int fb_w, int fb_h,
                      const void *game_state_ptr, const CustomFeatures *feat) {
    if (!feat || !feat->debug_hud || !fb || fb_w <= 0 || fb_h <= 0 ||
        !game_state_ptr) return;

    const GameState *gs = (const GameState *)game_state_ptr;
    if (gs->current_level < 0 || gs->current_level >= MAX_LEVELS ||
        gs->current_level >= gs->num_levels || gs->num_levels > MAX_LEVELS ||
        gs->party_x < 0 || gs->party_x >= MAP_WIDTH ||
        gs->party_y < 0 || gs->party_y >= MAP_HEIGHT) return;
    char buf[128];
    int y = 2;
    uint32_t color = 0xFF00FF00;

    snprintf(buf, sizeof(buf), "POS: %d,%d", gs->party_x, gs->party_y);
    draw_string(fb, fb_w, fb_h, 2, y, buf, color);
    y += 8;

    static const char *dir_names[] = {"N", "E", "S", "W"};
    int safe_dir = gs->party_dir % 4;
    if (safe_dir < 0) safe_dir += 4;
    snprintf(buf, sizeof(buf), "DIR: %s", dir_names[safe_dir]);
    draw_string(fb, fb_w, fb_h, 2, y, buf, color);
    y += 8;

    snprintf(buf, sizeof(buf), "LVL: %d/%d", gs->current_level + 1, gs->num_levels);
    draw_string(fb, fb_w, fb_h, 2, y, buf, color);
    y += 8;

    const DungeonLevel *lvl = &gs->levels[gs->current_level];
    CellType ct = lvl->cells[gs->party_y][gs->party_x].type;
    static const char *cell_names[] = {
        "WALL", "FLOOR", "DOOR", "LOCKED", "UP", "DOWN",
        "SHOP", "TELE", "GEN", "TERM"
    };
    int safe_cell = (int)ct;
    if (safe_cell < 0 || safe_cell >= 10) safe_cell = 0;
    snprintf(buf, sizeof(buf), "CELL: %s", cell_names[safe_cell]);
    draw_string(fb, fb_w, fb_h, 2, y, buf, color);
    y += 8;

    snprintf(buf, sizeof(buf), "TICK: %u", gs->tick);
    draw_string(fb, fb_w, fb_h, 2, y, buf, color);
    y += 8;

    snprintf(buf, sizeof(buf), "GEN: %d/%d",
             gs->generators_destroyed, gs->generators_total);
    draw_string(fb, fb_w, fb_h, 2, y, buf, color);
    y += 8;

    snprintf(buf, sizeof(buf), "GOLD: %d", gs->gold);
    draw_string(fb, fb_w, fb_h, 2, y, buf, color);
    y += 8;

    snprintf(buf, sizeof(buf), "SEED: %08X", gs->mission_seed);
    draw_string(fb, fb_w, fb_h, 2, y, buf, color);
    y += 12;

    for (int d = 0; d < 4; d++) {
        const Droid *dr = &gs->droids[d];
        snprintf(buf, sizeof(buf), "D%d: %d/%d HP %d/%d EN",
                 d + 1, dr->hp, dr->hp_max, dr->energy, dr->energy_max);
        draw_string(fb, fb_w, fb_h, 2, y, buf,
                    d == gs->selected_droid ? 0xFFFFFF00 : 0xFF00CC00);
        y += 8;
    }
}
