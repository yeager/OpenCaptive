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
    int zoom_level;
    uint32_t seed;
    /* Verified DOSBox-X frame captured from the original CAPPO holomap. */
    const uint8_t *reference_rgba;
    int reference_width, reference_height;
    const uint8_t *zoom_out_reference;
    int zoom_out_reference_width, zoom_out_reference_height;
    const uint8_t *zoom_in_reference;
    int zoom_in_reference_width, zoom_in_reference_height;
} Holamap;

void holamap_init(Holamap *hm, uint32_t mission_seed);
void holamap_set_reference_frame(Holamap *hm, const uint8_t *rgba,
                                 int width, int height);
void holamap_set_zoom_reference_frames(Holamap *hm,
                                       const uint8_t *zoom_out_rgba,
                                       int zoom_out_width, int zoom_out_height,
                                       const uint8_t *zoom_in_rgba,
                                       int zoom_in_width, int zoom_in_height);
/* Copy an authenticated original frame without drawing anything on top of it. */
void holamap_render_reference_frame(const uint8_t *rgba, int reference_width,
                                    int reference_height, uint32_t *framebuffer,
                                    int fb_width, int fb_height);
/* Apply one original navigation-arrow action to the real holomap cursor.
 * This changes input state only; it never creates surface or base data. */
void holamap_move_cursor(Holamap *hm, int dx, int dy);
/* Map an absolute point in CAPPO's verified 320x200 holomap frame to the
 * original cursor coordinates. Returns false outside the map panel. */
bool holamap_set_cursor_from_frame(Holamap *hm, int frame_x, int frame_y);
void holamap_center_cursor(Holamap *hm);
void holamap_zoom_in(Holamap *hm);
void holamap_zoom_out(Holamap *hm);
void holamap_reveal_base(Holamap *hm, int base_idx);
void holamap_render(const Holamap *hm, uint32_t *framebuffer,
                    int fb_width, int fb_height);

#endif
