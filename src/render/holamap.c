#include "holamap.h"
#include "captive_data.h"
#include <string.h>

/* The holamap is a 256×128 planet surface map. Bases are placed using the
 * mission PRNG and revealed when the player uses a PLANET PROBE item.
 * Surface terrain is procedurally generated from the mission seed. */

static void generate_surface(Holamap *hm) {
    uint32_t seed = hm->seed;
    for (int y = 0; y < HOLAMAP_HEIGHT; y++) {
        for (int x = 0; x < HOLAMAP_WIDTH; x++) {
            uint16_t h = (uint16_t)captive_prng(&seed);
            uint16_t v = (h >> 4) & 0x0F;
            if (v < 3) hm->surface[y][x] = 0;       /* water */
            else if (v < 6) hm->surface[y][x] = 1;   /* lowland */
            else if (v < 10) hm->surface[y][x] = 2;  /* midland */
            else if (v < 13) hm->surface[y][x] = 3;  /* highland */
            else hm->surface[y][x] = 4;              /* mountain */
        }
    }
}

static void place_bases(Holamap *hm) {
    uint32_t seed = hm->seed + 0x4D41;
    int num = 4 + (captive_prng(&seed) % 5);
    if (num > HOLAMAP_MAX_BASES) num = HOLAMAP_MAX_BASES;
    hm->num_bases = num;

    for (int i = 0; i < num; i++) {
        hm->bases[i].x = -1;
        hm->bases[i].y = -1;
        int attempts = 100;
        while (attempts-- > 0) {
            int bx = 8 + (captive_prng(&seed) % (HOLAMAP_WIDTH - 16));
            int by = 8 + (captive_prng(&seed) % (HOLAMAP_HEIGHT - 16));
            if (hm->surface[by][bx] >= 1 && hm->surface[by][bx] <= 3) {
                hm->bases[i].x = bx;
                hm->bases[i].y = by;
                hm->bases[i].revealed = false;
                hm->bases[i].destroyed = false;
                break;
            }
        }
        /* A pathological seed can exhaust the random attempts.  Do not
         * leave the zero-initialised (0,0) coordinate as a playable base;
         * scan for the first valid land cell instead. */
        if (hm->bases[i].x < 0) {
            for (int by = 8; by < HOLAMAP_HEIGHT - 8 && hm->bases[i].x < 0; by++) {
                for (int bx = 8; bx < HOLAMAP_WIDTH - 8; bx++) {
                    if (hm->surface[by][bx] >= 1 && hm->surface[by][bx] <= 3) {
                        hm->bases[i].x = bx;
                        hm->bases[i].y = by;
                        break;
                    }
                }
            }
        }
        /* The generated surface normally always contains land.  Keep the
         * coordinate invariant even for a synthetic all-water surface. */
        if (hm->bases[i].x < 0) {
            hm->bases[i].x = HOLAMAP_WIDTH / 2;
            hm->bases[i].y = HOLAMAP_HEIGHT / 2;
        }
        hm->bases[i].revealed = false;
        hm->bases[i].destroyed = false;
    }
}

void holamap_init(Holamap *hm, uint32_t mission_seed) {
    if (!hm) return;
    memset(hm, 0, sizeof(*hm));
    hm->seed = mission_seed;
    hm->cursor_x = HOLAMAP_WIDTH / 2;
    hm->cursor_y = HOLAMAP_HEIGHT / 2;
    generate_surface(hm);
    place_bases(hm);
}

void holamap_reveal_base(Holamap *hm, int base_idx) {
    if (hm && base_idx >= 0 && base_idx < hm->num_bases &&
        base_idx < HOLAMAP_MAX_BASES)
        hm->bases[base_idx].revealed = true;
}

static const uint32_t terrain_colors[5] = {
    0xFF1A3366,  /* water: dark blue */
    0xFF2D6B30,  /* lowland: dark green */
    0xFF4A8C3F,  /* midland: green */
    0xFF8B7355,  /* highland: brown */
    0xFFAAAAAA,  /* mountain: grey */
};

void holamap_render(const Holamap *hm, uint32_t *framebuffer,
                    int fb_width, int fb_height) {
    if (!hm || !framebuffer || fb_width <= 0 || fb_height <= 0) return;
    int ox = (fb_width - HOLAMAP_WIDTH) / 2;
    int oy = (fb_height - HOLAMAP_HEIGHT) / 2;
    if (ox < 0) ox = 0;
    if (oy < 0) oy = 0;

    for (int y = 0; y < HOLAMAP_HEIGHT && (oy + y) < fb_height; y++) {
        for (int x = 0; x < HOLAMAP_WIDTH && (ox + x) < fb_width; x++) {
            uint8_t t = hm->surface[y][x];
            if (t > 4) t = 4;
            framebuffer[(oy + y) * fb_width + (ox + x)] = terrain_colors[t];
        }
    }

    int base_count = hm->num_bases;
    if (base_count < 0) base_count = 0;
    if (base_count > HOLAMAP_MAX_BASES) base_count = HOLAMAP_MAX_BASES;
    for (int i = 0; i < base_count; i++) {
        if (!hm->bases[i].revealed) continue;
        int bx = ox + hm->bases[i].x;
        int by = oy + hm->bases[i].y;
        uint32_t color = hm->bases[i].destroyed ? 0xFF666666 : 0xFFFF0000;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int px = bx + dx, py = by + dy;
                if (px >= 0 && px < fb_width && py >= 0 && py < fb_height)
                    framebuffer[py * fb_width + px] = color;
            }
        }
    }

    /* Cursor crosshair */
    int cx = ox + hm->cursor_x, cy = oy + hm->cursor_y;
    for (int d = -3; d <= 3; d++) {
        if (d == 0) continue;
        int px = cx + d, py = cy;
        if (px >= 0 && px < fb_width && py >= 0 && py < fb_height)
            framebuffer[py * fb_width + px] = 0xFFFFFFFF;
        px = cx; py = cy + d;
        if (px >= 0 && px < fb_width && py >= 0 && py < fb_height)
            framebuffer[py * fb_width + px] = 0xFFFFFFFF;
    }
}
