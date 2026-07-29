#ifndef ADF_READER_H
#define ADF_READER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Amiga ADF (Amiga Disk File) - 880KB floppy image
// 80 tracks × 2 sides × 11 sectors × 512 bytes = 901120 bytes
#define ADF_DISK_SIZE   901120
#define ADF_SECTOR_SIZE 512
#define ADF_ROOT_BLOCK  880  // standard root block location

typedef enum {
    ADF_FS_OFS = 0,  // Old File System
    ADF_FS_FFS = 1,  // Fast File System
} ADFFilesystem;

typedef struct {
    char     name[32];
    uint32_t header_block;
    uint32_t size;
    bool     is_dir;
} ADFEntry;

typedef struct {
    const uint8_t *data;
    size_t         size;
    ADFFilesystem  fs_type;
    char           disk_name[32];
} ADFDisk;

bool adf_open(ADFDisk *disk, const uint8_t *data, size_t size);
int  adf_list_root(const ADFDisk *disk, ADFEntry *entries, int max_entries);
uint8_t *adf_read_file(const ADFDisk *disk, uint32_t header_block, uint32_t *out_size);

#endif
