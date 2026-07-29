#ifndef ST_DISK_READER_H
#define ST_DISK_READER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Atari ST disk image reader (FAT12)
// Standard ST disks: 80 tracks × 2 sides × 9 sectors × 512 bytes = 720KB (737280 bytes)
// Or 10 sectors = 800KB, or 11 sectors = 880KB

#define ST_SECTOR_SIZE 512

typedef struct {
    char     name[13];  // 8.3 format
    uint32_t offset;    // byte offset in disk image
    uint32_t size;
    uint16_t cluster;   // first cluster
    bool     is_dir;
} STEntry;

typedef struct {
    const uint8_t *data;
    size_t         size;
    uint16_t       sectors_per_cluster;
    uint16_t       reserved_sectors;
    uint8_t        num_fats;
    uint16_t       root_entries;
    uint16_t       sectors_per_fat;
    uint16_t       total_sectors;
    uint32_t       root_dir_offset;
    uint32_t       data_area_offset;
} STDisk;

bool st_disk_open(STDisk *disk, const uint8_t *data, size_t size);
int  st_disk_list_root(const STDisk *disk, STEntry *entries, int max_entries);
uint8_t *st_disk_read_file(const STDisk *disk, uint16_t first_cluster,
                            uint32_t file_size, uint32_t *out_size);

#endif
