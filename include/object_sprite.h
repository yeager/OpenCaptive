#ifndef OBJECT_SPRITE_H
#define OBJECT_SPRITE_H

#include "pl5_decoder.h"
#include <stdint.h>
#include <stdbool.h>

/* OBJECTS.PL5 contains item/object graphics in a sprite sheet.
 * Objects use 16x16 frames in a 20x12 grid = 240 possible frames. */

#define OBJECT_FRAME_W 16
#define OBJECT_FRAME_H 16
#define OBJECT_COLS    (PL5_WIDTH / OBJECT_FRAME_W)    /* 20 */
#define OBJECT_ROWS    (PL5_HEIGHT / OBJECT_FRAME_H)   /* 12 (last row 8px unused) */
#define OBJECT_MAX_FRAMES (OBJECT_COLS * OBJECT_ROWS)  /* 240 */

typedef struct {
    uint8_t pixels[OBJECT_FRAME_H][OBJECT_FRAME_W];
    bool valid;
} ObjectFrame;

typedef struct {
    ObjectFrame frames[OBJECT_MAX_FRAMES];
    int num_frames;
    uint32_t palette[PL5_COLORS];
} ObjectSpriteSet;

bool object_sprite_load(const PL5Image *sheet, ObjectSpriteSet *out);
void object_sprite_blit(const ObjectFrame *frame, const uint32_t *palette,
                        uint32_t *framebuffer, int fb_width, int fb_height,
                        int dst_x, int dst_y, int scale);

#endif
