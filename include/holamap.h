#ifndef HOLAMAP_H
#define HOLAMAP_H

#include "game_state.h"
#include <stdint.h>

#define HOLAMAP_WIDTH  256
#define HOLAMAP_HEIGHT 128
#define HOLAMAP_MAX_BASES 16

typedef struct {
    int x, y;
    bool revealed;
    bool destroyed;
} HolamapBase;

typedef struct {
    /* Surface/base fields remain zero until the original planet resources
     * are decoded. Zero means "not available", never a generated terrain. */
    uint8_t surface[HOLAMAP_HEIGHT][HOLAMAP_WIDTH];
    HolamapBase bases[HOLAMAP_MAX_BASES];
    int num_bases;
    int cursor_x, cursor_y;
    uint32_t seed;
    /* Verified DOSBox-X frame captured from the original CAPPO holomap. */
    const uint8_t *reference_rgba;
    int reference_width, reference_height;
} Holamap;

void holamap_init(Holamap *hm, uint32_t mission_seed);
void holamap_set_reference_frame(Holamap *hm, const uint8_t *rgba,
                                 int width, int height);
/* Apply one original navigation-arrow action to the real holomap cursor.
 * This changes input state only; it never creates surface or base data. */
void holamap_move_cursor(Holamap *hm, int dx, int dy);
void holamap_reveal_base(Holamap *hm, int base_idx);
void holamap_render(const Holamap *hm, uint32_t *framebuffer,
                    int fb_width, int fb_height);

#endif
