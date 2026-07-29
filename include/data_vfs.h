#ifndef DATA_VFS_H
#define DATA_VFS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VFS_MAX_ZIPS 16

typedef struct {
    char data_path[512];
    char zip_paths[VFS_MAX_ZIPS][512];
    int num_zips;
    bool initialized;
} DataVFS;

bool vfs_init(DataVFS *vfs, const char *data_path);
void vfs_free(DataVFS *vfs);

// Read a file by relative path (e.g. "CAPICS/WALLA.PL5").
// Checks loose files on disk first, then inside ZIP archives.
// Returns malloc'd buffer, caller must free. Sets *out_size.
uint8_t *vfs_read_file(const DataVFS *vfs, const char *rel_path, size_t *out_size);

// Check if a file exists (on disk or in any ZIP)
bool vfs_file_exists(const DataVFS *vfs, const char *rel_path);

#endif
