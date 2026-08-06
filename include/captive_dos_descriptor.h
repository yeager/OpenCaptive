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

/* Decode one original descriptor from a physical one-megabyte DOS memory
 * image. `descriptor_segment` and `source_bank_table_segment` are the
 * relocated values observed in that process; neither is hard-coded as a
 * media identity. The table segment is not necessarily the CPU's CS. */
bool captive_dos_descriptor_read(const uint8_t *memory, size_t memory_size,
                                 uint16_t descriptor_segment,
                                 uint16_t source_bank_table_segment,
                                 uint16_t graphic_id,
                                 CaptiveDosDescriptor *out);

#endif
