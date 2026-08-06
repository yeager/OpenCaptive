#include "iso9660_reader.h"
#include "sha256.h"
#include <stdlib.h>
#include <string.h>

#define ISO_MAX_FILE_SIZE (256U * 1024U * 1024U)

static uint32_t read_le32(const uint8_t *p) {
    return p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}

static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static const uint8_t *sector_ptr(const ISOImage *iso, uint32_t lba) {
    if (!iso || !iso->data) return NULL;
    if (iso->volume_space_size != 0 && lba >= iso->volume_space_size)
        return NULL;
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

    // Both-endian copies must agree and define the logical volume boundary.
    uint32_t volume_space_size = read_le32(pvd + 80);
    if (volume_space_size == 0 || read_be32(pvd + 84) != volume_space_size)
        return false;
    iso->volume_space_size = volume_space_size;

    // Volume ID at offset 40, 32 bytes
    memcpy(iso->volume_id, pvd + 40, 32);
    iso->volume_id[32] = '\0';
    // Trim trailing spaces
    for (int i = 31; i >= 0 && iso->volume_id[i] == ' '; i--)
        iso->volume_id[i] = '\0';

    // Root directory record at offset 156
    const uint8_t *root_rec = pvd + 156;
    if (root_rec[0] < 34 || (root_rec[25] & 0x02U) == 0) return false;
    iso->root_lba = read_le32(root_rec + 2);
    iso->root_size = read_le32(root_rec + 10);
    if (iso->root_size == 0) return false;
    uint64_t root_sectors = ((uint64_t)iso->root_size + ISO_SECTOR_SIZE - 1U) /
                            ISO_SECTOR_SIZE;
    if (root_sectors == 0 ||
        root_sectors > (uint64_t)UINT32_MAX - iso->root_lba + 1U ||
        !sector_ptr(iso, iso->root_lba + (uint32_t)root_sectors - 1U))
        return false;

    return true;
}

bool iso_open(ISOImage *iso, const uint8_t *data, size_t size) {
    if (!iso) return false;
    memset(iso, 0, sizeof(*iso));
    if (!data) return false;
    ISOImage candidate = {0};
    candidate.data = data;
    candidate.size = size;
    candidate.raw_mode = false;
    if (!parse_pvd(&candidate)) return false;
    *iso = candidate;
    return true;
}

bool iso_open_raw(ISOImage *iso, const uint8_t *data, size_t size) {
    if (!iso) return false;
    memset(iso, 0, sizeof(*iso));
    if (!data) return false;
    ISOImage candidate = {0};
    candidate.data = data;
    candidate.size = size;
    candidate.raw_mode = true;
    if (!parse_pvd(&candidate)) return false;
    *iso = candidate;
    return true;
}

int iso_list_dir(const ISOImage *iso, uint32_t dir_lba, uint32_t dir_size,
                 ISOEntry *entries, int max_entries) {
    if (!iso || !entries || max_entries <= 0 || dir_size == 0) return 0;
    int count = 0;
    uint32_t sectors = (uint32_t)(((uint64_t)dir_size + ISO_SECTOR_SIZE - 1U) /
                                  ISO_SECTOR_SIZE);

    for (uint32_t s = 0; s < sectors && count < max_entries; s++) {
        if (dir_lba > UINT32_MAX - s) break;
        const uint8_t *sector = sector_ptr(iso, dir_lba + s);
        if (!sector) break;

        uint64_t sector_start = (uint64_t)s * ISO_SECTOR_SIZE;
        uint32_t valid_bytes = (dir_size > sector_start) ?
            (uint32_t)((dir_size - sector_start > ISO_SECTOR_SIZE) ?
                       ISO_SECTOR_SIZE : dir_size - sector_start) : 0U;
        uint32_t pos = 0;
        while (pos + 33 <= valid_bytes && count < max_entries) {
            uint8_t rec_len = sector[pos];
            if (rec_len < 33) break;
            if (pos + rec_len > valid_bytes) break;

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
    if (!iso) return 0;
    return iso_list_dir(iso, iso->root_lba, iso->root_size, entries, max_entries);
}

uint8_t *iso_read_file(const ISOImage *iso, uint32_t lba, uint32_t size) {
    if (!iso || !iso->data || size > ISO_MAX_FILE_SIZE ||
        iso->volume_space_size == 0 || lba >= iso->volume_space_size)
        return NULL;
    uint64_t sectors = ((uint64_t)size + ISO_SECTOR_SIZE - 1U) /
                       ISO_SECTOR_SIZE;
    if (sectors > (uint64_t)iso->volume_space_size - lba) return NULL;
    uint8_t *result = malloc(size ? size : 1U);
    if (!result) return NULL;
    if (size == 0) return result;

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
        if (remaining > 0 && cur_lba == UINT32_MAX) {
            free(result);
            return NULL;
        }
        cur_lba++;
    }

    return result;
}

/* ISO directory traversal needs the current directory's extent.  Keep this
 * separate from the public hash API so callers cannot accidentally fall back
 * to path/name identity. */
static uint8_t *iso_find_hash_in_dir(const ISOImage *iso, uint32_t dir_lba,
                                     uint32_t dir_size,
                                     const char *expected_sha256,
                                     size_t *out_size, unsigned depth) {
    if (depth > 16U) return NULL;
    ISOEntry entries[256];
    int count = iso_list_dir(iso, dir_lba, dir_size, entries, 256);
    for (int i = 0; i < count; i++) {
        if (entries[i].is_dir) {
            uint8_t *nested = iso_find_hash_in_dir(iso, entries[i].lba,
                                                   entries[i].size,
                                                   expected_sha256, out_size,
                                                   depth + 1U);
            if (nested) return nested;
            continue;
        }
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

uint8_t *iso_read_file_sha256(const ISOImage *iso, const char *expected_sha256,
                              size_t *out_size) {
    if (out_size) *out_size = 0;
    if (!iso || !expected_sha256) return NULL;
    return iso_find_hash_in_dir(iso, iso->root_lba, iso->root_size,
                                expected_sha256, out_size, 0U);
}
