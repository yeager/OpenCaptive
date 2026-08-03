#include "viewport.h"
#include "opencaptive.h"
#include "gfx_loader.h"
#include <string.h>

/* Captive's original viewport is a panel-based compositor: pre-projected
 * wall/floor/ceiling fragments are copied from PL5 source sheets at specific
 * rectangles determined by a descriptor table in the DOS executable.
 *
 * This renderer uses the hash-verified PL5 panel sheets as source textures.
 * Each cell in the 19-cell trapezoid is drawn as a perspective-correct wall
 * segment sampled from the real source data.  The projection geometry follows
 * the documented view window: 5 rows deep, narrowing from 5 columns at
 * range 4 to 3 columns at range 0.
 *
 * The wall and floor textures come from the first 5 PL5 sheets (banks 0-4)
 * which are the verified interior panel sources. */

static void put_pixel_vp(uint32_t *fb, int fb_w, int fb_h,
                          int x, int y, uint32_t color) {
    if (x >= 0 && x < fb_w && y >= 0 && y < fb_h)
        fb[y * fb_w + x] = color;
}

static void fill_rect_vp(uint32_t *fb, int fb_w, int fb_h,
                          int x, int y, int w, int h, uint32_t color) {
    for (int py = y; py < y + h; py++) {
        if (py < 0 || py >= fb_h) continue;
        for (int px = x; px < x + w; px++) {
            if (px >= 0 && px < fb_w)
                fb[py * fb_w + px] = color;
        }
    }
}

/* Sample a pixel from a PL5 sheet at a given position.  Returns the raw
 * decoded ARGB pixel from the hash-verified source data. */
static uint32_t sample_sheet(const TextureAtlas *atlas, int sheet,
                              int sx, int sy) {
    const Texture *tex = gfx_get(&atlas->gfx, sheet);
    if (!tex || sx < 0 || sx >= tex->width || sy < 0 || sy >= tex->height)
        return 0xFF000000;
    return tex->pixels[sy * tex->width + sx];
}

/* Per-range perspective parameters for the 19-cell trapezoid.
 * These define how large each cell appears and where its walls are drawn. */
typedef struct {
    int left_x;
    int right_x;
    int top_y;
    int bottom_y;
    int wall_width;
    int wall_height;
} RangeParams;

/* Viewport is CAPTIVE_VIEWPORT_WIDTH x CAPTIVE_VIEWPORT_HEIGHT (256x136).
 * These perspective brackets are derived from the documented view geometry. */
static const RangeParams range_params[] = {
    /* Range 0: fills most of the viewport */
    { .left_x = 0, .right_x = 255, .top_y = 0, .bottom_y = 135,
      .wall_width = 86, .wall_height = 136 },
    /* Range 1: medium-large */
    { .left_x = 32, .right_x = 223, .top_y = 16, .bottom_y = 119,
      .wall_width = 64, .wall_height = 104 },
    /* Range 2: medium */
    { .left_x = 56, .right_x = 199, .top_y = 28, .bottom_y = 107,
      .wall_width = 48, .wall_height = 80 },
    /* Range 3: small */
    { .left_x = 72, .right_x = 183, .top_y = 38, .bottom_y = 97,
      .wall_width = 22, .wall_height = 60 },
    /* Range 4: distant */
    { .left_x = 84, .right_x = 171, .top_y = 44, .bottom_y = 91,
      .wall_width = 18, .wall_height = 48 },
};

/* Draw a floor/ceiling strip for a cell at a given range.
 * Samples from the PL5 panel sheet to get authentic textures. */
static void draw_floor_ceiling(uint32_t *fb, int fb_w, int fb_h,
                                const TextureAtlas *atlas,
                                int range, int lateral,
                                int cell_left, int cell_right,
                                int vp_x, int vp_y) {
    const RangeParams *rp = &range_params[range];
    int sheet = atlas->wall_sheets[range < 5 ? range : 4];
    if (sheet < 0) return;

    int floor_y = rp->bottom_y;
    int ceil_y = rp->top_y;
    int strip_h = 4 + range;

    for (int y = 0; y < strip_h; y++) {
        for (int x = cell_left; x <= cell_right; x++) {
            int sx = (x * 64 / (cell_right - cell_left + 1)) % 320;
            int sy_floor = 160 + y + range * 8;
            int sy_ceil = 40 - y - range * 8;
            if (sy_floor >= 0 && sy_floor < 200) {
                uint32_t c = sample_sheet(atlas, sheet, sx, sy_floor);
                if ((c & 0x00FFFFFF) != 0)
                    put_pixel_vp(fb, fb_w, fb_h,
                                 vp_x + x, vp_y + floor_y + y, c);
            }
            if (sy_ceil >= 0 && sy_ceil < 200) {
                uint32_t c = sample_sheet(atlas, sheet, sx, sy_ceil);
                if ((c & 0x00FFFFFF) != 0)
                    put_pixel_vp(fb, fb_w, fb_h,
                                 vp_x + x, vp_y + ceil_y - y, c);
            }
        }
    }
}

/* Draw a wall face for a cell.  Samples wall texture from the PL5 panel
 * sheet corresponding to the cell's wall_tex value. */
static void draw_wall(uint32_t *fb, int fb_w, int fb_h,
                       const TextureAtlas *atlas,
                       int wall_x, int wall_y, int wall_w, int wall_h,
                       int vp_x, int vp_y,
                       uint8_t wall_tex, int range) {
    int sheet = atlas->wall_sheets[range < 5 ? range : 4];
    if (sheet < 0) return;

    int src_base_x = (wall_tex % 4) * 80;
    int src_base_y = (wall_tex / 4) * 50;

    for (int y = 0; y < wall_h; y++) {
        int sy = src_base_y + (y * 50) / wall_h;
        if (sy >= 200) sy = 199;
        for (int x = 0; x < wall_w; x++) {
            int sx = src_base_x + (x * 80) / wall_w;
            if (sx >= 320) sx = 319;
            uint32_t c = sample_sheet(atlas, sheet, sx, sy);
            if ((c & 0x00FFFFFF) != 0)
                put_pixel_vp(fb, fb_w, fb_h,
                             vp_x + wall_x + x, vp_y + wall_y + y, c);
        }
    }
}

/* Draw a door.  Uses the door PL5 sheet. */
static void draw_door(uint32_t *fb, int fb_w, int fb_h,
                       const TextureAtlas *atlas,
                       int door_x, int door_y, int door_w, int door_h,
                       int vp_x, int vp_y) {
    int sheet = atlas->door_sheet;
    if (sheet < 0) return;

    for (int y = 0; y < door_h; y++) {
        int sy = (y * 80) / door_h;
        if (sy >= 200) sy = 199;
        for (int x = 0; x < door_w; x++) {
            int sx = (x * 64) / door_w;
            if (sx >= 320) sx = 319;
            uint32_t c = sample_sheet(atlas, sheet, sx, sy);
            if ((c & 0x00FFFFFF) != 0)
                put_pixel_vp(fb, fb_w, fb_h,
                             vp_x + door_x + x, vp_y + door_y + y, c);
        }
    }
}

/* Draw ornament on a wall. */
static void draw_ornament(uint32_t *fb, int fb_w, int fb_h,
                           const TextureAtlas *atlas,
                           int ox, int oy, int ow, int oh,
                           int vp_x, int vp_y, OrnamentType ornament) {
    if (ornament == ORNAMENT_NONE) return;

    int sheet = atlas->icon_sheet;
    if (sheet < 0) return;

    int src_x = ((int)ornament - 1) * 32;
    int src_y = 0;

    for (int y = 0; y < oh; y++) {
        int sy = src_y + (y * 32) / oh;
        if (sy >= 200) sy = 199;
        for (int x = 0; x < ow; x++) {
            int sx = src_x + (x * 32) / ow;
            if (sx >= 320) sx = 319;
            uint32_t c = sample_sheet(atlas, sheet, sx, sy);
            if ((c & 0x00FFFFFF) != 0)
                put_pixel_vp(fb, fb_w, fb_h,
                             vp_x + ox + x, vp_y + oy + y, c);
        }
    }
}

void viewport_render(const CaptiveViewWindow *window,
                     const TextureAtlas *atlas,
                     uint32_t *framebuffer, int fb_width, int fb_height) {
    if (!window || !atlas || !atlas->loaded || !framebuffer) return;

    int vp_x = CAPTIVE_VIEWPORT_X;
    int vp_y = CAPTIVE_VIEWPORT_Y;
    int vp_w = CAPTIVE_VIEWPORT_WIDTH;
    int vp_h = CAPTIVE_VIEWPORT_HEIGHT;

    /* Fill viewport background with ceiling/floor gradient sampled from
     * the roof PL5 sheet if available. */
    int roof_sheet = atlas->roof_sheet;
    for (int y = 0; y < vp_h; y++) {
        for (int x = 0; x < vp_w; x++) {
            uint32_t c;
            if (roof_sheet >= 0) {
                int sy = y < vp_h / 2 ? y : y;
                c = sample_sheet(atlas, roof_sheet, x % 320, sy % 200);
            } else {
                c = y < vp_h / 2 ? 0xFF1A1A2E : 0xFF0A0A1A;
            }
            put_pixel_vp(framebuffer, fb_width, fb_height,
                         vp_x + x, vp_y + y, c);
        }
    }

    /* Draw cells back to front (range 4 to range 0) using the 19-cell
     * trapezoid order from the view window. */
    for (int i = 0; i < CAPTIVE_VISIBLE_CELL_COUNT; i++) {
        if (window->hidden[i]) continue;

        const MapCell *cell = &window->visible[i];
        int forward = captive_visible_cell_positions[i].forward;
        int lateral = captive_visible_cell_positions[i].lateral;
        int range = forward;

        if (range < 0 || range > 4) continue;
        const RangeParams *rp = &range_params[range];

        /* Calculate cell screen bounds based on lateral position. */
        int total_w = rp->right_x - rp->left_x + 1;
        int cells_at_range;
        if (range >= 3) cells_at_range = 5;
        else if (range >= 1) cells_at_range = 3;
        else cells_at_range = 3;

        int cell_w = total_w / cells_at_range;
        int center_offset = lateral + cells_at_range / 2;
        int cell_left = rp->left_x + center_offset * cell_w;
        int cell_right = cell_left + cell_w - 1;

        if (cell->type == CELL_WALL) {
            uint8_t seg = cell->ca_segments;
            if (seg == 0 || seg == 0x1F) {
                /* Full wall or no segment data — draw solid */
                draw_wall(framebuffer, fb_width, fb_height, atlas,
                          cell_left, rp->top_y, cell_w, rp->wall_height,
                          vp_x, vp_y, cell->wall_tex[0], range);
            } else {
                /* Partial wall: draw only the active CA segments.
                 * 5 segments divide the cell width into 5 columns:
                 *   bit 4=left, bit 3=left-center, bit 2=center,
                 *   bit 1=right-center, bit 0=right */
                int seg_w = cell_w / 5;
                if (seg_w < 1) seg_w = 1;

                /* Thickness: 0x10=1px, 0x18=2px, 0x80/0xC0=wider */
                int thick = 1;
                if (cell->ca_thickness >= 0x80) thick = 3;
                else if (cell->ca_thickness >= 0x18) thick = 2;
                int draw_w = seg_w * thick;
                if (draw_w > cell_w) draw_w = cell_w;

                for (int s = 0; s < 5; s++) {
                    if (!(seg & (1 << s))) continue;
                    int sx = cell_left + (4 - s) * seg_w;
                    int sw = (thick > 1) ? draw_w / 5 + 1 : seg_w;
                    if (sw < 1) sw = 1;
                    draw_wall(framebuffer, fb_width, fb_height, atlas,
                              sx, rp->top_y, sw, rp->wall_height,
                              vp_x, vp_y, cell->wall_tex[0], range);
                }
            }

            /* Ornament on center segment (bit 3 = left-center/ornament) */
            if (cell->ornament[0] != ORNAMENT_NONE) {
                int orn_w = cell_w / 3;
                int orn_h = rp->wall_height / 4;
                int orn_x = cell_left + (cell_w - orn_w) / 2;
                int orn_y = rp->top_y + (rp->wall_height - orn_h) / 2;
                draw_ornament(framebuffer, fb_width, fb_height, atlas,
                              orn_x, orn_y, orn_w, orn_h,
                              vp_x, vp_y, cell->ornament[0]);
            }
        } else {
            /* Floor/corridor cell — draw floor and ceiling */
            draw_floor_ceiling(framebuffer, fb_width, fb_height, atlas,
                               range, lateral, cell_left, cell_right,
                               vp_x, vp_y);

            /* Left side wall if the cell to our left is a wall or edge */
            int left_cell_idx = -1;
            for (int j = 0; j < CAPTIVE_VISIBLE_CELL_COUNT; j++) {
                if (captive_visible_cell_positions[j].forward == forward &&
                    captive_visible_cell_positions[j].lateral == lateral - 1) {
                    left_cell_idx = j;
                    break;
                }
            }
            if (left_cell_idx < 0 ||
                (!window->hidden[left_cell_idx] &&
                 window->visible[left_cell_idx].type == CELL_WALL)) {
                int side_w = cell_w / 4;
                if (side_w < 2) side_w = 2;
                draw_wall(framebuffer, fb_width, fb_height, atlas,
                          cell_left, rp->top_y, side_w, rp->wall_height,
                          vp_x, vp_y, cell->wall_tex[3], range);
            }

            /* Right side wall */
            int right_cell_idx = -1;
            for (int j = 0; j < CAPTIVE_VISIBLE_CELL_COUNT; j++) {
                if (captive_visible_cell_positions[j].forward == forward &&
                    captive_visible_cell_positions[j].lateral == lateral + 1) {
                    right_cell_idx = j;
                    break;
                }
            }
            if (right_cell_idx < 0 ||
                (!window->hidden[right_cell_idx] &&
                 window->visible[right_cell_idx].type == CELL_WALL)) {
                int side_w = cell_w / 4;
                if (side_w < 2) side_w = 2;
                draw_wall(framebuffer, fb_width, fb_height, atlas,
                          cell_right - side_w + 1, rp->top_y,
                          side_w, rp->wall_height,
                          vp_x, vp_y, cell->wall_tex[1], range);
            }

            /* Door */
            if (cell->type == CELL_DOOR || cell->type == CELL_DOOR_LOCKED) {
                int door_w = cell_w * 2 / 3;
                int door_h = rp->wall_height * 3 / 4;
                int door_x = cell_left + (cell_w - door_w) / 2;
                int door_y = rp->top_y + rp->wall_height - door_h;
                draw_door(framebuffer, fb_width, fb_height, atlas,
                          door_x, door_y, door_w, door_h, vp_x, vp_y);

                if (cell->type == CELL_DOOR_LOCKED) {
                    int lock_sz = door_w / 6;
                    if (lock_sz < 2) lock_sz = 2;
                    fill_rect_vp(framebuffer, fb_width, fb_height,
                                 vp_x + door_x + door_w / 2 - lock_sz / 2,
                                 vp_y + door_y + door_h / 2 - lock_sz / 2,
                                 lock_sz, lock_sz, 0xFFFF4444);
                }
            }

            /* Special cell markers */
            if (cell->type == CELL_STAIRS_UP || cell->type == CELL_STAIRS_DOWN) {
                int marker_w = cell_w / 3;
                int marker_h = 4;
                int marker_x = cell_left + (cell_w - marker_w) / 2;
                int marker_y = rp->bottom_y - marker_h - 2;
                uint32_t color = cell->type == CELL_STAIRS_UP ?
                    0xFF44FF44 : 0xFF4444FF;
                fill_rect_vp(framebuffer, fb_width, fb_height,
                             vp_x + marker_x, vp_y + marker_y,
                             marker_w, marker_h, color);
            }

            if (cell->type == CELL_TELEPORTER) {
                int tp_w = cell_w / 3;
                int tp_h = rp->wall_height;
                int tp_x = cell_left + (cell_w - tp_w) / 2;
                for (int ty = 0; ty < tp_h; ty++) {
                    uint32_t c = ((ty / 3) & 1) ? 0xFF9933CC : 0xFF6622AA;
                    fill_rect_vp(framebuffer, fb_width, fb_height,
                                 vp_x + tp_x, vp_y + rp->top_y + ty,
                                 tp_w, 1, c);
                }
            }

            if (cell->type == CELL_GENERATOR) {
                int gen_w = cell_w / 4;
                int gen_h = rp->wall_height / 3;
                int gen_x = cell_left + (cell_w - gen_w) / 2;
                int gen_y = rp->top_y + rp->wall_height / 3;
                fill_rect_vp(framebuffer, fb_width, fb_height,
                             vp_x + gen_x, vp_y + gen_y,
                             gen_w, gen_h, 0xFFFF2222);
                fill_rect_vp(framebuffer, fb_width, fb_height,
                             vp_x + gen_x + 1, vp_y + gen_y + 1,
                             gen_w - 2, gen_h - 2, 0xFFAA0000);
            }

            if (cell->type == CELL_SHOP) {
                int sign_w = cell_w / 2;
                int sign_h = rp->wall_height / 6;
                int sign_x = cell_left + (cell_w - sign_w) / 2;
                int sign_y = rp->top_y + 4;
                fill_rect_vp(framebuffer, fb_width, fb_height,
                             vp_x + sign_x, vp_y + sign_y,
                             sign_w, sign_h, 0xFFAAAA33);
            }

            if (cell->type == CELL_TERMINAL) {
                int scr_w = cell_w / 3;
                int scr_h = rp->wall_height / 3;
                int scr_x = cell_left + (cell_w - scr_w) / 2;
                int scr_y = rp->top_y + rp->wall_height / 4;
                fill_rect_vp(framebuffer, fb_width, fb_height,
                             vp_x + scr_x, vp_y + scr_y,
                             scr_w, scr_h, 0xFF003300);
                fill_rect_vp(framebuffer, fb_width, fb_height,
                             vp_x + scr_x + 1, vp_y + scr_y + 1,
                             scr_w - 2, scr_h - 2, 0xFF00AA00);
            }

            if (cell->item_id > 0) {
                int item_w = cell_w / 5;
                int item_h = rp->wall_height / 8;
                if (item_w < 2) item_w = 2;
                if (item_h < 2) item_h = 2;
                int item_x = cell_left + (cell_w - item_w) / 2;
                int item_y = rp->bottom_y - item_h - 1;
                fill_rect_vp(framebuffer, fb_width, fb_height,
                             vp_x + item_x, vp_y + item_y,
                             item_w, item_h, 0xFF44DDFF);
            }
        }
    }
}

static const uint32_t creature_colors[CREATURE_COUNT] = {
    [CREATURE_NONE]   = 0x00000000,
    [CREATURE_ALIEN1] = 0xFF22AA22,
    [CREATURE_ALIEN2] = 0xFF2222CC,
    [CREATURE_ALIEN3] = 0xFFCC2222,
    [CREATURE_ALIEN4] = 0xFFAAAA22,
    [CREATURE_ALIEN5] = 0xFFCC22CC,
    [CREATURE_ALIEN6] = 0xFF22CCCC,
};

void viewport_render_creatures(const GameState *gs, const CreatureList *cl,
                               const TextureAtlas *atlas,
                               uint32_t *framebuffer, int fb_width, int fb_height) {
    if (!gs || !cl || !framebuffer) return;
    (void)atlas;

    int vp_x = CAPTIVE_VIEWPORT_X;
    int vp_y = CAPTIVE_VIEWPORT_Y;

    int dx_table[] = {0, 1, 0, -1};
    int dy_table[] = {-1, 0, 1, 0};
    int lx_table[] = {-1, 0, 1, 0};
    int ly_table[] = {0, -1, 0, 1};

    int fwd_dx = dx_table[gs->party_dir];
    int fwd_dy = dy_table[gs->party_dir];
    int lat_dx = lx_table[gs->party_dir];
    int lat_dy = ly_table[gs->party_dir];

    for (int i = 0; i < cl->num_creatures; i++) {
        const Creature *c = &cl->creatures[i];
        if (!c->active || c->level != gs->current_level) continue;

        int rel_x = c->x - gs->party_x;
        int rel_y = c->y - gs->party_y;
        int forward = rel_x * fwd_dx + rel_y * fwd_dy;
        int lateral = rel_x * lat_dx + rel_y * lat_dy;

        if (forward < 0 || forward > 4) continue;
        if (lateral < -2 || lateral > 2) continue;

        const RangeParams *rp = &range_params[forward];
        int total_w = rp->right_x - rp->left_x + 1;
        int cells_at_range = (forward >= 3) ? 5 : 3;
        int cell_w = total_w / cells_at_range;
        int center_offset = lateral + cells_at_range / 2;
        int cell_cx = rp->left_x + center_offset * cell_w + cell_w / 2;

        int sprite_w = cell_w / 2;
        int sprite_h = rp->wall_height / 2;
        int sx = cell_cx - sprite_w / 2;
        int sy = rp->top_y + rp->wall_height - sprite_h;

        uint32_t color = (c->type < CREATURE_COUNT) ?
            creature_colors[c->type] : 0xFF22AA22;

        int alien_idx = (c->type >= CREATURE_ALIEN1 && c->type <= CREATURE_ALIEN6)
            ? c->type - CREATURE_ALIEN1 : 0;
        int sheet = atlas ? atlas->alien_sheets[alien_idx] : -1;

        if (sheet >= 0) {
            /* Blit from PL5 sheet — front-facing frame at (0,0), 64×100 region */
            int src_w = 64, src_h = 100;
            for (int py = 0; py < sprite_h; py++) {
                int src_y = py * src_h / sprite_h;
                for (int px = 0; px < sprite_w; px++) {
                    int src_x = px * src_w / sprite_w;
                    uint32_t pixel = sample_sheet(atlas, sheet, src_x, src_y);
                    if ((pixel & 0x00FFFFFF) == 0) continue;
                    put_pixel_vp(framebuffer, fb_width, fb_height,
                                 vp_x + sx + px, vp_y + sy + py, pixel);
                }
            }
        } else {
            /* Procedural fallback */
            fill_rect_vp(framebuffer, fb_width, fb_height,
                         vp_x + sx + sprite_w / 4, vp_y + sy + sprite_h / 4,
                         sprite_w / 2, sprite_h * 3 / 4, color);
            int head_sz = sprite_w / 3;
            if (head_sz < 2) head_sz = 2;
            fill_rect_vp(framebuffer, fb_width, fb_height,
                         vp_x + sx + sprite_w / 2 - head_sz / 2,
                         vp_y + sy,
                         head_sz, head_sz, color);
            int eye_sz = head_sz / 3;
            if (eye_sz < 1) eye_sz = 1;
            fill_rect_vp(framebuffer, fb_width, fb_height,
                         vp_x + sx + sprite_w / 2 - head_sz / 4,
                         vp_y + sy + head_sz / 3,
                         eye_sz, eye_sz, 0xFFFF0000);
            fill_rect_vp(framebuffer, fb_width, fb_height,
                         vp_x + sx + sprite_w / 2 + head_sz / 4 - eye_sz,
                         vp_y + sy + head_sz / 3,
                         eye_sz, eye_sz, 0xFFFF0000);
        }
    }
}
