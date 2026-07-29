#ifndef ISO9660_READER_H
#define ISO9660_READER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define ISO_SECTOR_SIZE 2048
#define ISO_RAW_SECTOR  2352  // MODE1/2352 raw sector

typedef struct {
    char     name[64];
    uint32_t lba;       // logical block address
    uint32_t size;
    bool     is_dir;
} ISOEntry;

typedef struct {
    const uint8_t *data;
    size_t         size;
    bool           raw_mode;     // true if MODE1/2352 sectors
    char           volume_id[33];
    uint32_t       root_lba;
    uint32_t       root_size;
} ISOImage;

bool iso_open(ISOImage *iso, const uint8_t *data, size_t size);
bool iso_open_raw(ISOImage *iso, const uint8_t *data, size_t size);
int  iso_list_dir(const ISOImage *iso, uint32_t dir_lba, uint32_t dir_size,
                  ISOEntry *entries, int max_entries);
int  iso_list_root(const ISOImage *iso, ISOEntry *entries, int max_entries);
uint8_t *iso_read_file(const ISOImage *iso, uint32_t lba, uint32_t size);

#endif
