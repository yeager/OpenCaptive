#include "dos_vga_reference.h"

#include "pl5_decoder.h"

bool dos_vga_reference_decode(const uint8_t *memory, size_t memory_size,
                              uint32_t *out_pixels, size_t out_count) {
    if (!memory || !out_pixels || memory_size != DOS_VGA_MEMORY_SIZE ||
        out_count < DOS_VGA_FRAME_SIZE) {
        return false;
    }

    const uint8_t *indexed = memory + DOS_VGA_FRAME_OFFSET;
    for (size_t i = 0; i < DOS_VGA_FRAME_SIZE; ++i)
        out_pixels[i] = pl5_default_palette[indexed[i] & 0x1fu];
    return true;
}
