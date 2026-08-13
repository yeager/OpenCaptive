#include "captive_dos_runtime.h"

#include "dos_vga_reference.h"
#include <string.h>

bool captive_dos_runtime_render(const uint8_t *memory, size_t memory_size,
                                uint16_t ds_segment,
                                uint16_t source_bank_segment,
                                uint32_t *framebuffer, int fb_width,
                                int fb_height) {
    if (!memory || memory_size < DOS_VGA_MEMORY_SIZE || !framebuffer ||
        fb_width <= 0 || fb_height <= 0 ||
        DOS_VGA_FRAME_WIDTH > fb_width || DOS_VGA_FRAME_HEIGHT > fb_height) {
        return false;
    }

    /* DOSBox-X's MEMDUMPBIN contains the original CAPPO VGA surface at
     * A000:0000.  It is the strongest possible parity source: CAPPO has
     * already executed its real map lookup, descriptor dispatch, masking,
     * animation and palette work.  A valid dump is therefore rendered
     * exactly as captured, including an intentionally black frame.  Do not
     * fall back to a reconstructed descriptor scene: that would replace a
     * real original result with an approximation and can hide a bad runtime
     * handoff behind synthetic-looking pixels. */
    uint32_t vga[DOS_VGA_FRAME_SIZE];
    if (dos_vga_reference_decode(memory, memory_size, vga,
                                 DOS_VGA_FRAME_SIZE)) {
        for (int y = 0; y < DOS_VGA_FRAME_HEIGHT; ++y) {
            memcpy(framebuffer + (size_t)y * (size_t)fb_width,
                   vga + (size_t)y * DOS_VGA_FRAME_WIDTH,
                   DOS_VGA_FRAME_WIDTH * sizeof(uint32_t));
        }
        return true;
    }

    /* A complete DOSBox-X dump always has the 320x200 VGA window.  Reaching
     * this point means the supplied dump is malformed, so fail closed. */
    (void)ds_segment;
    (void)source_bank_segment;
    return false;
}
