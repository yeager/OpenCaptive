#include "viewport.h"
#include "opencaptive.h"
#include <string.h>

// Wall/floor colors per texture set
static const uint32_t wall_colors[] = {
    0xFF606080, // set 0: grey-blue
    0xFF806040, // set 1: brown
    0xFF408040, // set 2: green
    0xFF804040, // set 3: red-brown
};

static const uint32_t floor_colors[] = {
    0xFF303040,
    0xFF403020,
    0xFF204020,
    0xFF402020,
};

static const uint32_t ceil_colors[] = {
    0xFF404050,
    0xFF504030,
    0xFF305030,
    0xFF503030,
};

static const uint32_t door_color   = 0xFF5555AA;
static const uint32_t locked_color = 0xFFAA5555;
static const uint32_t stairs_color = 0xFF55AA55;
static const uint32_t gen_color    = 0xFFFF4444;
static const uint32_t shop_color   = 0xFFFFFF44;

// Direction offsets: N, E, S, W
static const int dir_dx[] = { 0, 1, 0, -1 };
static const int dir_dy[] = { -1, 0, 1, 0 };

static CellType get_cell(const DungeonLevel *lvl, int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return CELL_WALL;
    return lvl->cells[y][x].type;
}

static uint8_t get_wall_tex(const DungeonLevel *lvl, int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return 0;
    return lvl->cells[y][x].wall_tex[0] % 4;
}

static void fill_rect(uint32_t *pixels, int stride,
                      int x, int y, int w, int h, uint32_t color) {
    for (int ry = y; ry < y + h; ry++) {
        if (ry < 0 || ry >= CAPTIVE_VIEWPORT_HEIGHT) continue;
        for (int rx = x; rx < x + w; rx++) {
            if (rx < 0 || rx >= CAPTIVE_VIEWPORT_WIDTH) continue;
            pixels[ry * stride + rx] = color;
        }
    }
}

static uint32_t darken(uint32_t color, int depth) {
    // Darken color based on distance (0=closest, 4=furthest)
    int shift = depth;
    uint8_t r = ((color >> 16) & 0xFF) >> shift;
    uint8_t g = ((color >> 8) & 0xFF) >> shift;
    uint8_t b = (color & 0xFF) >> shift;
    return 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

// Render a single depth row of the viewport
// depth: 0=closest (player row), 3=farthest
static void render_row(const GameState *gs, uint32_t *pixels, int stride,
                       int depth, int center_x, int center_y, Direction dir) {
    const DungeonLevel *lvl = &gs->levels[gs->current_level];

    // Forward/right vectors based on direction
    int fwd_x = dir_dx[dir];
    int fwd_y = dir_dy[dir];
    int right_x = dir_dx[(dir + 1) % 4];
    int right_y = dir_dy[(dir + 1) % 4];

    int dist = 4 - depth; // cells ahead (4=far, 1=near, 0=player)

    // Cell position ahead
    int ahead_x = center_x + fwd_x * dist;
    int ahead_y = center_y + fwd_y * dist;

    // Perspective scaling
    int vp_w = CAPTIVE_VIEWPORT_WIDTH;
    int vp_h = CAPTIVE_VIEWPORT_HEIGHT;
    int half_h = vp_h / 2;

    // Wall height decreases with distance
    int wall_h = vp_h / (depth + 1);
    int wall_top = half_h - wall_h / 2;

    // Width of one cell at this depth
    int cell_w = vp_w / (depth + 2);

    // Visible lateral range: -(depth+1) to +(depth+1), centered
    int lateral_range = (depth < 2) ? 2 : depth;

    for (int lat = -lateral_range; lat <= lateral_range; lat++) {
        int cx = ahead_x + right_x * lat;
        int cy = ahead_y + right_y * lat;

        // Screen x position for this cell
        int screen_x = vp_w / 2 + lat * cell_w - cell_w / 2;

        CellType cell = get_cell(lvl, cx, cy);
        uint8_t tex = get_wall_tex(lvl, cx, cy);

        if (cell == CELL_WALL) {
            // Draw wall face
            uint32_t wc = darken(wall_colors[tex], depth);
            fill_rect(pixels, stride, screen_x, wall_top, cell_w, wall_h, wc);

            // Wall edge highlight
            uint32_t edge = darken(0xFF888888, depth);
            fill_rect(pixels, stride, screen_x, wall_top, 1, wall_h, edge);
            fill_rect(pixels, stride, screen_x + cell_w - 1, wall_top, 1, wall_h, edge);
        } else {
            // Floor
            uint32_t fc = darken(floor_colors[tex], depth);
            fill_rect(pixels, stride, screen_x, half_h, cell_w, wall_h / 2, fc);

            // Ceiling
            uint32_t cc = darken(ceil_colors[tex], depth);
            fill_rect(pixels, stride, screen_x, wall_top, cell_w, wall_h / 2, cc);

            // Special cell overlays
            uint32_t overlay = 0;
            switch (cell) {
                case CELL_DOOR:        overlay = darken(door_color, depth); break;
                case CELL_DOOR_LOCKED: overlay = darken(locked_color, depth); break;
                case CELL_STAIRS_UP:
                case CELL_STAIRS_DOWN: overlay = darken(stairs_color, depth); break;
                case CELL_GENERATOR:   overlay = darken(gen_color, depth); break;
                case CELL_SHOP:        overlay = darken(shop_color, depth); break;
                default: break;
            }
            if (overlay) {
                int ow = cell_w / 2;
                int oh = wall_h / 3;
                fill_rect(pixels, stride,
                         screen_x + cell_w / 4, half_h - oh / 2,
                         ow, oh, overlay);
            }
        }

        // Side walls: check left and right neighbors
        int left_cx = cx - right_x;
        int left_cy = cy - right_y;
        int right_cx = cx + right_x;
        int right_cy = cy + right_y;

        if (cell != CELL_WALL) {
            if (get_cell(lvl, left_cx, left_cy) == CELL_WALL) {
                uint32_t sw = darken(wall_colors[tex] - 0x00101010, depth);
                fill_rect(pixels, stride, screen_x, wall_top, cell_w / 6, wall_h, sw);
            }
            if (get_cell(lvl, right_cx, right_cy) == CELL_WALL) {
                uint32_t sw = darken(wall_colors[tex] - 0x00101010, depth);
                fill_rect(pixels, stride,
                         screen_x + cell_w - cell_w / 6, wall_top,
                         cell_w / 6, wall_h, sw);
            }
        }
    }
}

void viewport_render(const GameState *gs, uint32_t *pixels, int stride) {
    // Clear viewport to black
    for (int y = 0; y < CAPTIVE_VIEWPORT_HEIGHT; y++)
        memset(&pixels[y * stride], 0, CAPTIVE_VIEWPORT_WIDTH * sizeof(uint32_t));

    // Render back-to-front (depth 3 = farthest, 0 = closest)
    for (int depth = 3; depth >= 0; depth--) {
        render_row(gs, pixels, stride, depth,
                   gs->party_x, gs->party_y, gs->party_dir);
    }
}
