#include "amiga_hunk.h"
#include <string.h>

#define HUNK_CODE    0x3E9U
#define HUNK_DATA    0x3EAU
#define HUNK_BSS     0x3EBU
#define HUNK_RELOC32 0x3ECU
#define HUNK_SYMBOL  0x3F0U
#define HUNK_END     0x3F2U
#define HUNK_HEADER  0x3F3U

static bool read_be32(const uint8_t *data, size_t size, size_t *offset, uint32_t *value) {
    if (*offset > size || size - *offset < 4) return false;
    *value = ((uint32_t)data[*offset] << 24) |
        ((uint32_t)data[*offset + 1] << 16) |
        ((uint32_t)data[*offset + 2] << 8) | data[*offset + 3];
    *offset += 4;
    return true;
}

static bool skip_longs(size_t size, size_t *offset, uint32_t longs) {
    if (longs > (size - *offset) / 4) return false;
    *offset += (size_t)longs * 4;
    return true;
}

bool amiga_hunk_parse(const uint8_t *data, size_t size, AmigaHunkInfo *info) {
    if (!data || !info) return false;
    memset(info, 0, sizeof(*info));
    size_t offset = 0;
    uint32_t value;
    if (!read_be32(data, size, &offset, &value) || value != HUNK_HEADER) return false;

    /* Resident-library names are length-prefixed longword strings, ending in 0. */
    do {
        if (!read_be32(data, size, &offset, &value) || !skip_longs(size, &offset, value))
            return false;
    } while (value != 0);

    uint32_t table_size, first, last;
    if (!read_be32(data, size, &offset, &table_size) ||
        !read_be32(data, size, &offset, &first) || !read_be32(data, size, &offset, &last) ||
        first > last || table_size != last - first + 1 ||
        !skip_longs(size, &offset, table_size)) return false;
    info->hunk_count = table_size;

    size_t end_count = 0;
    while (offset < size) {
        if (!read_be32(data, size, &offset, &value)) return false;
        if (value == HUNK_END) {
            end_count++;
            continue;
        }
        if (value == HUNK_CODE || value == HUNK_DATA) {
            uint32_t longs;
            if (!read_be32(data, size, &offset, &longs)) return false;
            size_t payload_offset = offset;
            if (!skip_longs(size, &offset, longs)) return false;
            if (value == HUNK_CODE) {
                if (info->code_count++ == 0) {
                    info->first_code_offset = payload_offset;
                    info->first_code_size = (size_t)longs * 4;
                }
            } else {
                info->data_count++;
            }
        } else if (value == HUNK_BSS) {
            if (!read_be32(data, size, &offset, &value)) return false;
            info->bss_count++;
        } else if (value == HUNK_RELOC32) {
            uint32_t count;
            do {
                if (!read_be32(data, size, &offset, &count)) return false;
                if (count == 0) break;
                if (!read_be32(data, size, &offset, &value) || !skip_longs(size, &offset, count))
                    return false;
                info->reloc32_count += count;
            } while (count != 0);
        } else if (value == HUNK_SYMBOL) {
            do {
                if (!read_be32(data, size, &offset, &value) || !skip_longs(size, &offset, value))
                    return false;
                if (value != 0 && !read_be32(data, size, &offset, &value)) return false;
            } while (value != 0);
        } else {
            return false;
        }
    }
    return end_count == info->hunk_count &&
        info->code_count + info->data_count + info->bss_count > 0;
}
