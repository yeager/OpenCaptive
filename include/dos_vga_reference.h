#ifndef DOS_VGA_REFERENCE_H
#define DOS_VGA_REFERENCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* A DOSBox MEMDUMPBIN 0 0 100000 capture is a complete conventional-memory
 * image.  Mode 13h's indexed VGA frame starts at physical address A0000.
 * This module deliberately accepts the dump as caller-owned reference input;
 * it contains no game media and does not identify an asset by its filename. */
#define DOS_VGA_MEMORY_SIZE 0x100000u
#define DOS_VGA_FRAME_OFFSET 0xA0000u
#define DOS_VGA_FRAME_WIDTH 320
#define DOS_VGA_FRAME_HEIGHT 200
#define DOS_VGA_FRAME_SIZE (DOS_VGA_FRAME_WIDTH * DOS_VGA_FRAME_HEIGHT)

bool dos_vga_reference_decode(const uint8_t *memory, size_t memory_size,
                              uint32_t *out_pixels, size_t out_count);

#endif
