#ifndef CAPTIVE_COMPOSITOR_H
#define CAPTIVE_COMPOSITOR_H

#include <stdbool.h>
#include <stdint.h>
#include "captive_dos_descriptor.h"

#define CAPTIVE_COMPOSITOR_PALETTE_SIZE 32

/*
 * The original Captive view is not a texture-mapped scene.  It is a sequence
 * of masked copies of already projected panels.  Keep that distinction in
 * the API: callers supply source rectangles and their one-bit masks, and the
 * compositor performs only the original-style copy.  It never scales, shades
 * or synthesises pixels.
 */
typedef struct {
    const uint32_t *pixels;
    const uint8_t *mask;      /* One byte per source pixel; zero is transparent. */
    int source_width;
    int source_height;
    int source_x;
    int source_y;
    int width;
    int height;
    int destination_x;
    int destination_y;
} CaptivePanelBlit;

/* Copy one original panel into a 144 x 112 view buffer.  Coordinates are
 * local to that buffer and clipped, so recovered commands may safely retain
 * their original off-screen extents. */
bool captive_compositor_blit(uint32_t *view, int stride,
                             const CaptivePanelBlit *panel);

/* Execute an already sorted (back-to-front) list of original panel commands.
 * Later opaque pixels intentionally replace earlier pixels. */
bool captive_compositor_blit_all(uint32_t *view, int stride,
                                 const CaptivePanelBlit *panels,
                                 int panel_count);

/* Execute a decoded DOS descriptor panel. Flags are the original CAPPO
 * flags: 0x01 mirrors source pixels horizontally and 0x04 treats palette
 * index zero as transparent. destination_offset uses the 160-byte DOS view
 * work-buffer layout. */
bool captive_compositor_blit_indices(
    uint32_t *view, int stride, const uint8_t *indices,
    int source_width, int source_height,
    const uint32_t palette[CAPTIVE_COMPOSITOR_PALETTE_SIZE],
    uint16_t destination_offset, uint8_t flags);

#endif
