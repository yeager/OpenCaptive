#ifndef CAPTIVE_DOS_RUNTIME_H
#define CAPTIVE_DOS_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Render the caller-owned DOSBox-X CAPPO memory image. A complete dump is
 * presented from its original A000:0000 VGA surface; malformed/incomplete
 * input fails closed instead of reconstructing a synthetic scene. */
bool captive_dos_runtime_render(const uint8_t *memory, size_t memory_size,
                                uint16_t ds_segment,
                                uint16_t source_bank_segment,
                                uint32_t *framebuffer, int fb_width,
                                int fb_height);

#endif
