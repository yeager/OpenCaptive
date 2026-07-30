#ifndef CAPTIVE_DOS_BLITTER_H
#define CAPTIVE_DOS_BLITTER_H

#include "captive_dos_descriptor.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Captive's DOS view sheets use five packed bytes for eight indexed pixels.
 * The original blitter lays each source row out at a 200-byte stride and
 * writes into the same 320 x 200 logical canvas.  This helper performs the
 * pixel-equivalent operation on indexed pixels.  `memory` remains caller
 * owned: it is normally an original DOS memory fixture used for analysis. */
#define CAPTIVE_DOS_CANVAS_WIDTH 320u
#define CAPTIVE_DOS_CANVAS_HEIGHT 200u
#define CAPTIVE_DOS_CANVAS_PIXELS (CAPTIVE_DOS_CANVAS_WIDTH * CAPTIVE_DOS_CANVAS_HEIGHT)

bool captive_dos_blit_descriptor(const uint8_t *memory, size_t memory_size,
                                 const CaptiveDosDescriptor *descriptor,
                                 uint8_t *canvas, size_t canvas_size);

#endif
