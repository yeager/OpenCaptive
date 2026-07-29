#include "st_disk_reader.h"
#include <stdlib.h>
#include <string.h>

static uint16_t read_le16(const uint8_t *p) {
    return p[0] | ((uint16_t)p[1] << 8);
}

bool st_disk_open(STDisk *disk, const uint8_t *data, size_t size) {
    if (!disk || !data || size < 512) return false;

    memset(disk, 0, sizeof(*disk));
    disk->data = data;
    disk->size = size;

    // BPB at offset 11
    uint16_t bytes_per_sector = read_le16(data + 11);
    if (bytes_per_sector != 512) return false;

    disk->sectors_per_cluster = data[13];
    disk->reserved_sectors = read_le16(data + 14);
    disk->num_fats = data[16];
    disk->root_entries = read_le16(data + 17);
    disk->total_sectors = read_le16(data + 19);
    disk->sectors_per_fat = read_le16(data + 22);

    if (disk->sectors_per_cluster == 0 || disk->num_fats == 0) return false;

    disk->root_dir_offset = (disk->reserved_sectors + disk->num_fats * disk->sectors_per_fat)
                            * ST_SECTOR_SIZE;
    uint32_t root_dir_size = disk->root_entries * 32;
    disk->data_area_offset = disk->root_dir_offset + root_dir_size;

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
    int count = 0;
    for (int i = 0; i < disk->root_entries && count < max_entries; i++) {
        uint32_t offset = disk->root_dir_offset + i * 32;
        if (offset + 32 > disk->size) break;

        const uint8_t *entry = disk->data + offset;
        if (entry[0] == 0x00) break;     // end of directory
        if (entry[0] == 0xE5) continue;   // deleted
        if (entry[11] == 0x0F) continue;  // long name entry
        if (entry[11] & 0x08) continue;   // volume label

        STEntry *e = &entries[count];
        format_name(entry, e->name);
        e->cluster = read_le16(entry + 26);
        e->size = entry[28] | (entry[29]<<8) | (entry[30]<<16) | (entry[31]<<24);
        e->is_dir = (entry[11] & 0x10) != 0;
        e->offset = disk->data_area_offset +
                    (e->cluster - 2) * disk->sectors_per_cluster * ST_SECTOR_SIZE;
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
    if (file_size == 0) return NULL;

    uint8_t *result = malloc(file_size);
    if (!result) return NULL;

    const uint8_t *fat = disk->data + disk->reserved_sectors * ST_SECTOR_SIZE;
    uint32_t cluster_size = disk->sectors_per_cluster * ST_SECTOR_SIZE;
    uint16_t cluster = first_cluster;
    uint32_t written = 0;

    while (cluster >= 2 && cluster < 0xFF0 && written < file_size) {
        uint32_t offset = disk->data_area_offset +
                          (cluster - 2) * cluster_size;
        if (offset + cluster_size > disk->size) break;

        uint32_t chunk = file_size - written;
        if (chunk > cluster_size) chunk = cluster_size;

        memcpy(result + written, disk->data + offset, chunk);
        written += chunk;

        cluster = fat12_next(fat, cluster);
    }

    if (out_size) *out_size = written;
    return result;
}
