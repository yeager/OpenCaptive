#include "holamap.h"

#include <string.h>

void holamap_init(Holamap *hm, uint32_t mission_seed) {
    if (!hm) return;
    memset(hm, 0, sizeof(*hm));
    hm->seed = mission_seed;
    hm->cursor_x = HOLAMAP_WIDTH / 2;
    hm->cursor_y = HOLAMAP_HEIGHT / 2;
}

void holamap_set_reference_frame(Holamap *hm, const uint8_t *rgba,
                                 int width, int height) {
    if (!hm) return;
    hm->reference_rgba = rgba;
    hm->reference_width = width;
    hm->reference_height = height;
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

    /* This is the real 640x400 DOSBox-X CAPPO frame reduced to native size.
     * Preserve its pixels; do not redraw missing objects with generated
     * terrain, stars, labels, markers, or fonts. */
    for (int y = 0; y < fb_height; ++y) {
        int sy = (int)((long long)y * hm->reference_height / fb_height);
        if (sy >= hm->reference_height) sy = hm->reference_height - 1;
        for (int x = 0; x < fb_width; ++x) {
            int sx = (int)((long long)x * hm->reference_width / fb_width);
            if (sx >= hm->reference_width) sx = hm->reference_width - 1;
            const uint8_t *p = hm->reference_rgba +
                ((size_t)sy * (size_t)hm->reference_width + (size_t)sx) * 4U;
            framebuffer[(size_t)y * (size_t)fb_width + (size_t)x] =
                ((uint32_t)p[3] << 24) | ((uint32_t)p[0] << 16) |
                ((uint32_t)p[1] << 8) | (uint32_t)p[2];
        }
    }
}
