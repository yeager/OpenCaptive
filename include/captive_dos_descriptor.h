#ifndef CAPTIVE_DOS_DESCRIPTOR_H
#define CAPTIVE_DOS_DESCRIPTOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Captive's DOS renderer indexes an eight-byte runtime descriptor record at
 * segment:0x00c0 + (graphic_id * 8). The source bank is resolved through a
 * relocated word table at source-bank-table-segment:0x423c. This API describes that
 * observed original layout without treating a memory dump as game data. */
typedef struct {
    uint16_t source_offset;
    uint16_t destination_offset;
    uint8_t width_bytes;
    uint8_t height;
    uint8_t flags;
    uint8_t source_bank;
    uint16_t source_segment;
} CaptiveDosDescriptor;

#define CAPTIVE_DOS_PACKED_SOURCE_STRIDE 200U
#define CAPTIVE_DOS_VIEW_STRIDE 160U
#define CAPTIVE_DOS_VIEW_HEIGHT 112U

/* The DOS renderer builds the active view in a 160-byte row buffer.  The
 * display copy exposes only the first 144 pixels of each row.  Descriptor
 * destination offsets are byte offsets in that work buffer (CAPPO.EXE:
 * 0x2db4-0x2dc0), not packed viewport coordinates. */
bool captive_dos_descriptor_destination_xy(uint16_t destination_offset,
                                           int *x, int *y);

/* Decode one original descriptor from a physical one-megabyte DOS memory
 * image. `descriptor_segment` and `source_bank_table_segment` are the
 * relocated values observed in that process; neither is hard-coded as a
 * media identity. The table segment is not necessarily the CPU's CS. */
bool captive_dos_descriptor_read(const uint8_t *memory, size_t memory_size,
                                 uint16_t descriptor_segment,
                                 uint16_t source_bank_table_segment,
                                 uint16_t graphic_id,
                                 CaptiveDosDescriptor *out);

/* Expand one original descriptor source rectangle to 5-bit palette indices.
 * The output remains in source order; callers apply the descriptor's mirror
 * and transparency flags when executing the recovered draw command. */
bool captive_dos_descriptor_decode_indices(const uint8_t *memory,
                                           size_t memory_size,
                                           const CaptiveDosDescriptor *descriptor,
                                           uint8_t *indices,
                                           size_t indices_stride);

#endif
