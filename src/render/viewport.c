#include "viewport.h"
#include "opencaptive.h"
#include "gfx_loader.h"
#include "object_sprite.h"
#include "creature_stats.h"
#include "creature_sprite.h"
#include "captive_viewport_descriptors.h"
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

/* Object sprite frame indices in OBJECTS.PL5 (20-column, 16x16 grid).
 * Mapped from the original sprite sheet layout. */
#define OBJ_STAIRS_UP    0
#define OBJ_STAIRS_DOWN  1
#define OBJ_TELEPORTER   2
#define OBJ_GENERATOR    3
#define OBJ_SHOP_SIGN    4
#define OBJ_TERMINAL     5
#define OBJ_PIT          6
#define OBJ_PRESSURE     7
#define OBJ_FLOOR_ITEM   8

static void blit_object_scaled(const TextureAtlas *atlas, int frame_idx,
                               uint32_t *fb, int fb_w, int fb_h,
                               int dst_x, int dst_y, int dst_w, int dst_h) {
    if (!atlas || !fb || fb_w <= 0 || fb_h <= 0 ||
        dst_w <= 0 || dst_h <= 0 || frame_idx < 0 ||
        atlas->object_sheet < 0) return;
    const Texture *tex = gfx_get(&atlas->gfx, atlas->object_sheet);
    if (!tex) return;
    int col = frame_idx % OBJECT_COLS;
    int row = frame_idx / OBJECT_COLS;
    int src_x0 = col * OBJECT_FRAME_W;
    int src_y0 = row * OBJECT_FRAME_H;
    for (int dy = 0; dy < dst_h; dy++) {
        int sy = src_y0 + dy * OBJECT_FRAME_H / dst_h;
        if (sy >= tex->height) continue;
        int py = dst_y + dy;
        if (py < 0 || py >= fb_h) continue;
        for (int dx = 0; dx < dst_w; dx++) {
            int sx = src_x0 + dx * OBJECT_FRAME_W / dst_w;
            if (sx >= tex->width) continue;
            int px = dst_x + dx;
            if (px < 0 || px >= fb_w) continue;
            if (tex->indices[sy * tex->width + sx] == 0) continue;
            uint32_t c = tex->pixels[sy * tex->width + sx];
            fb[py * fb_w + px] = c;
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

/* PL5 transparency is an index property, not an RGB property. Indices 16
 * and 18 are both opaque black in the Captive palette. */
static uint8_t sample_sheet_index(const TextureAtlas *atlas, int sheet,
                                  int sx, int sy) {
    const Texture *tex = atlas ? gfx_get(&atlas->gfx, sheet) : NULL;
    if (!tex || sx < 0 || sx >= tex->width || sy < 0 || sy >= tex->height)
        return 0;
    return tex->indices[sy * tex->width + sx];
}

/* Map a local floor/ceiling coordinate to a source position in the PL5
 * sheet.  The texture index selects a 80x50 tile within the 320x200 sheet
 * (4 columns x 4 rows). */
static void panel_tile_position(uint8_t texture, int local_x, int local_y,
                                int *sx, int *sy) {
    if (!sx || !sy) return;
    *sx = ((int)(texture % 4) * 80) + (local_x % 80);
    *sy = ((int)(texture / 4) * 50) + (local_y % 50);
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

/* Viewport is CAPTIVE_VIEWPORT_WIDTH x CAPTIVE_VIEWPORT_HEIGHT (144x112).
 * The vertical bands below are the recovered CAPPO panel coordinates, not
 * estimates: descriptor records 17-22, 11-16, 5-10 and 43-52 place the
 * active wall bands at y=9/25/37/45 with heights 98/70/49/35. */
static const RangeParams range_params[] = {
    /* Range 0: fills most of the viewport */
    { .left_x = 0, .right_x = 143, .top_y = 0, .bottom_y = 111,
      .wall_width = 48, .wall_height = 112 },
    /* Range 1: medium-large */
    { .left_x = 18, .right_x = 125, .top_y = 9, .bottom_y = 105,
      .wall_width = 36, .wall_height = 98 },
    /* Range 2: medium */
    { .left_x = 32, .right_x = 111, .top_y = 25, .bottom_y = 93,
      .wall_width = 27, .wall_height = 70 },
    /* Range 3: small */
    { .left_x = 40, .right_x = 103, .top_y = 37, .bottom_y = 84,
      .wall_width = 12, .wall_height = 49 },
    /* Range 4: distant */
    { .left_x = 47, .right_x = 96, .top_y = 45, .bottom_y = 79,
      .wall_width = 10, .wall_height = 35 },
};

/* Draw a floor/ceiling strip for a cell at a given range.
 * Samples from the PL5 panel sheet to get authentic textures. */
static void draw_floor_ceiling(uint32_t *fb, int fb_w, int fb_h,
                                const TextureAtlas *atlas,
                                int range, int lateral,
                                int cell_left, int cell_right,
                                uint8_t floor_tex, uint8_t ceil_tex,
                                int vp_x, int vp_y) {
    (void)lateral;
    const RangeParams *rp = &range_params[range];
    /* CAPPO descriptor records route floor and ceiling strips through source
     * bank 4, which resolves to ROOFS.PL5.  Range selects the descriptor
     * geometry, not the source sheet. */
    int sheet = atlas->roof_sheet;
    if (sheet < 0) return;

    int floor_y = rp->bottom_y;
    int ceil_y = rp->top_y;
    int strip_h = 4 + range;

    for (int y = 0; y < strip_h; y++) {
        for (int x = cell_left; x <= cell_right; x++) {
            /* Project each cell from its own local origin.  Using the
             * viewport-relative x coordinate here shifted the PL5 sample
             * phase for lateral cells and produced visible floor/ceiling
             * seams when otherwise identical cells were adjacent. */
            int local_x = (x - cell_left) * 64 /
                          (cell_right - cell_left + 1);
            int sy_floor = 160 + y + range * 8;
            int sy_ceil = 40 - y - range * 8;
            if (sy_floor >= 0 && sy_floor < 200) {
                int sx, sy;
                panel_tile_position(floor_tex, local_x, sy_floor, &sx, &sy);
                uint32_t c = sample_sheet(atlas, sheet, sx, sy);
                if (sample_sheet_index(atlas, sheet, sx, sy) != 0)
                    put_pixel_vp(fb, fb_w, fb_h,
                                 vp_x + x, vp_y + floor_y + y, c);
            }
            if (sy_ceil >= 0 && sy_ceil < 200) {
                int sx, sy;
                panel_tile_position(ceil_tex, local_x, sy_ceil, &sx, &sy);
                uint32_t c = sample_sheet(atlas, sheet, sx, sy);
                if (sample_sheet_index(atlas, sheet, sx, sy) != 0)
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
    /* Map generation stores the selected WALLA-WALLE set in wall_tex.  The
     * view range changes projection size, not the source wall set. */
    (void)range;
    int wall_set = wall_tex < 5U ? wall_tex : 4;
    int sheet = atlas->wall_sheets[wall_set];
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
            if (sample_sheet_index(atlas, sheet, sx, sy) != 0)
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
            if (sample_sheet_index(atlas, sheet, sx, sy) != 0)
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
            if (sample_sheet_index(atlas, sheet, sx, sy) != 0)
                put_pixel_vp(fb, fb_w, fb_h,
                             vp_x + ox + x, vp_y + oy + y, c);
        }
    }
}

static const MapCell *visible_cell_at(const CaptiveViewWindow *window,
                                      int forward, int lateral) {
    if (!window) return NULL;
    for (int i = 0; i < CAPTIVE_VISIBLE_CELL_COUNT; i++) {
        if (captive_visible_cell_positions[i].forward == forward &&
            captive_visible_cell_positions[i].lateral == lateral)
            return &window->visible[i];
    }
    return NULL;
}

int viewport_descriptor_source_sheet(const TextureAtlas *atlas,
                                     uint8_t source_bank) {
    if (!atlas) return -1;
    if (source_bank < 4U)
        return atlas->wall_sheets[source_bank];
    if (source_bank == 4U)
        return atlas->roof_sheet;
    return -1;
}

/* Blit one descriptor panel from a decoded PL5 sheet into the viewport
 * work buffer.  source_offset is in packed PL5 byte space (200 bytes/row);
 * we convert to decoded pixel coordinates (320 pixels/row). */
static void descriptor_blit_sheet(const CaptiveDosDescriptor *desc,
                                  const TextureAtlas *atlas,
                                  int source_sheet,
                                  uint32_t *vp_buf, int vp_stride) {
    if (!desc || !atlas || !vp_buf || desc->width_bytes == 0 || desc->height == 0)
        return;
    int sheet = source_sheet >= 0 ? source_sheet
        : viewport_descriptor_source_sheet(atlas, desc->source_bank);
    if (sheet < 0) return;
    const Texture *tex = gfx_get(&atlas->gfx, sheet);
    if (!tex || !tex->indices) return;

    int src_row = (int)(desc->source_offset / 200U);
    int src_col = (int)((desc->source_offset % 200U) * 8 / 5);
    int pixel_w = (int)desc->width_bytes * 8;
    bool mirror = (desc->flags & CAPTIVE_DESC_FLAG_MIRROR_H) != 0;
    bool mask_zero = (desc->flags & CAPTIVE_DESC_FLAG_MASK_ZERO) != 0;

    int dst_x, dst_y;
    if (desc->destination_offset >= CAPTIVE_DOS_VIEW_STRIDE * CAPTIVE_DOS_VIEW_HEIGHT)
        return;
    dst_x = (int)(desc->destination_offset % CAPTIVE_DOS_VIEW_STRIDE);
    dst_y = (int)(desc->destination_offset / CAPTIVE_DOS_VIEW_STRIDE);

    for (int y = 0; y < (int)desc->height; y++) {
        int sy = src_row + y;
        int dy = dst_y + y;
        if (sy < 0 || sy >= tex->height || dy < 0 || dy >= (int)CAPTIVE_DOS_VIEW_HEIGHT)
            continue;
        for (int x = 0; x < pixel_w; x++) {
            int sx = mirror ? src_col + pixel_w - 1 - x : src_col + x;
            if (sx < 0 || sx >= tex->width) continue;
            int dx = dst_x + x;
            if (dx < 0 || dx >= CAPTIVE_VIEWPORT_WIDTH) continue;
            uint8_t idx = tex->indices[sy * tex->width + sx];
            if (mask_zero && idx == 0) continue;
            vp_buf[dy * vp_stride + dx] = tex->pixels[sy * tex->width + sx];
        }
    }
}

static void descriptor_blit(const CaptiveDosDescriptor *desc,
                            const TextureAtlas *atlas,
                            uint32_t *vp_buf, int vp_stride) {
    descriptor_blit_sheet(desc, atlas, -1, vp_buf, vp_stride);
}

static int original_wall_descriptor(int range, int lateral) {
    /* These are the fixed panel bands recovered from CAPPO's descriptor
     * table.  The source selector for the individual wall graphic is still
     * carried by the verified PL5 bank; no procedural wall is introduced. */
    static const int groups[4][3][2] = {
        {{17, 18}, {19, 20}, {21, 22}},
        {{11, 12}, {13, 14}, {15, 16}},
        {{5, 6}, {7, 8}, {9, 10}},
        {{43, 44}, {47, 48}, {51, 52}},
    };
    int band = 3 - range;
    if (band < 0 || band >= 4) return -1;
    int side = lateral < 0 ? 0 : lateral > 0 ? 2 : 1;
    return groups[band][side][0];
}

static int original_floor_descriptor(int range, int lateral, bool ceiling) {
    static const int floor_groups[4][3] = {
        {75, 77, 79}, {69, 71, 73}, {63, 65, 67}, {53, 57, 61}
    };
    static const int ceiling_groups[4][3] = {
        {103, 105, 107}, {97, 99, 101}, {91, 93, 95}, {81, 85, 89}
    };
    int band = 3 - range;
    if (band < 0 || band >= 4) return -1;
    int side = lateral < 0 ? 0 : lateral > 0 ? 2 : 1;
    return (ceiling ? ceiling_groups : floor_groups)[band][side];
}

void viewport_render_original_descriptors(const CaptiveViewWindow *window,
                                          const TextureAtlas *atlas,
                                          uint32_t *framebuffer,
                                          int fb_width, int fb_height) {
    if (!window || !atlas || !atlas->loaded || !framebuffer ||
        fb_width <= 0 || fb_height <= 0)
        return;

    uint32_t work[CAPTIVE_DOS_VIEW_STRIDE * CAPTIVE_DOS_VIEW_HEIGHT];
    memset(work, 0, sizeof(work));

    /* CAPPO builds the view back-to-front.  The full panel pair supplies the
     * authentic room background; depth-specific ceiling/floor and wall
     * panels then overwrite it through the original mask convention. */
    descriptor_blit(&captive_viewport_descriptors[3], atlas, work,
                    CAPTIVE_DOS_VIEW_STRIDE);
    descriptor_blit(&captive_viewport_descriptors[1], atlas, work,
                    CAPTIVE_DOS_VIEW_STRIDE);

    for (int range = 4; range >= 1; --range) {
        for (int lateral = -2; lateral <= 2; ++lateral) {
            int cell_index = -1;
            for (int i = 0; i < CAPTIVE_VISIBLE_CELL_COUNT; ++i) {
                if (captive_visible_cell_positions[i].forward == range &&
                    captive_visible_cell_positions[i].lateral == lateral) {
                    cell_index = i;
                    break;
                }
            }
            if (cell_index < 0 || window->hidden[cell_index]) continue;
            const MapCell *cell = &window->visible[cell_index];
            int floor_id = original_floor_descriptor(range, lateral, false);
            int ceiling_id = original_floor_descriptor(range, lateral, true);
            if (floor_id >= 0)
                descriptor_blit(&captive_viewport_descriptors[floor_id], atlas,
                                work, CAPTIVE_DOS_VIEW_STRIDE);
            if (ceiling_id >= 0)
                descriptor_blit(&captive_viewport_descriptors[ceiling_id], atlas,
                                work, CAPTIVE_DOS_VIEW_STRIDE);

            if (cell->type == CELL_WALL || cell->type == CELL_DOOR ||
                cell->type == CELL_DOOR_LOCKED) {
                int wall_id = original_wall_descriptor(range, lateral);
                if (wall_id >= 0) {
                    int face = (window->facing + 2) & 3;
                    int wall_set = cell->wall_tex[face] < 5U
                        ? cell->wall_tex[face] : 0;
                    descriptor_blit_sheet(
                        &captive_viewport_descriptors[wall_id], atlas,
                        atlas->wall_sheets[wall_set], work,
                        CAPTIVE_DOS_VIEW_STRIDE);
                }
            }
        }
    }

    /* The descriptor destination is in the original 160-byte work stride;
     * the VGA copy exposes the first 144 pixels at the documented viewport
     * origin. */
    for (int y = 0; y < CAPTIVE_VIEWPORT_HEIGHT &&
                    CAPTIVE_VIEWPORT_Y + y < fb_height; ++y) {
        for (int x = 0; x < CAPTIVE_VIEWPORT_WIDTH &&
                        CAPTIVE_VIEWPORT_X + x < fb_width; ++x) {
            framebuffer[(size_t)(CAPTIVE_VIEWPORT_Y + y) * (size_t)fb_width +
                        (size_t)(CAPTIVE_VIEWPORT_X + x)] =
                work[(size_t)y * CAPTIVE_DOS_VIEW_STRIDE + (size_t)x];
        }
    }
}

void viewport_render(const CaptiveViewWindow *window,
                     const TextureAtlas *atlas,
                     uint32_t *framebuffer, int fb_width, int fb_height) {
    if (!window || !atlas || !atlas->loaded || !framebuffer ||
        fb_width <= 0 || fb_height <= 0) return;

    int vp_x = CAPTIVE_VIEWPORT_X;
    int vp_y = CAPTIVE_VIEWPORT_Y;
    int vp_w = CAPTIVE_VIEWPORT_WIDTH;
    int vp_h = CAPTIVE_VIEWPORT_HEIGHT;

    /* Fill viewport background with ceiling/floor gradient sampled from
     * the roof PL5 sheet if available. */
    int roof_sheet = atlas->roof_sheet;
    for (int y = 0; y < vp_h; y++) {
        for (int x = 0; x < vp_w; x++) {
            /* Sampled from the original roof sheet only.  The old invented
             * two-tone gradient is gone: the atlas is hash-verified and
             * all-or-nothing, so roof_sheet is always present here. */
            if (roof_sheet < 0) continue;
            uint32_t c = sample_sheet(atlas, roof_sheet, x % 320, y % 200);
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
                          /* A solid wall cell is viewed from the side facing
                           * the party.  wall_tex is stored in map (N/E/S/W)
                           * order, so that face is opposite the party's
                           * viewing direction. */
                          vp_x, vp_y,
                          cell->wall_tex[(window->facing + 2) & 3], range);
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
                              vp_x, vp_y,
                              cell->wall_tex[(window->facing + 2) & 3], range);
                }
            }

            /* Wall ornaments belong to the adjacent floor cell, where the
             * map generator and Captive puzzle placement store them.  For a
             * frontal wall that cell is one step nearer to the party.  At
             * range zero, the visible side walls are backed by the party's
             * cell instead. */
            const MapCell *ornament_cell = NULL;
            int ornament_face = -1;
            if (forward > 0) {
                ornament_cell = visible_cell_at(window, forward - 1, lateral);
                ornament_face = window->facing;
            } else if (forward == 0 && (lateral == -1 || lateral == 1)) {
                ornament_cell = visible_cell_at(window, 0, 0);
                ornament_face = lateral < 0
                    ? (window->facing + 3) & 3 : (window->facing + 1) & 3;
            }
            OrnamentType ornament = ornament_cell && ornament_face >= 0
                ? ornament_cell->ornament[ornament_face] : ORNAMENT_NONE;
            if (ornament != ORNAMENT_NONE) {
                int orn_w = cell_w / 3;
                int orn_h = rp->wall_height / 4;
                int orn_x = cell_left + (cell_w - orn_w) / 2;
                int orn_y = rp->top_y + (rp->wall_height - orn_h) / 2;
                draw_ornament(framebuffer, fb_width, fb_height, atlas,
                              orn_x, orn_y, orn_w, orn_h,
                              vp_x, vp_y, ornament);
            }
        } else {
            /* Floor/corridor cell — draw floor and ceiling */
            draw_floor_ceiling(framebuffer, fb_width, fb_height, atlas,
                               range, lateral, cell_left, cell_right,
                               cell->floor_tex, cell->ceil_tex,
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
                /* The visible face belongs to the adjacent wall cell, not
                 * the floor cell being projected.  Facing north, for
                 * example, a left wall exposes its east face. */
                uint8_t side_wall_tex = cell->wall_tex[(window->facing + 3) & 3];
                if (left_cell_idx >= 0 &&
                    window->visible[left_cell_idx].type == CELL_WALL)
                    side_wall_tex = window->visible[left_cell_idx].wall_tex[
                        (window->facing + 1) & 3];
                draw_wall(framebuffer, fb_width, fb_height, atlas,
                          cell_left, rp->top_y, side_w, rp->wall_height,
                          vp_x, vp_y,
                          side_wall_tex, range);
                if (cell->ornament[(window->facing + 3) & 3] != ORNAMENT_NONE)
                    draw_ornament(framebuffer, fb_width, fb_height, atlas,
                                  cell_left, rp->top_y + rp->wall_height / 3,
                                  side_w, rp->wall_height / 4, vp_x, vp_y,
                                  cell->ornament[(window->facing + 3) & 3]);
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
                /* The right wall exposes the opposite side of its own map
                 * cell; use the floor cell only for a synthetic edge wall. */
                uint8_t side_wall_tex = cell->wall_tex[(window->facing + 1) & 3];
                if (right_cell_idx >= 0 &&
                    window->visible[right_cell_idx].type == CELL_WALL)
                    side_wall_tex = window->visible[right_cell_idx].wall_tex[
                        (window->facing + 3) & 3];
                draw_wall(framebuffer, fb_width, fb_height, atlas,
                          cell_right - side_w + 1, rp->top_y,
                          side_w, rp->wall_height,
                          vp_x, vp_y,
                          side_wall_tex, range);
                if (cell->ornament[(window->facing + 1) & 3] != ORNAMENT_NONE)
                    draw_ornament(framebuffer, fb_width, fb_height, atlas,
                                  cell_right - side_w + 1,
                                  rp->top_y + rp->wall_height / 3,
                                  side_w, rp->wall_height / 4, vp_x, vp_y,
                                  cell->ornament[(window->facing + 1) & 3]);
            }

            /* Door */
            /* A last-resort puzzle control may live on a completely open
             * floor cell.  Wall-backed ornaments are drawn from the wall
             * branch above; render this wall-less north-facing panel on the
             * floor surface instead of leaving the interaction invisible. */
            if (forward == 0 && lateral == 0 &&
                cell->ornament[window->facing] != ORNAMENT_NONE) {
                const MapCell *front = visible_cell_at(window, 1, 0);
                if (!front || front->type != CELL_WALL) {
                    int orn_w = cell_w / 3;
                    int orn_h = rp->wall_height / 5;
                    if (orn_w < 4) orn_w = 4;
                    if (orn_h < 4) orn_h = 4;
                    draw_ornament(framebuffer, fb_width, fb_height,
                                  atlas,
                                  cell_left + (cell_w - orn_w) / 2,
                                  rp->bottom_y - orn_h - 2,
                                  orn_w, orn_h, vp_x, vp_y,
                                  cell->ornament[window->facing]);
                }
            }

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

            /* Special cell objects — use OBJECTS.PL5 sprites when loaded */
            bool has_obj = atlas->object_sheet >= 0;

            if (cell->type == CELL_STAIRS_UP || cell->type == CELL_STAIRS_DOWN) {
                int obj_w = cell_w / 2;
                int obj_h = rp->wall_height / 3;
                if (obj_w < 4) obj_w = 4;
                if (obj_h < 4) obj_h = 4;
                int obj_x = cell_left + (cell_w - obj_w) / 2;
                int obj_y = rp->bottom_y - obj_h - 2;
                int fi = cell->type == CELL_STAIRS_UP ? OBJ_STAIRS_UP : OBJ_STAIRS_DOWN;
                if (has_obj) {
                    blit_object_scaled(atlas, fi, framebuffer, fb_width, fb_height,
                                       vp_x + obj_x, vp_y + obj_y, obj_w, obj_h);
                } else {
                    uint32_t color = cell->type == CELL_STAIRS_UP ? 0xFF44FF44 : 0xFF4444FF;
                    fill_rect_vp(framebuffer, fb_width, fb_height,
                                 vp_x + obj_x, vp_y + obj_y, obj_w, obj_h, color);
                }
            }

            if (cell->type == CELL_TELEPORTER) {
                int obj_w = cell_w / 3;
                int obj_h = rp->wall_height;
                int obj_x = cell_left + (cell_w - obj_w) / 2;
                if (has_obj) {
                    blit_object_scaled(atlas, OBJ_TELEPORTER, framebuffer, fb_width, fb_height,
                                       vp_x + obj_x, vp_y + rp->top_y, obj_w, obj_h);
                } else {
                    for (int ty = 0; ty < obj_h; ty++) {
                        uint32_t c = ((ty / 3) & 1) ? 0xFF9933CC : 0xFF6622AA;
                        fill_rect_vp(framebuffer, fb_width, fb_height,
                                     vp_x + obj_x, vp_y + rp->top_y + ty, obj_w, 1, c);
                    }
                }
            }

            if (cell->type == CELL_GENERATOR) {
                int obj_w = cell_w / 3;
                int obj_h = rp->wall_height / 2;
                int obj_x = cell_left + (cell_w - obj_w) / 2;
                int obj_y = rp->top_y + rp->wall_height / 4;
                if (has_obj) {
                    blit_object_scaled(atlas, OBJ_GENERATOR, framebuffer, fb_width, fb_height,
                                       vp_x + obj_x, vp_y + obj_y, obj_w, obj_h);
                } else {
                    fill_rect_vp(framebuffer, fb_width, fb_height,
                                 vp_x + obj_x, vp_y + obj_y, obj_w, obj_h, 0xFFFF2222);
                    fill_rect_vp(framebuffer, fb_width, fb_height,
                                 vp_x + obj_x + 1, vp_y + obj_y + 1,
                                 obj_w - 2, obj_h - 2, 0xFFAA0000);
                }
            }

            if (cell->type == CELL_SHOP) {
                int obj_w = cell_w / 2;
                int obj_h = rp->wall_height / 4;
                int obj_x = cell_left + (cell_w - obj_w) / 2;
                int obj_y = rp->top_y + 4;
                if (has_obj) {
                    blit_object_scaled(atlas, OBJ_SHOP_SIGN, framebuffer, fb_width, fb_height,
                                       vp_x + obj_x, vp_y + obj_y, obj_w, obj_h);
                } else {
                    fill_rect_vp(framebuffer, fb_width, fb_height,
                                 vp_x + obj_x, vp_y + obj_y, obj_w, obj_h, 0xFFAAAA33);
                }
            }

            if (cell->type == CELL_TERMINAL) {
                int obj_w = cell_w / 3;
                int obj_h = rp->wall_height / 3;
                int obj_x = cell_left + (cell_w - obj_w) / 2;
                int obj_y = rp->top_y + rp->wall_height / 4;
                if (has_obj) {
                    blit_object_scaled(atlas, OBJ_TERMINAL, framebuffer, fb_width, fb_height,
                                       vp_x + obj_x, vp_y + obj_y, obj_w, obj_h);
                } else {
                    fill_rect_vp(framebuffer, fb_width, fb_height,
                                 vp_x + obj_x, vp_y + obj_y, obj_w, obj_h, 0xFF003300);
                    fill_rect_vp(framebuffer, fb_width, fb_height,
                                 vp_x + obj_x + 1, vp_y + obj_y + 1,
                                 obj_w - 2, obj_h - 2, 0xFF00AA00);
                }
            }

            if (cell->type == CELL_PIT) {
                int obj_w = cell_w * 2 / 3;
                int obj_h = rp->wall_height / 4;
                int obj_x = cell_left + (cell_w - obj_w) / 2;
                int obj_y = rp->top_y + rp->wall_height - obj_h;
                if (has_obj) {
                    blit_object_scaled(atlas, OBJ_PIT, framebuffer, fb_width, fb_height,
                                       vp_x + obj_x, vp_y + obj_y, obj_w, obj_h);
                } else {
                    fill_rect_vp(framebuffer, fb_width, fb_height,
                                 vp_x + obj_x, vp_y + obj_y, obj_w, obj_h, 0xFF111111);
                }
            }

            if (cell->type == CELL_PRESSURE_PLATE) {
                int obj_w = cell_w / 3;
                int obj_h = rp->wall_height / 6;
                if (obj_h < 2) obj_h = 2;
                int obj_x = cell_left + (cell_w - obj_w) / 2;
                int obj_y = rp->top_y + rp->wall_height - obj_h;
                if (has_obj) {
                    blit_object_scaled(atlas, OBJ_PRESSURE, framebuffer, fb_width, fb_height,
                                       vp_x + obj_x, vp_y + obj_y, obj_w, obj_h);
                } else {
                    fill_rect_vp(framebuffer, fb_width, fb_height,
                                 vp_x + obj_x, vp_y + obj_y, obj_w, obj_h, 0xFF888844);
                }
            }

            if (cell->item_id > 0) {
                int obj_w = cell_w / 4;
                int obj_h = rp->wall_height / 4;
                if (obj_w < 4) obj_w = 4;
                if (obj_h < 4) obj_h = 4;
                int obj_x = cell_left + (cell_w - obj_w) / 2;
                int obj_y = rp->bottom_y - obj_h - 1;
                if (has_obj) {
                    blit_object_scaled(atlas, OBJ_FLOOR_ITEM, framebuffer, fb_width, fb_height,
                                       vp_x + obj_x, vp_y + obj_y, obj_w, obj_h);
                } else {
                    fill_rect_vp(framebuffer, fb_width, fb_height,
                                 vp_x + obj_x, vp_y + obj_y, obj_w, obj_h, 0xFF44DDFF);
                }
            }
        }
    }
}

void viewport_render_creatures(const GameState *gs, const CreatureList *cl,
                               const TextureAtlas *atlas,
                               uint32_t *framebuffer, int fb_width, int fb_height) {
    if (!gs || !cl || !framebuffer || fb_width <= 0 || fb_height <= 0) return;

    (void)atlas;

    CaptiveViewWindow view_window;
    captive_view_window_build(gs, &view_window);

    int vp_x = CAPTIVE_VIEWPORT_X;
    int vp_y = CAPTIVE_VIEWPORT_Y;

    int dx_table[] = {0, 1, 0, -1};
    int dy_table[] = {-1, 0, 1, 0};
    int lx_table[] = {-1, 0, 1, 0};
    int ly_table[] = {0, -1, 0, 1};

    int safe_dir = (gs->party_dir >= DIR_NORTH && gs->party_dir <= DIR_WEST)
        ? gs->party_dir : DIR_NORTH;
    int fwd_dx = dx_table[safe_dir];
    int fwd_dy = dy_table[safe_dir];
    int lat_dx = lx_table[safe_dir];
    int lat_dy = ly_table[safe_dir];

    int creature_count = cl->num_creatures;
    if (creature_count < 0) creature_count = 0;
    if (creature_count > MAX_CREATURES) creature_count = MAX_CREATURES;
    /* Painter's order matters when projected sprites overlap.  Captive's
     * original panel compositor is back-to-front, so sort only the active
     * current-level creatures by forward distance before drawing them.  Keep
     * list order for equal depths to retain deterministic behaviour. */
    int render_order[MAX_CREATURES];
    int render_forward[MAX_CREATURES];
    int render_count = 0;
    for (int i = 0; i < creature_count; i++) {
        const Creature *c = &cl->creatures[i];
        if (!c->active || c->level != gs->current_level ||
            c->x < 0 || c->x >= MAP_WIDTH || c->y < 0 || c->y >= MAP_HEIGHT)
            continue;
        int rel_x = c->x - gs->party_x;
        int rel_y = c->y - gs->party_y;
        int forward = rel_x * fwd_dx + rel_y * fwd_dy;
        if (forward < 0 || forward > 4) continue;
        int insert = render_count++;
        while (insert > 0 && render_forward[insert - 1] < forward) {
            render_order[insert] = render_order[insert - 1];
            render_forward[insert] = render_forward[insert - 1];
            --insert;
        }
        render_order[insert] = i;
        render_forward[insert] = forward;
    }

    for (int order = 0; order < render_count; order++) {
        const Creature *c = &cl->creatures[render_order[order]];
        if (!c->active || c->level != gs->current_level) continue;
        if (c->x < 0 || c->x >= MAP_WIDTH || c->y < 0 || c->y >= MAP_HEIGHT)
            continue;

        int rel_x = c->x - gs->party_x;
        int rel_y = c->y - gs->party_y;
        int forward = rel_x * fwd_dx + rel_y * fwd_dy;
        int lateral = rel_x * lat_dx + rel_y * lat_dy;

        if (forward < 0 || forward > 4) continue;
        if (lateral < -2 || lateral > 2) continue;

        int visible_index = -1;
        for (int vi = 0; vi < CAPTIVE_VISIBLE_CELL_COUNT; vi++) {
            if (captive_visible_cell_positions[vi].forward == forward &&
                captive_visible_cell_positions[vi].lateral == lateral) {
                visible_index = vi;
                break;
            }
        }
        if (visible_index < 0 || view_window.hidden[visible_index] ||
            view_window.visible[visible_index].type == CELL_WALL)
            continue;

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

        /* ReDMCSB/CAPPO disassembly: DS:0xA16E stores a graphic_id per
         * creature type.  The enum value is not the source-sheet selector;
         * types 7..24 deliberately reuse ALIEN2-5 resources. */
        int alien_idx = creature_sprite_sheet_index(c->type);
        int sheet = (atlas && alien_idx >= 0 && alien_idx < 6)
            ? atlas->alien_sheets[alien_idx] : -1;

        if (sheet >= 0) {
            /* CAPPO DS:0xA16E stores the animation frame as the second byte
             * after graphic_id. The old path always used frame zero, so
             * several creature types displayed the wrong pose. */
            int frame_x = 0, frame_y = 0;
            int frame_index = creature_sprite_frame_index(c->type);
            if (!creature_sprite_frame_origin(frame_index, &frame_x, &frame_y))
                continue;
            for (int py = 0; py < sprite_h; py++) {
                int src_y = frame_y + py * CREATURE_FRAME_H / sprite_h;
                for (int px = 0; px < sprite_w; px++) {
                    int src_x = frame_x + px * CREATURE_FRAME_W / sprite_w;
                    uint32_t pixel = sample_sheet(atlas, sheet, src_x, src_y);
                    if (sample_sheet_index(atlas, sheet, src_x, src_y) == 0)
                        continue;
                    put_pixel_vp(framebuffer, fb_width, fb_height,
                                 vp_x + sx + px, vp_y + sy + py, pixel);
                }
            }
        }
        /* No fallback: the atlas is an all-or-nothing hash-verified contract,
           so a creature is drawn from its original sheet or not at all.  The
           previous procedural body/head/eye rectangles invented sprite data
           the game data already provides. */
    }
}
