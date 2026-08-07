#include "amiga_hunk.h"
#include <string.h>

#define HUNK_CODE    0x3E9U
#define HUNK_DATA    0x3EAU
#define HUNK_BSS     0x3EBU
#define HUNK_RELOC32 0x3ECU
#define HUNK_SYMBOL  0x3F0U
#define HUNK_END     0x3F2U
#define HUNK_HEADER  0x3F3U
/* HUNK memory-attribute bits occupy the high two bits of a hunk tag. */
#define HUNK_TYPE_MASK 0x3FFFFFFFU

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
        first > last || (uint64_t)last - first + 1U != table_size || table_size == 0U ||
        !skip_longs(size, &offset, table_size)) return false;
    info->hunk_count = table_size;

    size_t end_count = 0;
    size_t payload_hunks = 0;
    bool hunk_open = false;
    while (offset < size) {
        if (!read_be32(data, size, &offset, &value)) return false;
        value &= HUNK_TYPE_MASK;
        if (value == HUNK_END) {
            if (!hunk_open) return false;
            hunk_open = false;
            if (++end_count > info->hunk_count) return false;
            continue;
        }
        if (value == HUNK_CODE || value == HUNK_DATA) {
            if (hunk_open) return false;
            uint32_t longs;
            if (!read_be32(data, size, &offset, &longs)) return false;
            size_t payload_offset = offset;
            if (!skip_longs(size, &offset, longs)) return false;
            if (++payload_hunks > info->hunk_count) return false;
            if (value == HUNK_CODE) {
                if (info->code_count++ == 0) {
                    info->first_code_offset = payload_offset;
                    info->first_code_size = (size_t)longs * 4;
                }
            } else {
                info->data_count++;
            }
            hunk_open = true;
        } else if (value == HUNK_BSS) {
            if (hunk_open) return false;
            if (!read_be32(data, size, &offset, &value)) return false;
            if (++payload_hunks > info->hunk_count) return false;
            info->bss_count++;
            hunk_open = true;
        } else if (value == HUNK_RELOC32) {
            if (!hunk_open) return false;
            uint32_t count;
            do {
                if (!read_be32(data, size, &offset, &count)) return false;
                if (count == 0) break;
                if (!read_be32(data, size, &offset, &value) || !skip_longs(size, &offset, count))
                    return false;
                info->reloc32_count += count;
            } while (count != 0);
        } else if (value == HUNK_SYMBOL) {
            if (!hunk_open) return false;
            for (;;) {
                uint32_t name_len;
                if (!read_be32(data, size, &offset, &name_len)) return false;
                if (name_len == 0) break;
                if (!skip_longs(size, &offset, name_len)) return false;
                uint32_t sym_val;
                if (!read_be32(data, size, &offset, &sym_val)) return false;
            }
        } else {
            return false;
        }
    }
    return !hunk_open && end_count == info->hunk_count &&
        payload_hunks == info->hunk_count &&
        info->hunk_count > 0;
}
