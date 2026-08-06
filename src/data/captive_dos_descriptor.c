#include "captive_dos_descriptor.h"

enum {
    DOS_PHYSICAL_MEMORY_SIZE = 1024 * 1024,
    DESCRIPTOR_BASE = 0x00c0,
    DESCRIPTOR_SIZE = 8,
    SOURCE_BANK_TABLE = 0x423c,
};

static bool range_fits(size_t offset, size_t count, size_t size) {
    return offset <= size && count <= size - offset;
}

static uint16_t read_le16(const uint8_t *bytes) {
    return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
}

bool captive_dos_descriptor_read(const uint8_t *memory, size_t memory_size,
                                 uint16_t descriptor_segment,
                                 uint16_t source_bank_table_segment,
                                 uint16_t graphic_id,
                                 CaptiveDosDescriptor *out) {
    if (!memory || !out || memory_size < DOS_PHYSICAL_MEMORY_SIZE) return false;
    size_t record = ((size_t)descriptor_segment << 4) + DESCRIPTOR_BASE +
                    (size_t)graphic_id * DESCRIPTOR_SIZE;
    if (!range_fits(record, DESCRIPTOR_SIZE, memory_size)) return false;
    const uint8_t *raw = memory + record;
    size_t bank_word = ((size_t)source_bank_table_segment << 4) + SOURCE_BANK_TABLE +
                       (size_t)raw[7] * sizeof(uint16_t);
    if (!range_fits(bank_word, sizeof(uint16_t), memory_size)) return false;

    out->source_offset = read_le16(raw);
    out->destination_offset = read_le16(raw + 2);
    out->width_bytes = raw[4];
    out->height = raw[5];
    out->flags = raw[6];
    out->source_bank = raw[7];
    out->source_segment = read_le16(memory + bank_word);
    return true;
}
