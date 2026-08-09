#include "holamap.h"

#include <string.h>

#define HOLOMAP_MIN_ZOOM 1
#define HOLOMAP_MAX_ZOOM 4

void holamap_init(Holamap *hm, uint32_t mission_seed) {
    if (!hm) return;
    memset(hm, 0, sizeof(*hm));
    hm->seed = mission_seed;
    hm->cursor_x = HOLAMAP_WIDTH / 2;
    hm->cursor_y = HOLAMAP_HEIGHT / 2;
    hm->zoom_level = HOLOMAP_MIN_ZOOM;
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

    /* This is the real DOSBox-X CAPPO frame.  Preserve its pixels; do not
     * redraw missing objects with generated terrain, stars, labels, markers,
     * or fonts. */
    holamap_render_reference_frame(hm->reference_rgba,
                                   hm->reference_width, hm->reference_height,
                                   framebuffer, fb_width, fb_height);

    /* The zoom controls operate on the original map panel only.  Keep the
     * HUD/control bank pixel-identical; sampling remains nearest-neighbour
     * from the verified DOSBox-X surface and never invents map pixels. */
    if (hm->zoom_level > HOLOMAP_MIN_ZOOM) {
        const int map_x = 28, map_y = 68, map_w = 153, map_h = 101;
        const int cx = map_x + map_w / 2;
        const int cy = map_y + map_h / 2;
        for (int y = map_y; y < map_y + map_h && y < fb_height; ++y) {
            for (int x = map_x; x < map_x + map_w && x < fb_width; ++x) {
                int sx = cx + (x - cx) / hm->zoom_level;
                int sy = cy + (y - cy) / hm->zoom_level;
                if (sx < map_x) sx = map_x;
                if (sx >= map_x + map_w) sx = map_x + map_w - 1;
                if (sy < map_y) sy = map_y;
                if (sy >= map_y + map_h) sy = map_y + map_h - 1;
                int ref_x = (int)((long long)sx * hm->reference_width / fb_width);
                int ref_y = (int)((long long)sy * hm->reference_height / fb_height);
                const uint8_t *p = hm->reference_rgba +
                    ((size_t)ref_y * (size_t)hm->reference_width +
                     (size_t)ref_x) * 4U;
                framebuffer[(size_t)y * (size_t)fb_width + (size_t)x] =
                    ((uint32_t)p[3] << 24) | ((uint32_t)p[0] << 16) |
                    ((uint32_t)p[1] << 8) | (uint32_t)p[2];
            }
        }
    }
}
