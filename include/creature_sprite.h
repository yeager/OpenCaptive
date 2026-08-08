#ifndef CREATURE_SPRITE_H
#define CREATURE_SPRITE_H

#include "pl5_decoder.h"
#include <stdint.h>
#include <stdbool.h>

/* Captive creature sprites are arranged in ALIEN1-6.PL5 sheets (320x200).
 * Each sheet contains animation frames in a grid. The standard frame size
 * varies by creature type but 32x40 is common, giving up to 50 frames. */

#define CREATURE_FRAME_W 32
#define CREATURE_FRAME_H 40
#define CREATURE_COLS    (PL5_WIDTH / CREATURE_FRAME_W)   /* 10 */
#define CREATURE_ROWS    (PL5_HEIGHT / CREATURE_FRAME_H)  /* 5 */
#define CREATURE_MAX_FRAMES (CREATURE_COLS * CREATURE_ROWS) /* 50 */

typedef struct {
    uint8_t pixels[CREATURE_FRAME_H][CREATURE_FRAME_W];
    bool valid;
} CreatureFrame;

typedef struct {
    CreatureFrame frames[CREATURE_MAX_FRAMES];
    int num_frames;
    uint32_t palette[PL5_COLORS];
} CreatureSpriteSet;

/* Convert the frame number stored by CAPPO at DS:0xA16E to its position in
 * the 10x5 ALIEN*.PL5 frame grid. Returns false for an invalid frame. */
bool creature_sprite_frame_origin(int frame_index, int *x, int *y);

bool creature_sprite_load(const PL5Image *sheet, CreatureSpriteSet *out);
void creature_sprite_blit(const CreatureFrame *frame, const uint32_t *palette,
                          uint32_t *framebuffer, int fb_width, int fb_height,
                          int dst_x, int dst_y, int scale);

#endif
