#include "viewport.h"
#include "opencaptive.h"
#include "texture_atlas.h"
#include "combat.h"
#include <string.h>

// Flat-color fallback palettes
static const uint32_t wall_colors[] = {
    0xFF606080, 0xFF806040, 0xFF408040, 0xFF804040,
};
static const uint32_t floor_colors[] = {
    0xFF303040, 0xFF403020, 0xFF204020, 0xFF402020,
};
static const uint32_t ceil_colors[] = {
    0xFF404050, 0xFF504030, 0xFF305030, 0xFF503030,
};

static const uint32_t door_color   = 0xFF5555AA;
static const uint32_t locked_color = 0xFFAA5555;
static const uint32_t stairs_color = 0xFF55AA55;
static const uint32_t gen_color    = 0xFFFF4444;
static const uint32_t shop_color   = 0xFFFFFF44;

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

static void fill_rect_textured(uint32_t *pixels, int stride,
                               int x, int y, int w, int h,
                               const TextureAtlas *atlas, int sheet_id,
                               int reg_x, int reg_y, int reg_w, int reg_h,
                               int depth) {
    if (sheet_id < 0 || !atlas) return;

    int darken = depth;
    for (int ry = 0; ry < h; ry++) {
        int py = y + ry;
        if (py < 0 || py >= CAPTIVE_VIEWPORT_HEIGHT) continue;
        float v = (float)ry / h;
        for (int rx = 0; rx < w; rx++) {
            int px = x + rx;
            if (px < 0 || px >= CAPTIVE_VIEWPORT_WIDTH) continue;
            float u = (float)rx / w;

            uint32_t color = texture_sample(atlas, sheet_id,
                                            reg_x, reg_y, reg_w, reg_h, u, v);

            // Distance darkening
            if (darken > 0) {
                uint8_t r = ((color >> 16) & 0xFF) >> darken;
                uint8_t g = ((color >> 8) & 0xFF) >> darken;
                uint8_t b = (color & 0xFF) >> darken;
                color = 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
            }

            pixels[py * stride + px] = color;
        }
    }
}

static uint32_t darken(uint32_t color, int depth) {
    int shift = depth;
    uint8_t r = ((color >> 16) & 0xFF) >> shift;
    uint8_t g = ((color >> 8) & 0xFF) >> shift;
    uint8_t b = (color & 0xFF) >> shift;
    return 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static void render_row(const GameState *gs, uint32_t *pixels, int stride,
                       int depth, const TextureAtlas *atlas) {
    const DungeonLevel *lvl = &gs->levels[gs->current_level];

    int fwd_x = dir_dx[gs->party_dir];
    int fwd_y = dir_dy[gs->party_dir];
    int right_x = dir_dx[(gs->party_dir + 1) % 4];
    int right_y = dir_dy[(gs->party_dir + 1) % 4];

    int dist = 4 - depth;
    int ahead_x = gs->party_x + fwd_x * dist;
    int ahead_y = gs->party_y + fwd_y * dist;

    int vp_w = CAPTIVE_VIEWPORT_WIDTH;
    int vp_h = CAPTIVE_VIEWPORT_HEIGHT;
    int half_h = vp_h / 2;

    int wall_h = vp_h / (depth + 1);
    int wall_top = half_h - wall_h / 2;
    int cell_w = vp_w / (depth + 2);
    int lateral_range = (depth < 2) ? 2 : depth;

    bool use_textures = atlas && atlas->loaded;

    for (int lat = -lateral_range; lat <= lateral_range; lat++) {
        int cx = ahead_x + right_x * lat;
        int cy = ahead_y + right_y * lat;
        int screen_x = vp_w / 2 + lat * cell_w - cell_w / 2;

        CellType cell = get_cell(lvl, cx, cy);
        uint8_t tex = get_wall_tex(lvl, cx, cy);
        int wall_set = tex % 5;

        if (cell == CELL_WALL) {
            if (use_textures && atlas->wall_sheets[wall_set] >= 0) {
                // Textured wall face — use tile 0 for front walls
                TextureRegion reg = texture_get_wall(atlas, wall_set, 0);
                fill_rect_textured(pixels, stride, screen_x, wall_top, cell_w, wall_h,
                                   atlas, reg.sheet_id, reg.x, reg.y, reg.w, reg.h, depth);
            } else {
                fill_rect(pixels, stride, screen_x, wall_top, cell_w, wall_h,
                         darken(wall_colors[tex % 4], depth));
            }
            // Edge highlight
            uint32_t edge = darken(0xFF888888, depth);
            fill_rect(pixels, stride, screen_x, wall_top, 1, wall_h, edge);
            fill_rect(pixels, stride, screen_x + cell_w - 1, wall_top, 1, wall_h, edge);
        } else {
            // Floor
            if (use_textures && atlas->roof_sheet >= 0) {
                // Floor texture from ROOFS.PL5 bottom half
                fill_rect_textured(pixels, stride, screen_x, half_h, cell_w, wall_h / 2,
                                   atlas, atlas->roof_sheet, 0, 128, 160, 72, depth);
            } else {
                fill_rect(pixels, stride, screen_x, half_h, cell_w, wall_h / 2,
                         darken(floor_colors[tex % 4], depth));
            }

            // Ceiling
            if (use_textures && atlas->roof_sheet >= 0) {
                // Ceiling texture from ROOFS.PL5 top half
                fill_rect_textured(pixels, stride, screen_x, wall_top, cell_w, wall_h / 2,
                                   atlas, atlas->roof_sheet, 0, 0, 160, 64, depth);
            } else {
                fill_rect(pixels, stride, screen_x, wall_top, cell_w, wall_h / 2,
                         darken(ceil_colors[tex % 4], depth));
            }

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

            // Door texture
            if (use_textures && atlas->door_sheet >= 0 &&
                (cell == CELL_DOOR || cell == CELL_DOOR_LOCKED)) {
                int door_tile = (cell == CELL_DOOR_LOCKED) ? 1 : 0;
                int dw = 48, dh = wall_h * 2 / 3;
                int dx_pos = screen_x + (cell_w - dw) / 2;
                int dy_pos = half_h - dh;
                fill_rect_textured(pixels, stride, dx_pos, dy_pos, dw, dh,
                                   atlas, atlas->door_sheet,
                                   door_tile * 64, 0, 64, 128, depth);
            } else if (overlay) {
                int ow = cell_w / 2;
                int oh = wall_h / 3;
                fill_rect(pixels, stride,
                         screen_x + cell_w / 4, half_h - oh / 2,
                         ow, oh, overlay);
            }

            // Side walls
            int left_cx = cx - right_x;
            int left_cy = cy - right_y;
            int right_cx = cx + right_x;
            int right_cy = cy + right_y;

            if (get_cell(lvl, left_cx, left_cy) == CELL_WALL) {
                int sw = cell_w / 5;
                if (use_textures && atlas->wall_sheets[wall_set] >= 0) {
                    TextureRegion reg = texture_get_wall(atlas, wall_set, 1);
                    fill_rect_textured(pixels, stride, screen_x, wall_top, sw, wall_h,
                                       atlas, reg.sheet_id, reg.x, reg.y, reg.w, reg.h, depth);
                } else {
                    fill_rect(pixels, stride, screen_x, wall_top, sw, wall_h,
                             darken(wall_colors[tex % 4] - 0x00101010, depth));
                }
            }
            if (get_cell(lvl, right_cx, right_cy) == CELL_WALL) {
                int sw = cell_w / 5;
                if (use_textures && atlas->wall_sheets[wall_set] >= 0) {
                    TextureRegion reg = texture_get_wall(atlas, wall_set, 1);
                    fill_rect_textured(pixels, stride,
                                       screen_x + cell_w - sw, wall_top, sw, wall_h,
                                       atlas, reg.sheet_id,
                                       reg.x + reg.w - reg.w/4, reg.y, reg.w/4, reg.h, depth);
                } else {
                    fill_rect(pixels, stride,
                             screen_x + cell_w - sw, wall_top, sw, wall_h,
                             darken(wall_colors[tex % 4] - 0x00101010, depth));
                }
            }
        }
    }
}

static const uint32_t creature_colors[] = {
    [CREATURE_NONE]     = 0x00000000,
    [CREATURE_DRONE]    = 0xFF44AAFF,
    [CREATURE_GUARD]    = 0xFFFF8844,
    [CREATURE_TURRET]   = 0xFFAA4444,
    [CREATURE_ROBOT]    = 0xFF888888,
    [CREATURE_ENFORCER] = 0xFFFF44FF,
    [CREATURE_BOSS]     = 0xFFFF2222,
};

static void render_creatures(const GameState *gs, uint32_t *pixels, int stride,
                             int depth, const TextureAtlas *atlas,
                             const CreatureList *creatures) {
    (void)atlas;
    if (!creatures) return;

    int fwd_x = dir_dx[gs->party_dir];
    int fwd_y = dir_dy[gs->party_dir];
    int right_x = dir_dx[(gs->party_dir + 1) % 4];
    int right_y = dir_dy[(gs->party_dir + 1) % 4];

    int dist = 4 - depth;
    int ahead_x = gs->party_x + fwd_x * dist;
    int ahead_y = gs->party_y + fwd_y * dist;

    int vp_w = CAPTIVE_VIEWPORT_WIDTH;
    int vp_h = CAPTIVE_VIEWPORT_HEIGHT;
    int half_h = vp_h / 2;
    int wall_h = vp_h / (depth + 1);
    int cell_w = vp_w / (depth + 2);
    int lateral_range = (depth < 2) ? 2 : depth;

    for (int i = 0; i < creatures->num_creatures; i++) {
        const Creature *c = &creatures->creatures[i];
        if (!c->active || c->level != gs->current_level) continue;

        // Check if creature is in this row
        int dx = c->x - ahead_x;
        int dy = c->y - ahead_y;
        int lat = dx * right_x + dy * right_y;
        int fwd = dx * fwd_x + dy * fwd_y;
        if (fwd != 0) continue;
        if (lat < -lateral_range || lat > lateral_range) continue;

        int screen_x = vp_w / 2 + lat * cell_w - cell_w / 2;
        int sprite_w = cell_w * 2 / 3;
        int sprite_h = wall_h * 2 / 3;
        int sx = screen_x + (cell_w - sprite_w) / 2;
        int sy = half_h - sprite_h;

        uint32_t color = darken(creature_colors[c->type], depth);

        // Draw as a simple shape: body rectangle + head circle approximation
        fill_rect(pixels, stride, sx, sy + sprite_h / 4, sprite_w, sprite_h * 3 / 4, color);
        // Head
        int head_w = sprite_w / 2;
        int head_h = sprite_h / 4;
        fill_rect(pixels, stride, sx + (sprite_w - head_w) / 2, sy, head_w, head_h, color);
        // Eyes
        uint32_t eye_color = darken(0xFFFF0000, depth);
        int ew = head_w / 4;
        if (ew < 1) ew = 1;
        fill_rect(pixels, stride, sx + sprite_w / 2 - head_w / 3, sy + head_h / 3, ew, ew, eye_color);
        fill_rect(pixels, stride, sx + sprite_w / 2 + head_w / 3 - ew, sy + head_h / 3, ew, ew, eye_color);

        // HP bar
        if (c->hp < c->hp_max) {
            int bar_w = sprite_w;
            int bar_h = 2;
            int bar_y = sy - 3;
            fill_rect(pixels, stride, sx, bar_y, bar_w, bar_h, darken(0xFF444444, depth));
            int fill_w = bar_w * c->hp / c->hp_max;
            fill_rect(pixels, stride, sx, bar_y, fill_w, bar_h, darken(0xFFFF0000, depth));
        }
    }
}

void viewport_render(const GameState *gs, uint32_t *pixels, int stride) {
    viewport_render_full(gs, pixels, stride, NULL, NULL);
}

void viewport_render_textured(const GameState *gs, uint32_t *pixels, int stride,
                              const TextureAtlas *atlas) {
    viewport_render_full(gs, pixels, stride, atlas, NULL);
}

void viewport_render_full(const GameState *gs, uint32_t *pixels, int stride,
                          const TextureAtlas *atlas, const CreatureList *creatures) {
    for (int y = 0; y < CAPTIVE_VIEWPORT_HEIGHT; y++)
        memset(&pixels[y * stride], 0, CAPTIVE_VIEWPORT_WIDTH * sizeof(uint32_t));

    for (int depth = 3; depth >= 0; depth--) {
        render_row(gs, pixels, stride, depth, atlas);
        render_creatures(gs, pixels, stride, depth, atlas, creatures);
    }
}
