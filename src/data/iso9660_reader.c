#include "iso9660_reader.h"
#include "sha256.h"
#include <stdlib.h>
#include <string.h>

static uint32_t read_le32(const uint8_t *p) {
    return p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}

static const uint8_t *sector_ptr(const ISOImage *iso, uint32_t lba) {
    if (iso->raw_mode) {
        // MODE1/2352: 16 bytes sync/header, then 2048 bytes user data
        uint64_t offset = (uint64_t)lba * ISO_RAW_SECTOR + 16;
        if (offset + ISO_SECTOR_SIZE > iso->size) return NULL;
        return iso->data + offset;
    } else {
        uint64_t offset = (uint64_t)lba * ISO_SECTOR_SIZE;
        if (offset + ISO_SECTOR_SIZE > iso->size) return NULL;
        return iso->data + offset;
    }
}

static bool parse_pvd(ISOImage *iso) {
    // Primary Volume Descriptor at sector 16
    const uint8_t *pvd = sector_ptr(iso, 16);
    if (!pvd) return false;

    // Type 1 = PVD, identifier "CD001"
    if (pvd[0] != 1 || memcmp(pvd + 1, "CD001", 5) != 0) return false;

    // Volume ID at offset 40, 32 bytes
    memcpy(iso->volume_id, pvd + 40, 32);
    iso->volume_id[32] = '\0';
    // Trim trailing spaces
    for (int i = 31; i >= 0 && iso->volume_id[i] == ' '; i--)
        iso->volume_id[i] = '\0';

    // Root directory record at offset 156
    const uint8_t *root_rec = pvd + 156;
    iso->root_lba = read_le32(root_rec + 2);
    iso->root_size = read_le32(root_rec + 10);

    return true;
}

bool iso_open(ISOImage *iso, const uint8_t *data, size_t size) {
    memset(iso, 0, sizeof(*iso));
    iso->data = data;
    iso->size = size;
    iso->raw_mode = false;
    return parse_pvd(iso);
}

bool iso_open_raw(ISOImage *iso, const uint8_t *data, size_t size) {
    memset(iso, 0, sizeof(*iso));
    iso->data = data;
    iso->size = size;
    iso->raw_mode = true;
    return parse_pvd(iso);
}

int iso_list_dir(const ISOImage *iso, uint32_t dir_lba, uint32_t dir_size,
                 ISOEntry *entries, int max_entries) {
    int count = 0;
    uint32_t sectors = (dir_size + ISO_SECTOR_SIZE - 1) / ISO_SECTOR_SIZE;

    for (uint32_t s = 0; s < sectors && count < max_entries; s++) {
        const uint8_t *sector = sector_ptr(iso, dir_lba + s);
        if (!sector) break;

        uint32_t pos = 0;
        while (pos + 33 < ISO_SECTOR_SIZE && count < max_entries) {
            uint8_t rec_len = sector[pos];
            if (rec_len < 33) break;
            if (pos + rec_len > ISO_SECTOR_SIZE) break;

            uint8_t name_len = sector[pos + 32];
            if (name_len > rec_len - 33) break;
            if (name_len == 0) { pos += rec_len; continue; }

            // Skip "." and ".." entries
            if (name_len == 1 && (sector[pos + 33] == 0 || sector[pos + 33] == 1)) {
                pos += rec_len;
                continue;
            }

            ISOEntry *e = &entries[count];
            e->lba = read_le32(sector + pos + 2);
            e->size = read_le32(sector + pos + 10);
            e->is_dir = (sector[pos + 25] & 0x02) != 0;

            int copy_len = (name_len > 63) ? 63 : name_len;
            memcpy(e->name, sector + pos + 33, copy_len);
            e->name[copy_len] = '\0';

            // Remove ";1" version suffix
            char *semi = strchr(e->name, ';');
            if (semi) *semi = '\0';

            count++;
            pos += rec_len;
        }
    }

    return count;
}

int iso_list_root(const ISOImage *iso, ISOEntry *entries, int max_entries) {
    return iso_list_dir(iso, iso->root_lba, iso->root_size, entries, max_entries);
}

uint8_t *iso_read_file(const ISOImage *iso, uint32_t lba, uint32_t size) {
    uint8_t *result = malloc(size);
    if (!result) return NULL;

    uint32_t remaining = size;
    uint32_t written = 0;
    uint32_t cur_lba = lba;

    while (remaining > 0) {
        const uint8_t *sector = sector_ptr(iso, cur_lba);
        if (!sector) { free(result); return NULL; }

        uint32_t chunk = (remaining > ISO_SECTOR_SIZE) ? ISO_SECTOR_SIZE : remaining;
        memcpy(result + written, sector, chunk);
        written += chunk;
        remaining -= chunk;
        cur_lba++;
    }

    return result;
}

uint8_t *iso_read_file_sha256(const ISOImage *iso, const char *expected_sha256,
                              size_t *out_size) {
    if (out_size) *out_size = 0;
    if (!iso || !expected_sha256) return NULL;

    ISOEntry entries[256];
    int count = iso_list_root(iso, entries, 256);
    for (int i = 0; i < count; i++) {
        if (entries[i].is_dir) continue;
        uint8_t *file = iso_read_file(iso, entries[i].lba, entries[i].size);
        if (!file) continue;

        uint8_t digest[32];
        sha256_digest(file, entries[i].size, digest);
        if (sha256_matches_hex(digest, expected_sha256)) {
            if (out_size) *out_size = entries[i].size;
            return file;
        }
        free(file);
    }
    return NULL;
}
