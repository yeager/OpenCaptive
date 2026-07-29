#include "adf_reader.h"
#include <stdlib.h>
#include <string.h>

static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}

static const uint8_t *block_ptr(const ADFDisk *disk, uint32_t block) {
    uint32_t offset = block * ADF_SECTOR_SIZE;
    if (offset + ADF_SECTOR_SIZE > disk->size) return NULL;
    return disk->data + offset;
}

bool adf_open(ADFDisk *disk, const uint8_t *data, size_t size) {
    if (!disk || !data || size < ADF_DISK_SIZE) return false;

    disk->data = data;
    disk->size = size;

    // Check boot block
    if (data[0] != 'D' || data[1] != 'O' || data[2] != 'S') return false;

    disk->fs_type = (data[3] & 1) ? ADF_FS_FFS : ADF_FS_OFS;

    // Read root block
    const uint8_t *root = block_ptr(disk, ADF_ROOT_BLOCK);
    if (!root) return false;

    // Root block type check: primary type = 2 (T_HEADER), secondary type = 1 (ST_ROOT)
    if (read_be32(root) != 2) return false;
    if (read_be32(root + ADF_SECTOR_SIZE - 4) != 1) return false;

    // Disk name: at offset 432, length byte + name
    int name_len = root[432];
    if (name_len > 30) name_len = 30;
    memcpy(disk->disk_name, root + 433, name_len);
    disk->disk_name[name_len] = '\0';

    return true;
}

int adf_list_root(const ADFDisk *disk, ADFEntry *entries, int max_entries) {
    const uint8_t *root = block_ptr(disk, ADF_ROOT_BLOCK);
    if (!root) return 0;

    // Hash table at offset 24, 72 entries (288 bytes)
    int count = 0;
    for (int i = 0; i < 72 && count < max_entries; i++) {
        uint32_t block_num = read_be32(root + 24 + i * 4);
        while (block_num != 0 && count < max_entries) {
            const uint8_t *hdr = block_ptr(disk, block_num);
            if (!hdr) break;

            // Check it's a header block
            if (read_be32(hdr) != 2) break;

            ADFEntry *e = &entries[count];
            e->header_block = block_num;

            uint32_t sec_type = read_be32(hdr + ADF_SECTOR_SIZE - 4);
            e->is_dir = (sec_type == 2); // ST_USERDIR

            // File size at offset 324
            e->size = read_be32(hdr + 324);

            // Name at offset 432
            int nlen = hdr[432];
            if (nlen > 30) nlen = 30;
            memcpy(e->name, hdr + 433, nlen);
            e->name[nlen] = '\0';

            count++;

            // Hash chain: next entry with same hash at offset 496
            block_num = read_be32(hdr + 496);
        }
    }

    return count;
}

uint8_t *adf_read_file(const ADFDisk *disk, uint32_t header_block, uint32_t *out_size) {
    const uint8_t *hdr = block_ptr(disk, header_block);
    if (!hdr) return NULL;

    uint32_t file_size = read_be32(hdr + 324);
    if (file_size == 0) return NULL;

    uint8_t *result = malloc(file_size);
    if (!result) return NULL;

    if (disk->fs_type == ADF_FS_OFS) {
        // OFS: data blocks with 24-byte header each, 488 bytes payload
        uint32_t first_data = read_be32(hdr + 16);
        uint32_t remaining = file_size;
        uint32_t written = 0;
        uint32_t data_block = first_data;

        while (data_block != 0 && remaining > 0) {
            const uint8_t *db = block_ptr(disk, data_block);
            if (!db) break;

            // OFS data block: type=8 at offset 0, data size at offset 12
            uint32_t data_size = read_be32(db + 12);
            if (data_size > 488) data_size = 488;
            if (data_size > remaining) data_size = remaining;

            memcpy(result + written, db + 24, data_size);
            written += data_size;
            remaining -= data_size;

            // Next data block
            data_block = read_be32(db + 16);
        }
    } else {
        // FFS: data blocks are pure 512-byte payload, block list in header
        // File header has up to 72 data block pointers at offset 24..311
        // Extension blocks for files > 72*512 bytes
        uint32_t remaining = file_size;
        uint32_t written = 0;
        uint32_t cur_header = header_block;

        while (remaining > 0 && cur_header != 0) {
            const uint8_t *h = block_ptr(disk, cur_header);
            if (!h) break;

            // Data block table: 72 entries, stored HIGH to LOW
            int num_blocks = read_be32(h + 8); // HighSeq
            if (num_blocks > 72) num_blocks = 72;

            for (int i = num_blocks - 1; i >= 0 && remaining > 0; i--) {
                uint32_t db = read_be32(h + 308 - i * 4);
                const uint8_t *dp = block_ptr(disk, db);
                if (!dp) break;

                uint32_t chunk = (remaining > 512) ? 512 : remaining;
                memcpy(result + written, dp, chunk);
                written += chunk;
                remaining -= chunk;
            }

            // Extension block
            cur_header = read_be32(h + ADF_SECTOR_SIZE - 8);
        }
    }

    if (out_size) *out_size = file_size;
    return result;
}
