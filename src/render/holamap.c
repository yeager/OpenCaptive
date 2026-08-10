#include "holamap.h"

#include <string.h>

#define HOLOMAP_MIN_ZOOM 0
#define HOLOMAP_MAX_ZOOM 2

void holamap_init(Holamap *hm, uint32_t mission_seed) {
    if (!hm) return;
    memset(hm, 0, sizeof(*hm));
    hm->seed = mission_seed;
    hm->cursor_x = HOLAMAP_WIDTH / 2;
    hm->cursor_y = HOLAMAP_HEIGHT / 2;
    /* 1 is CAPPO's opening scale; 0 and 2 are the authenticated ladder
     * zoom-out/zoom-in checkpoints. */
    hm->zoom_level = 1;
}

void holamap_zoom_in(Holamap *hm) {
    if (hm && hm->zoom_level < HOLOMAP_MAX_ZOOM) hm->zoom_level++;
}

void holamap_zoom_out(Holamap *hm) {
    if (hm && hm->zoom_level > HOLOMAP_MIN_ZOOM) hm->zoom_level--;
}

void holamap_set_reference_frame(Holamap *hm, const uint8_t *rgba,
                                 int width, int height) {
    if (!hm) return;
    hm->reference_rgba = rgba;
    hm->reference_width = width;
    hm->reference_height = height;
}

void holamap_set_zoom_reference_frames(Holamap *hm,
                                       const uint8_t *zoom_out_rgba,
                                       int zoom_out_width, int zoom_out_height,
                                       const uint8_t *zoom_in_rgba,
                                       int zoom_in_width, int zoom_in_height) {
    if (!hm) return;
    hm->zoom_out_reference = zoom_out_rgba;
    hm->zoom_out_reference_width = zoom_out_width;
    hm->zoom_out_reference_height = zoom_out_height;
    hm->zoom_in_reference = zoom_in_rgba;
    hm->zoom_in_reference_width = zoom_in_width;
    hm->zoom_in_reference_height = zoom_in_height;
}

void holamap_render_reference_frame(const uint8_t *rgba, int reference_width,
                                    int reference_height, uint32_t *framebuffer,
                                    int fb_width, int fb_height) {
    if (!rgba || !framebuffer || reference_width <= 0 || reference_height <= 0 ||
        fb_width <= 0 || fb_height <= 0)
        return;
    for (int y = 0; y < fb_height; ++y) {
        int sy = (int)((long long)y * reference_height / fb_height);
        if (sy >= reference_height) sy = reference_height - 1;
        for (int x = 0; x < fb_width; ++x) {
            int sx = (int)((long long)x * reference_width / fb_width);
            if (sx >= reference_width) sx = reference_width - 1;
            const uint8_t *p = rgba +
                ((size_t)sy * (size_t)reference_width + (size_t)sx) * 4U;
            framebuffer[(size_t)y * (size_t)fb_width + (size_t)x] =
                ((uint32_t)p[3] << 24) | ((uint32_t)p[0] << 16) |
                ((uint32_t)p[1] << 8) | (uint32_t)p[2];
        }
    }
}

void holamap_move_cursor(Holamap *hm, int dx, int dy) {
    if (!hm) return;
    int x = hm->cursor_x + dx;
    int y = hm->cursor_y + dy;
    if (x < 0) x = 0;
    if (x >= HOLAMAP_WIDTH) x = HOLAMAP_WIDTH - 1;
    if (y < 0) y = 0;
    if (y >= HOLAMAP_HEIGHT) y = HOLAMAP_HEIGHT - 1;
    hm->cursor_x = x;
    hm->cursor_y = y;
}

bool holamap_set_cursor_from_frame(Holamap *hm, int frame_x, int frame_y) {
    /* These bounds are the map rectangle in the authenticated
     * assets/captive/holamap-initial.png / holamap-target.png captures.
     * CAPPO's 256x128 cursor space is scaled into that original panel; no
     * surface, base, or target data is inferred by this conversion. */
    const int map_left = 28;
    const int map_top = 65;
    const int map_right = 180;
    const int map_bottom = 165;
    if (!hm || frame_x < map_left || frame_x >= map_right ||
        frame_y < map_top || frame_y >= map_bottom)
        return false;
    int x = (frame_x - map_left) * HOLAMAP_WIDTH /
            (map_right - map_left);
    int y = (frame_y - map_top) * HOLAMAP_HEIGHT /
            (map_bottom - map_top);
    if (x >= HOLAMAP_WIDTH) x = HOLAMAP_WIDTH - 1;
    if (y >= HOLAMAP_HEIGHT) y = HOLAMAP_HEIGHT - 1;
    hm->cursor_x = x;
    hm->cursor_y = y;
    return true;
}

void holamap_center_cursor(Holamap *hm) {
    if (!hm) return;
    /* CAPPO's Pyramid command returns the cursor to The Swan.  This only
     * changes the selection state; no marker or map pixels are fabricated. */
    hm->cursor_x = HOLAMAP_WIDTH / 2;
    hm->cursor_y = HOLAMAP_HEIGHT / 2;
}

void holamap_reveal_base(Holamap *hm, int base_idx) {
    if (hm && base_idx >= 0 && base_idx < hm->num_bases &&
        base_idx < HOLAMAP_MAX_BASES)
        hm->bases[base_idx].revealed = true;
}

void holamap_render(const Holamap *hm, uint32_t *framebuffer,
                    int fb_width, int fb_height) {
    if (!hm || !framebuffer || !hm->reference_rgba ||
        hm->reference_width <= 0 || hm->reference_height <= 0 ||
        fb_width <= 0 || fb_height <= 0)
        return;

    /* CAPPO switches the complete map composition for the ladder commands.
     * Select only frames captured from the original runtime; never invent
     * map geometry by scaling the opening screenshot. */
    const uint8_t *rgba = hm->reference_rgba;
    int width = hm->reference_width;
    int height = hm->reference_height;
    if (hm->zoom_level == 0 && hm->zoom_out_reference) {
        rgba = hm->zoom_out_reference;
        width = hm->zoom_out_reference_width;
        height = hm->zoom_out_reference_height;
    } else if (hm->zoom_level > 1 && hm->zoom_in_reference) {
        rgba = hm->zoom_in_reference;
        width = hm->zoom_in_reference_width;
        height = hm->zoom_in_reference_height;
    }
    holamap_render_reference_frame(rgba, width, height,
                                   framebuffer, fb_width, fb_height);
}
