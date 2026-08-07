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

bool captive_dos_descriptor_destination_xy(uint16_t destination_offset,
                                           int *x, int *y) {
    if (!x || !y || destination_offset >=
        CAPTIVE_DOS_VIEW_STRIDE * CAPTIVE_DOS_VIEW_HEIGHT)
        return false;
    *x = (int)(destination_offset % CAPTIVE_DOS_VIEW_STRIDE);
    *y = (int)(destination_offset / CAPTIVE_DOS_VIEW_STRIDE);
    return true;
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

bool captive_dos_descriptor_decode_indices(const uint8_t *memory,
                                           size_t memory_size,
                                           const CaptiveDosDescriptor *descriptor,
                                           uint8_t *indices,
                                           size_t indices_stride) {
    if (!memory || !descriptor || !indices || descriptor->width_bytes == 0U ||
        descriptor->height == 0U) return false;
    size_t width = (size_t)descriptor->width_bytes * 8U;
    if (indices_stride < width) return false;
    size_t source_base = (size_t)descriptor->source_segment << 4;
    if (source_base > memory_size || descriptor->source_offset > memory_size - source_base)
        return false;
    size_t source_first = source_base + descriptor->source_offset;
    size_t last_row = (size_t)descriptor->height - 1U;
    if (last_row > (memory_size - source_first) / CAPTIVE_DOS_PACKED_SOURCE_STRIDE)
        return false;
    size_t source_last = source_first + last_row * CAPTIVE_DOS_PACKED_SOURCE_STRIDE;
    size_t packed_width = (size_t)descriptor->width_bytes * 5U;
    if (!range_fits(source_last, packed_width, memory_size)) return false;

    for (size_t y = 0; y < descriptor->height; ++y) {
        const uint8_t *row = memory + source_first + y * CAPTIVE_DOS_PACKED_SOURCE_STRIDE;
        for (size_t group = 0; group < descriptor->width_bytes; ++group) {
            const uint8_t *source = row + group * 5U;
            uint8_t *out = indices + y * indices_stride + group * 8U;
            out[0] = source[0] & 0x1fU;
            out[1] = source[1] & 0x1fU;
            out[2] = ((source[0] >> 5) & 0x07U) | ((source[1] >> 3) & 0x18U);
            out[3] = ((source[2] << 1) | ((source[1] >> 5) & 0x01U)) & 0x1fU;
            out[4] = source[3] & 0x1fU;
            out[5] = ((source[2] >> 6) & 0x03U) | ((source[3] >> 3) & 0x1cU);
            out[6] = source[4] & 0x1fU;
            out[7] = ((source[2] >> 4) & 0x03U) | ((source[4] >> 3) & 0x1cU);
        }
    }
    return true;
}
