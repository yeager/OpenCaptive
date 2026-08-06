#include "st_disk_reader.h"
#include <stdlib.h>
#include <string.h>

static uint16_t read_le16(const uint8_t *p) {
    return p[0] | ((uint16_t)p[1] << 8);
}

bool st_disk_open(STDisk *disk, const uint8_t *data, size_t size) {
    if (!disk) return false;
    memset(disk, 0, sizeof(*disk));
    if (!data || size < 512) return false;

    STDisk candidate = {0};
    candidate.data = data;
    candidate.size = size;

    // BPB at offset 11
    uint16_t bytes_per_sector = read_le16(data + 11);
    if (bytes_per_sector != 512) return false;

    candidate.sectors_per_cluster = data[13];
    candidate.reserved_sectors = read_le16(data + 14);
    candidate.num_fats = data[16];
    candidate.root_entries = read_le16(data + 17);
    candidate.total_sectors = read_le16(data + 19);
    candidate.sectors_per_fat = read_le16(data + 22);

    if (candidate.sectors_per_cluster == 0 || candidate.num_fats == 0 ||
        candidate.total_sectors == 0) return false;

    uint64_t fat_end = ((uint64_t)candidate.reserved_sectors +
                        (uint64_t)candidate.num_fats * candidate.sectors_per_fat) *
                       ST_SECTOR_SIZE;
    uint64_t root_dir_bytes = (uint64_t)candidate.root_entries * 32U;
    /* The root directory occupies complete sectors on FAT12/FAT16 disks.
     * Rounding here is required when a malformed or unusual BPB declares a
     * number of entries that is not itself sector-aligned; otherwise cluster
     * 2 is read from inside the final root-directory sector. */
    uint64_t root_dir_size = (root_dir_bytes + ST_SECTOR_SIZE - 1U) /
                             ST_SECTOR_SIZE * ST_SECTOR_SIZE;
    uint64_t data_area_offset = fat_end + root_dir_size;
    uint64_t declared_disk_size = (uint64_t)candidate.total_sectors * ST_SECTOR_SIZE;

    if (fat_end > UINT32_MAX || data_area_offset > UINT32_MAX ||
        fat_end > size || data_area_offset > size ||
        declared_disk_size > size || data_area_offset >= declared_disk_size ||
        (uint64_t)candidate.reserved_sectors * ST_SECTOR_SIZE +
            (uint64_t)candidate.num_fats * candidate.sectors_per_fat * ST_SECTOR_SIZE > size)
        return false;

    candidate.root_dir_offset = (uint32_t)fat_end;
    candidate.data_area_offset = (uint32_t)data_area_offset;

    *disk = candidate;
    return true;
}

static void format_name(const uint8_t *raw, char *out) {
    // Convert 8.3 dir entry name to readable form
    int j = 0;
    for (int i = 0; i < 8 && raw[i] != ' '; i++)
        out[j++] = raw[i];
    if (raw[8] != ' ') {
        out[j++] = '.';
        for (int i = 8; i < 11 && raw[i] != ' '; i++)
            out[j++] = raw[i];
    }
    out[j] = '\0';
}

int st_disk_list_root(const STDisk *disk, STEntry *entries, int max_entries) {
    if (!disk || !disk->data || !entries || max_entries <= 0) return 0;
    int count = 0;
    for (int i = 0; i < disk->root_entries && count < max_entries; i++) {
        uint64_t offset64 = (uint64_t)disk->root_dir_offset + (uint64_t)i * 32U;
        if (offset64 + 32U > disk->size) break;
        uint32_t offset = (uint32_t)offset64;

        const uint8_t *entry = disk->data + offset;
        if (entry[0] == 0x00) break;     // end of directory
        if (entry[0] == 0xE5) continue;   // deleted
        if (entry[11] == 0x0F) continue;  // long name entry
        if (entry[11] & 0x08) continue;   // volume label

        STEntry *e = &entries[count];
        format_name(entry, e->name);
        e->cluster = read_le16(entry + 26);
        e->size = (uint32_t)entry[28] | ((uint32_t)entry[29] << 8) |
                  ((uint32_t)entry[30] << 16) | ((uint32_t)entry[31] << 24);
        e->is_dir = (entry[11] & 0x10) != 0;
        e->offset = 0;
        if (e->cluster >= 2) {
            uint64_t cluster_offset = (uint64_t)disk->data_area_offset +
                (uint64_t)(e->cluster - 2U) * disk->sectors_per_cluster *
                ST_SECTOR_SIZE;
            if (cluster_offset < disk->size && cluster_offset <= UINT32_MAX)
                e->offset = (uint32_t)cluster_offset;
        }
        count++;
    }
    return count;
}

static uint16_t fat12_next(const uint8_t *fat, uint16_t cluster) {
    uint32_t offset = cluster + cluster / 2;
    uint16_t value = fat[offset] | ((uint16_t)fat[offset + 1] << 8);
    if (cluster & 1)
        value >>= 4;
    else
        value &= 0x0FFF;
    return value;
}

uint8_t *st_disk_read_file(const STDisk *disk, uint16_t first_cluster,
                            uint32_t file_size, uint32_t *out_size) {
    if (out_size) *out_size = 0;
    if (!disk || !disk->data || first_cluster < 2) return NULL;
    if (disk->data_area_offset > disk->size ||
        (uint64_t)file_size > (uint64_t)(disk->size - disk->data_area_offset))
        return NULL;

    uint8_t *result = malloc(file_size ? file_size : 1U);
    if (!result) return NULL;
    if (file_size == 0) {
        if (out_size) *out_size = 0;
        return result;
    }

    const uint8_t *fat = disk->data + disk->reserved_sectors * ST_SECTOR_SIZE;
    size_t fat_size = (size_t)disk->sectors_per_fat * ST_SECTOR_SIZE;
    uint32_t cluster_size = disk->sectors_per_cluster * ST_SECTOR_SIZE;
    uint16_t cluster = first_cluster;
    uint32_t written = 0;
    size_t clusters_left = (disk->size > disk->data_area_offset && cluster_size != 0) ?
        (disk->size - disk->data_area_offset + cluster_size - 1U) / cluster_size : 0;

    while (cluster >= 2 && cluster < 0xFF0 && written < file_size && clusters_left-- > 0) {
        size_t offset = (size_t)disk->data_area_offset +
                        (size_t)(cluster - 2) * cluster_size;
        if (offset > disk->size || cluster_size > disk->size - offset) break;

        uint32_t chunk = file_size - written;
        if (chunk > cluster_size) chunk = cluster_size;

        memcpy(result + written, disk->data + offset, chunk);
        written += chunk;

        uint32_t fat_offset = cluster + cluster / 2;
        if (fat_offset + 1 >= fat_size) {
            free(result);
            return NULL;
        }
        cluster = fat12_next(fat, cluster);
    }

    if (written != file_size) {
        free(result);
        return NULL;
    }
    if (out_size) *out_size = written;
    return result;
}
