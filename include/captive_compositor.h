#ifndef CAPTIVE_COMPOSITOR_H
#define CAPTIVE_COMPOSITOR_H

#include <stdbool.h>
#include <stdint.h>

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

#endif
