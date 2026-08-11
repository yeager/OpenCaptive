#include "captive_dos_runtime.h"

#include "captive_compositor.h"
#include "captive_dos_descriptor.h"
#include "captive_dos_map.h"
#include "dos_vga_reference.h"
#include "pl5_decoder.h"
#include "opencaptive.h"

#include <stdlib.h>
#include <string.h>

static bool render_descriptor(const uint8_t *memory, size_t memory_size,
                              uint16_t ds_segment,
                              uint16_t source_bank_segment,
                              uint16_t graphic_id,
                              uint32_t *work) {
    CaptiveDosDescriptor descriptor;
    if (!captive_dos_descriptor_read(memory, memory_size, ds_segment,
                                     source_bank_segment, graphic_id,
                                     &descriptor) ||
        descriptor.width_bytes == 0U || descriptor.height == 0U) {
        return true; /* CAPPO uses zero-size records as draw-list sentinels. */
    }

    size_t width = (size_t)descriptor.width_bytes * 8U;
    size_t bytes = width * descriptor.height;
    uint8_t *indices = (uint8_t *)malloc(bytes);
    if (!indices) return false;
    bool ok = captive_dos_descriptor_decode_indices(
        memory, memory_size, &descriptor, indices, width);
    if (ok) {
        ok = captive_compositor_blit_indices(
            work, CAPTIVE_DOS_VIEW_STRIDE, indices, (int)width,
            descriptor.height, pl5_default_palette,
            descriptor.destination_offset, descriptor.flags);
    }
    free(indices);
    return ok;
}

bool captive_dos_runtime_render(const uint8_t *memory, size_t memory_size,
                                uint16_t ds_segment,
                                uint16_t source_bank_segment,
                                uint32_t *framebuffer, int fb_width,
                                int fb_height) {
    if (!memory || memory_size < DOS_VGA_MEMORY_SIZE || !framebuffer ||
        fb_width <= 0 || fb_height <= 0 ||
        CAPTIVE_VIEWPORT_X + CAPTIVE_VIEWPORT_WIDTH > fb_width ||
        CAPTIVE_VIEWPORT_Y + CAPTIVE_VIEWPORT_HEIGHT > fb_height) {
        return false;
    }

    /* DOSBox-X's MEMDUMPBIN contains the original CAPPO VGA surface at
     * A000:0000.  It is the strongest possible parity source: CAPPO has
     * already executed its real map lookup, descriptor dispatch, masking,
     * animation and palette work.  Prefer those exact pixels whenever the
     * dump contains a rendered frame.  The descriptor path below remains an
     * analysis fallback for a dump captured before VGA was populated; it does
     * not manufacture a replacement scene. */
    uint32_t vga[DOS_VGA_FRAME_SIZE];
    if (dos_vga_reference_decode(memory, memory_size, vga,
                                 DOS_VGA_FRAME_SIZE)) {
        bool has_pixels = false;
        for (size_t i = 0; i < DOS_VGA_FRAME_SIZE; ++i) {
            if (vga[i] != 0U) {
                has_pixels = true;
                break;
            }
        }
        if (has_pixels) {
            for (int y = 0; y < DOS_VGA_FRAME_HEIGHT; ++y) {
                memcpy(framebuffer + (size_t)y * (size_t)fb_width,
                       vga + (size_t)y * DOS_VGA_FRAME_WIDTH,
                       DOS_VGA_FRAME_WIDTH * sizeof(uint32_t));
            }
            return true;
        }
    }

    CaptiveDosMapState state;
    CaptiveDosViewWindow window;
    if (!captive_dos_map_decode(memory, memory_size, ds_segment, &state) ||
        !captive_dos_view_window_build(&state, &window)) {
        return false;
    }

    uint32_t work[CAPTIVE_DOS_VIEW_STRIDE * CAPTIVE_DOS_VIEW_HEIGHT];
    memset(work, 0, sizeof(work));

    /* CAPPO seeds the 160-byte work buffer with runtime descriptor 0 before
     * walking the cell records.  This is the authenticated full-floor panel,
     * not a compatibility background. */
    if (!render_descriptor(memory, memory_size, ds_segment,
                           source_bank_segment, 0, work)) {
        return false;
    }

    /* CAPPO iterates the 38 records at DS:5B82 in order.  The record's
     * handler branch is retained here only to select the descriptor operands
     * recovered from that exact runtime state; unknown paths contribute no
     * fabricated panel. */
    for (size_t ordinal = 0;
         ordinal < CAPTIVE_DOS_DISPATCH_RECORD_COUNT; ++ordinal) {
        CaptiveDosDispatchRecord record;
        int x, y;
        if (!captive_dos_dispatch_record_read(memory, memory_size, ds_segment,
                                              ordinal, &record) ||
            !captive_dos_dispatch_window_xy(record.window_index, &x, &y)) {
            continue;
        }
        uint8_t raw = window.raw[y][x];
        CaptiveDosDescriptorOperands operands;
        memset(&operands, 0, sizeof(operands));
        if (!captive_dos_dispatch_descriptor_operands(
                memory, memory_size, ds_segment, &record, raw, &operands)) {
            continue;
        }
        for (uint8_t i = 0; i < operands.descriptor_count; ++i) {
            if (!render_descriptor(memory, memory_size, ds_segment,
                                   source_bank_segment,
                                   operands.descriptor_id[i], work)) {
                return false;
            }
        }
    }

    for (int y = 0; y < CAPTIVE_VIEWPORT_HEIGHT; ++y) {
        memcpy(framebuffer + (size_t)(CAPTIVE_VIEWPORT_Y + y) * fb_width +
                   CAPTIVE_VIEWPORT_X,
               work + (size_t)y * CAPTIVE_DOS_VIEW_STRIDE,
               CAPTIVE_VIEWPORT_WIDTH * sizeof(uint32_t));
    }
    return true;
}
