#include "creature_sprite.h"
#include <stdint.h>
#include <string.h>

bool creature_sprite_load(const PL5Image *sheet, CreatureSpriteSet *out) {
    if (!sheet || !out || !sheet->pixel_data) return false;
    if (sheet->width < PL5_WIDTH || sheet->height < PL5_HEIGHT) return false;
    size_t required_size = (size_t)sheet->width * (size_t)sheet->height;
    if (sheet->data_size < required_size) return false;

    memset(out, 0, sizeof(*out));
    memcpy(out->palette, sheet->palette, sizeof(out->palette));

    int frame = 0;
    for (int row = 0; row < CREATURE_ROWS; row++) {
        for (int col = 0; col < CREATURE_COLS; col++) {
            CreatureFrame *f = &out->frames[frame];
            int sx = col * CREATURE_FRAME_W;
            int sy = row * CREATURE_FRAME_H;
            bool all_zero = true;

            for (int y = 0; y < CREATURE_FRAME_H; y++) {
                for (int x = 0; x < CREATURE_FRAME_W; x++) {
                    uint8_t px = sheet->pixel_data[(size_t)(sy + y) * sheet->width + (sx + x)];
                    f->pixels[y][x] = px;
                    if (px != 0) all_zero = false;
                }
            }
            f->valid = !all_zero;
            frame++;
        }
    }
    out->num_frames = frame;
    return true;
}

void creature_sprite_blit(const CreatureFrame *frame, const uint32_t *palette,
                          uint32_t *framebuffer, int fb_width, int fb_height,
                          int dst_x, int dst_y, int scale) {
    if (!frame || !frame->valid || !palette || !framebuffer) return;
    if (fb_width <= 0 || fb_height <= 0) return;
    if (scale < 1) scale = 1;

    for (int y = 0; y < CREATURE_FRAME_H; y++) {
        for (int x = 0; x < CREATURE_FRAME_W; x++) {
            uint8_t idx = frame->pixels[y][x];
            if (idx == 0) continue;
            uint32_t color = palette[idx % PL5_COLORS];
            for (int sy = 0; sy < scale; sy++) {
                for (int sx = 0; sx < scale; sx++) {
                    int64_t px = (int64_t)dst_x + (int64_t)x * scale + sx;
                    int64_t py = (int64_t)dst_y + (int64_t)y * scale + sy;
                    if (px >= 0 && px < fb_width && py >= 0 && py < fb_height) {
                        size_t offset = (size_t)py * (size_t)fb_width + (size_t)px;
                        framebuffer[offset] = color;
                    }
                }
            }
        }
    }
}
