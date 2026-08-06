#ifndef DATA_VFS_H
#define DATA_VFS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
/* MSVC exposes the file-mode predicates under underscored names. Keep the
 * POSIX spelling used by the portable scanner and launcher code. */
#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & _S_IFMT) == _S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#endif
#endif

#define VFS_MAX_ZIPS 64

typedef struct {
    char data_path[512];
    char zip_paths[VFS_MAX_ZIPS][512];
    int num_zips;
    bool initialized;
    char cache_signature[65];
    bool cache_signature_valid;
    char cache_probe_signature[65];
    bool cache_probe_signature_valid;
} DataVFS;

bool vfs_init(DataVFS *vfs, const char *data_path);
void vfs_free(DataVFS *vfs);

// Read a file by relative path (e.g. "CAPICS/WALLA.PL5").
// Checks loose files on disk first, then inside ZIP archives.
// Returns malloc'd buffer, caller must free. Sets *out_size.
uint8_t *vfs_read_file(const DataVFS *vfs, const char *rel_path, size_t *out_size);

// Find archive content by its SHA-256 digest. File names are not part of the
// identity decision. Returns malloc'd content on success.
uint8_t *vfs_find_sha256(const DataVFS *vfs, const char expected_sha256[65],
                         size_t *out_size);

// Check if a file exists (on disk or in any ZIP)
bool vfs_file_exists(const DataVFS *vfs, const char *rel_path);

#endif
