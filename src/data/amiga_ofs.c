#include "amiga_ofs.h"
#include "sha256.h"

#include <stdlib.h>
#include <string.h>

#define ADF_BLOCK_SIZE 512U
#define OFS_FILE_HEADER 2U
#define OFS_DATA_BLOCK 8U
#define OFS_FILE_SECONDARY 0xfffffffdu
#define OFS_DATA_PAYLOAD_OFFSET 24U
#define OFS_DATA_PAYLOAD_SIZE (ADF_BLOCK_SIZE - OFS_DATA_PAYLOAD_OFFSET)

static uint32_t read_be32(const uint8_t *data) {
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | data[3];
}

static int is_amiga_dos(const uint8_t *adf, size_t size) {
    return size >= ADF_BLOCK_SIZE && memcmp(adf, "DOS", 3) == 0 &&
        adf[3] <= 7U && (size % ADF_BLOCK_SIZE) == 0 &&
        size / ADF_BLOCK_SIZE <= UINT32_MAX;
}

static uint8_t *read_file_chain(const uint8_t *adf, size_t block_count,
                                uint32_t header_block, size_t *out_size) {
    if (!adf || block_count == 0U || header_block >= block_count) return NULL;
    const uint8_t *header = adf + (size_t)header_block * ADF_BLOCK_SIZE;
    uint32_t expected_size = read_be32(header + 324U);
    uint32_t current = read_be32(header + 16U);
    if (current == 0U) {
        if (expected_size != 0U) return NULL;
        uint8_t *empty = malloc(1U);
        if (empty && out_size) *out_size = 0;
        return empty;
    }
    if (current >= block_count) return NULL;

    uint8_t *out = NULL;
    size_t size = 0, capacity = 0;
    for (size_t visited = 0; current != 0U && visited < block_count; ++visited) {
        if (current >= block_count) { free(out); return NULL; }
        const uint8_t *block = adf + (size_t)current * ADF_BLOCK_SIZE;
        uint32_t payload_size = read_be32(block + 12U);
        uint32_t next = read_be32(block + 16U);
        if (read_be32(block) != OFS_DATA_BLOCK ||
            read_be32(block + 4U) != header_block ||
            payload_size > OFS_DATA_PAYLOAD_SIZE || next >= block_count) {
            free(out);
            return NULL;
        }
        if (payload_size > SIZE_MAX - size) { free(out); return NULL; }
        size_t required = size + payload_size;
        if (required > capacity) {
            size_t next_capacity;
            if (capacity == 0) {
                next_capacity = ADF_BLOCK_SIZE;
            } else if (capacity > SIZE_MAX / 2U) {
                next_capacity = required;
            } else {
                next_capacity = capacity * 2U;
            }
            while (next_capacity < required) {
                if (next_capacity > SIZE_MAX / 2U) { next_capacity = required; break; }
                next_capacity *= 2U;
            }
            uint8_t *grown = realloc(out, next_capacity);
            if (!grown) { free(out); return NULL; }
            out = grown;
            capacity = next_capacity;
        }
        memcpy(out + size, block + OFS_DATA_PAYLOAD_OFFSET, payload_size);
        size = required;
        current = next;
    }
    if (current != 0U || size == 0U || size != expected_size) {
        free(out);
        return NULL;
    }
    if (out_size) *out_size = size;
    return out;
}

int amiga_ofs_visit_file_hashes(const uint8_t *adf, size_t adf_size,
                                AmigaOFSHashVisitor visitor, void *context) {
    if (!adf || !visitor || !is_amiga_dos(adf, adf_size)) return 0;
    size_t blocks = adf_size / ADF_BLOCK_SIZE;
    int count = 0;
    for (uint32_t block = 2U; block < blocks; ++block) {
        const uint8_t *header = adf + (size_t)block * ADF_BLOCK_SIZE;
        if (read_be32(header) != OFS_FILE_HEADER || read_be32(header + 4U) != block ||
            read_be32(header + 508U) != OFS_FILE_SECONDARY) continue;
        size_t size = 0;
        uint8_t *file = read_file_chain(adf, blocks, block, &size);
        if (!file) continue;
        uint8_t digest[32];
        sha256_digest(file, size, digest);
        ++count;
        int keep_going = visitor(file, size, digest, context);
        free(file);
        if (!keep_going) break;
    }
    return count;
}

uint8_t *amiga_ofs_find_file_sha256(const uint8_t *adf, size_t adf_size,
                                    const char expected_sha256[65],
                                    size_t *out_size) {
    if (out_size) *out_size = 0;
    if (!adf || !expected_sha256 || !is_amiga_dos(adf, adf_size)) return NULL;
    size_t blocks = adf_size / ADF_BLOCK_SIZE;
    for (uint32_t block = 2U; block < blocks; ++block) {
        const uint8_t *header = adf + (size_t)block * ADF_BLOCK_SIZE;
        if (read_be32(header) != OFS_FILE_HEADER || read_be32(header + 4U) != block ||
            read_be32(header + 508U) != OFS_FILE_SECONDARY) continue;
        size_t size = 0;
        uint8_t *file = read_file_chain(adf, blocks, block, &size);
        if (!file) continue;
        uint8_t digest[32];
        sha256_digest(file, size, digest);
        if (sha256_matches_hex(digest, expected_sha256)) {
            if (out_size) *out_size = size;
            return file;
        }
        free(file);
    }
    return NULL;
}
