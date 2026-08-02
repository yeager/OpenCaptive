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
    uint8_t surface[HOLAMAP_HEIGHT][HOLAMAP_WIDTH];
    HolamapBase bases[HOLAMAP_MAX_BASES];
    int num_bases;
    int cursor_x, cursor_y;
    uint32_t seed;
} Holamap;

void holamap_init(Holamap *hm, uint32_t mission_seed);
void holamap_reveal_base(Holamap *hm, int base_idx);
void holamap_render(const Holamap *hm, uint32_t *framebuffer,
                    int fb_width, int fb_height);

#endif
