#ifndef CAPTIVE_DOS_RUNTIME_H
#define CAPTIVE_DOS_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Execute the descriptor draw list from a caller-owned DOSBox-X CAPPO
 * memory image.  This is deliberately a raw-runtime boundary: no cell code
 * is translated into an OpenCaptive map and no pixels are generated. */
bool captive_dos_runtime_render(const uint8_t *memory, size_t memory_size,
                                uint16_t ds_segment,
                                uint16_t source_bank_segment,
                                uint32_t *framebuffer, int fb_width,
                                int fb_height);

#endif
