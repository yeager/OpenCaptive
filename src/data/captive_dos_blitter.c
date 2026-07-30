#include "captive_dos_blitter.h"

enum {
    DOS_PHYSICAL_MEMORY_SIZE = 1024 * 1024,
    PACKED_ROW_STRIDE = 200,
    PIXELS_PER_GROUP = 8,
    BYTES_PER_GROUP = 5,
    FLAG_HORIZONTAL_FLIP = 0x01,
    FLAG_TRANSPARENT = 0x04,
};

static bool range_fits(size_t offset, size_t count, size_t size) {
    return offset <= size && count <= size - offset;
}

static void unpack_group(const uint8_t *source, uint8_t pixels[PIXELS_PER_GROUP]) {
    uint8_t s0 = source[0], s1 = source[1], s2 = source[2];
    uint8_t s3 = source[3], s4 = source[4];
    pixels[0] = s0 & 0x1f;
    pixels[1] = s1 & 0x1f;
    pixels[2] = ((s0 >> 5) & 0x07) | ((s1 >> 3) & 0x18);
    pixels[3] = ((s2 << 1) | ((s1 >> 5) & 0x01)) & 0x1f;
    pixels[4] = s3 & 0x1f;
    pixels[5] = ((s2 >> 6) & 0x03) | ((s3 >> 3) & 0x1c);
    pixels[6] = s4 & 0x1f;
    pixels[7] = ((s2 >> 4) & 0x03) | ((s4 >> 3) & 0x1c);
}

bool captive_dos_blit_descriptor(const uint8_t *memory, size_t memory_size,
                                 const CaptiveDosDescriptor *descriptor,
                                 uint8_t *canvas, size_t canvas_size) {
    if (!memory || !descriptor || !canvas || memory_size < DOS_PHYSICAL_MEMORY_SIZE ||
        canvas_size < CAPTIVE_DOS_CANVAS_PIXELS ||
        descriptor->destination_offset % BYTES_PER_GROUP != 0) {
        return false;
    }

    size_t source_base = (size_t)descriptor->source_segment << 4;
    size_t source_last = source_base + descriptor->source_offset +
        (size_t)(descriptor->height - 1) * PACKED_ROW_STRIDE +
        (size_t)descriptor->width_bytes * BYTES_PER_GROUP;
    if (descriptor->height == 0 || descriptor->width_bytes == 0 ||
        !range_fits(source_base, source_last - source_base, memory_size)) return false;

    size_t destination_y = descriptor->destination_offset / PACKED_ROW_STRIDE;
    size_t destination_x = (descriptor->destination_offset % PACKED_ROW_STRIDE) /
                           BYTES_PER_GROUP * PIXELS_PER_GROUP;
    if (destination_y + descriptor->height > CAPTIVE_DOS_CANVAS_HEIGHT ||
        destination_x + (size_t)descriptor->width_bytes * PIXELS_PER_GROUP >
            CAPTIVE_DOS_CANVAS_WIDTH) return false;

    for (size_t y = 0; y < descriptor->height; ++y) {
        const uint8_t *source = memory + source_base + descriptor->source_offset +
            y * PACKED_ROW_STRIDE;
        for (size_t group = 0; group < descriptor->width_bytes; ++group) {
            uint8_t pixels[PIXELS_PER_GROUP];
            unpack_group(source + group * BYTES_PER_GROUP, pixels);
            for (size_t x = 0; x < PIXELS_PER_GROUP; ++x) {
                size_t local_x = group * PIXELS_PER_GROUP + x;
                if (descriptor->flags & FLAG_HORIZONTAL_FLIP)
                    local_x = (size_t)descriptor->width_bytes * PIXELS_PER_GROUP - 1 - local_x;
                uint8_t pixel = pixels[x];
                if ((descriptor->flags & FLAG_TRANSPARENT) && pixel == 0) continue;
                canvas[(destination_y + y) * CAPTIVE_DOS_CANVAS_WIDTH + destination_x + local_x] = pixel;
            }
        }
    }
    return true;
}
