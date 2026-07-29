#include "data_vfs.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

// ZIP format constants
#define ZIP_LOCAL_SIG    0x04034b50
#define ZIP_CENTRAL_SIG  0x02014b50
#define ZIP_END_SIG      0x06054b50
#define ZIP_METHOD_STORE 0
#define ZIP_METHOD_DEFLATE 8

/* Archives are user supplied. Keep metadata and allocations bounded before
 * using any value from an archive header. */
#define ZIP_MAX_ENTRY_NAME 511
#define ZIP_MAX_ENTRY_SIZE (256u * 1024u * 1024u)

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static bool path_matches(const char *zip_name, int zip_len, const char *rel_path) {
    int rel_len = (int)strlen(rel_path);

    // Skip leading directory in zip entry (e.g. "Captive_DOS_EN/CAPICS/WALLA.PL5")
    // Try exact match first
    if (zip_len == rel_len) {
        for (int i = 0; i < rel_len; i++) {
            char a = zip_name[i], b = rel_path[i];
            if (a == '\\') a = '/';
            if (b == '\\') b = '/';
            if (a >= 'a' && a <= 'z') a -= 32;
            if (b >= 'a' && b <= 'z') b -= 32;
            if (a != b) goto try_suffix;
        }
        return true;
    }

try_suffix:
    // Try suffix match: zip entry ends with /rel_path
    if (zip_len > rel_len && (zip_name[zip_len - rel_len - 1] == '/' ||
                               zip_name[zip_len - rel_len - 1] == '\\')) {
        const char *suffix = zip_name + zip_len - rel_len;
        for (int i = 0; i < rel_len; i++) {
            char a = suffix[i], b = rel_path[i];
            if (a == '\\') a = '/';
            if (b == '\\') b = '/';
            if (a >= 'a' && a <= 'z') a -= 32;
            if (b >= 'a' && b <= 'z') b -= 32;
            if (a != b) return false;
        }
        return true;
    }
    return false;
}

static uint8_t *zip_extract(const char *zip_path, const char *rel_path, size_t *out_size) {
    FILE *f = fopen(zip_path, "rb");
    if (!f) return NULL;
    if (!out_size) { fclose(f); return NULL; }

    fseek(f, 0, SEEK_END);
    long zip_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (zip_size < 22) { fclose(f); return NULL; }

    // Find End of Central Directory (scan backwards)
    long search_start = zip_size - 22;
    if (search_start > zip_size - 65557) search_start = zip_size - 65557;
    if (search_start < 0) search_start = 0;

    uint8_t eocd_buf[65557];
    long search_len = zip_size - search_start;
    fseek(f, search_start, SEEK_SET);
    if (fread(eocd_buf, 1, search_len, f) != (size_t)search_len) {
        fclose(f);
        return NULL;
    }

    long eocd_offset = -1;
    for (long i = search_len - 22; i >= 0; i--) {
        if (read_u32(eocd_buf + i) == ZIP_END_SIG) {
            eocd_offset = search_start + i;
            break;
        }
    }
    if (eocd_offset < 0) { fclose(f); return NULL; }

    fseek(f, eocd_offset, SEEK_SET);
    uint8_t eocd[22];
    if (fread(eocd, 1, 22, f) != 22) { fclose(f); return NULL; }

    uint16_t num_entries = read_u16(eocd + 10);
    uint32_t cd_size = read_u32(eocd + 12);
    uint32_t cd_offset = read_u32(eocd + 16);

    if ((uint64_t)cd_offset + cd_size > (uint64_t)zip_size) {
        fclose(f);
        return NULL;
    }

    // Read central directory
    fseek(f, cd_offset, SEEK_SET);
    uint8_t cd_hdr[46];

    for (uint16_t i = 0; i < num_entries; i++) {
        if (fread(cd_hdr, 1, 46, f) != 46) break;
        if (read_u32(cd_hdr) != ZIP_CENTRAL_SIG) break;

        uint16_t method = read_u16(cd_hdr + 10);
        uint32_t comp_size = read_u32(cd_hdr + 20);
        uint32_t uncomp_size = read_u32(cd_hdr + 24);
        uint16_t name_len = read_u16(cd_hdr + 28);
        uint16_t extra_len = read_u16(cd_hdr + 30);
        uint16_t comment_len = read_u16(cd_hdr + 32);
        uint32_t local_offset = read_u32(cd_hdr + 42);

        if (name_len == 0 || name_len > ZIP_MAX_ENTRY_NAME ||
            comp_size > ZIP_MAX_ENTRY_SIZE || uncomp_size > ZIP_MAX_ENTRY_SIZE) {
            fclose(f);
            return NULL;
        }

        char name[ZIP_MAX_ENTRY_NAME + 1];
        int read_len = name_len;
        if (fread(name, 1, name_len, f) != name_len) break;
        name[read_len] = '\0';

        // Skip extra + comment
        long after_name = ftell(f);
        if (after_name < 0 || (uint64_t)after_name + extra_len + comment_len >
            (uint64_t)zip_size) break;
        fseek(f, extra_len + comment_len, SEEK_CUR);

        if (name[read_len - 1] == '/') continue; // directory

        if (!path_matches(name, read_len, rel_path)) continue;

        // Found match — read from local file header
        if ((uint64_t)local_offset + 30 > (uint64_t)zip_size) break;
        fseek(f, local_offset, SEEK_SET);
        uint8_t local_hdr[30];
        if (fread(local_hdr, 1, 30, f) != 30) break;
        if (read_u32(local_hdr) != ZIP_LOCAL_SIG) break;

        uint16_t local_name_len = read_u16(local_hdr + 26);
        uint16_t local_extra_len = read_u16(local_hdr + 28);
        long data_offset = ftell(f);
        if (data_offset < 0 || (uint64_t)data_offset + local_name_len + local_extra_len + comp_size >
            (uint64_t)zip_size) break;
        fseek(f, local_name_len + local_extra_len, SEEK_CUR);

        uint8_t *comp_data = malloc(comp_size);
        if (!comp_data) break;
        if (fread(comp_data, 1, comp_size, f) != comp_size) {
            free(comp_data);
            break;
        }
        fclose(f);

        if (method == ZIP_METHOD_STORE) {
            *out_size = comp_size;
            return comp_data;
        }

        if (method == ZIP_METHOD_DEFLATE) {
            uint8_t *out = malloc(uncomp_size);
            if (!out) { free(comp_data); return NULL; }

            z_stream strm = {0};
            strm.next_in = comp_data;
            strm.avail_in = comp_size;
            strm.next_out = out;
            strm.avail_out = uncomp_size;

            if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) {
                free(comp_data);
                free(out);
                return NULL;
            }
            int ret = inflate(&strm, Z_FINISH);
            inflateEnd(&strm);
            free(comp_data);

            if (ret != Z_STREAM_END) {
                free(out);
                return NULL;
            }
            *out_size = uncomp_size;
            return out;
        }

        free(comp_data);
        return NULL;
    }

    fclose(f);
    return NULL;
}

static uint8_t *zip_find_sha256(const char *zip_path, const char expected_sha256[65],
                                size_t *out_size) {
    FILE *f = fopen(zip_path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long zip_size = ftell(f);
    if (zip_size < 22) { fclose(f); return NULL; }
    long start = zip_size > 65557 ? zip_size - 65557 : 0;
    uint8_t tail[65557];
    long tail_len = zip_size - start;
    fseek(f, start, SEEK_SET);
    if (fread(tail, 1, tail_len, f) != (size_t)tail_len) { fclose(f); return NULL; }
    long eocd = -1;
    for (long i = tail_len - 22; i >= 0; i--)
        if (read_u32(tail + i) == ZIP_END_SIG) { eocd = start + i; break; }
    if (eocd < 0) { fclose(f); return NULL; }
    fseek(f, eocd, SEEK_SET);
    uint8_t end[22];
    if (fread(end, 1, sizeof(end), f) != sizeof(end)) { fclose(f); return NULL; }
    uint16_t entries = read_u16(end + 10);
    uint32_t cd_size = read_u32(end + 12), cd_offset = read_u32(end + 16);
    if ((uint64_t)cd_offset + cd_size > (uint64_t)zip_size) { fclose(f); return NULL; }
    fseek(f, cd_offset, SEEK_SET);
    for (uint16_t i = 0; i < entries; i++) {
        uint8_t hdr[46];
        if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr) || read_u32(hdr) != ZIP_CENTRAL_SIG) break;
        uint16_t name_len = read_u16(hdr + 28), extra_len = read_u16(hdr + 30), comment_len = read_u16(hdr + 32);
        if (!name_len || name_len > ZIP_MAX_ENTRY_NAME) break;
        char name[ZIP_MAX_ENTRY_NAME + 1];
        if (fread(name, 1, name_len, f) != name_len) break;
        name[name_len] = '\0';
        if (fseek(f, extra_len + comment_len, SEEK_CUR) != 0) break;
        if (name[name_len - 1] == '/') continue;
        size_t size = 0;
        uint8_t *data = zip_extract(zip_path, name, &size);
        if (!data) continue;
        uint8_t digest[32]; sha256_digest(data, size, digest);
        if (sha256_matches_hex(digest, expected_sha256)) {
            fclose(f); if (out_size) *out_size = size; return data;
        }
        free(data);
    }
    fclose(f);
    return NULL;
}

static void scan_for_zips(DataVFS *vfs) {
    vfs->num_zips = 0;

#ifdef _WIN32
    char pattern[600];
    snprintf(pattern, sizeof(pattern), "%s\\*.zip", vfs->data_path);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            vfs->num_zips < VFS_MAX_ZIPS) {
            snprintf(vfs->zip_paths[vfs->num_zips], 512,
                     "%s\\%s", vfs->data_path, fd.cFileName);
            vfs->num_zips++;
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR *d = opendir(vfs->data_path);
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && vfs->num_zips < VFS_MAX_ZIPS) {
        int len = (int)strlen(ent->d_name);
        if (len > 4 && strcasecmp(ent->d_name + len - 4, ".zip") == 0) {
            snprintf(vfs->zip_paths[vfs->num_zips], 512,
                     "%s/%s", vfs->data_path, ent->d_name);
            vfs->num_zips++;
        }
    }
    closedir(d);
#endif
}

bool vfs_init(DataVFS *vfs, const char *data_path) {
    memset(vfs, 0, sizeof(*vfs));
    if (!data_path || !data_path[0]) return false;
    strncpy(vfs->data_path, data_path, sizeof(vfs->data_path) - 1);
    scan_for_zips(vfs);
    vfs->initialized = true;
    printf("VFS: data path = %s, found %d ZIP archive(s)\n",
           vfs->data_path, vfs->num_zips);
    return true;
}

void vfs_free(DataVFS *vfs) {
    memset(vfs, 0, sizeof(*vfs));
}

uint8_t *vfs_read_file(const DataVFS *vfs, const char *rel_path, size_t *out_size) {
    if (!vfs || !vfs->initialized || !rel_path) return NULL;

    // Try loose file on disk first
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", vfs->data_path, rel_path);
    FILE *f = fopen(full_path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint8_t *data = malloc(size);
        if (data) {
            if (fread(data, 1, size, f) == (size_t)size) {
                fclose(f);
                *out_size = size;
                return data;
            }
            free(data);
        }
        fclose(f);
    }

    // Try each ZIP archive
    for (int i = 0; i < vfs->num_zips; i++) {
        uint8_t *data = zip_extract(vfs->zip_paths[i], rel_path, out_size);
        if (data) return data;
    }

    return NULL;
}

static uint8_t *read_regular_file(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long length = ftell(f);
    if (length < 0 || (uint64_t)length > ZIP_MAX_ENTRY_SIZE || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f); return NULL;
    }
    uint8_t *data = malloc((size_t)length);
    if (!data || fread(data, 1, (size_t)length, f) != (size_t)length) {
        free(data); fclose(f); return NULL;
    }
    fclose(f);
    if (out_size) *out_size = (size_t)length;
    return data;
}

#ifndef _WIN32
static uint8_t *find_hash_in_directory(const char *path, const char expected_sha256[65],
                                       size_t *out_size, int depth) {
    if (depth > 32) return NULL;
    DIR *dir = opendir(path);
    if (!dir) return NULL;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        char child[1024];
        if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) >= (int)sizeof(child)) continue;
        struct stat st;
        if (stat(child, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            uint8_t *found = find_hash_in_directory(child, expected_sha256, out_size, depth + 1);
            if (found) { closedir(dir); return found; }
        } else if (S_ISREG(st.st_mode) && (uint64_t)st.st_size <= ZIP_MAX_ENTRY_SIZE) {
            size_t size = 0; uint8_t *data = read_regular_file(child, &size);
            if (!data) continue;
            uint8_t digest[32]; sha256_digest(data, size, digest);
            if (sha256_matches_hex(digest, expected_sha256)) {
                closedir(dir); if (out_size) *out_size = size; return data;
            }
            free(data);
        }
    }
    closedir(dir);
    return NULL;
}
#else
static uint8_t *find_hash_in_directory(const char *path, const char expected_sha256[65],
                                       size_t *out_size, int depth) {
    if (depth > 32) return NULL;
    char pattern[1024];
    if (snprintf(pattern, sizeof(pattern), "%s\\*", path) >= (int)sizeof(pattern)) return NULL;
    WIN32_FIND_DATAA entry;
    HANDLE handle = FindFirstFileA(pattern, &entry);
    if (handle == INVALID_HANDLE_VALUE) return NULL;
    do {
        if (!strcmp(entry.cFileName, ".") || !strcmp(entry.cFileName, "..")) continue;
        char child[1024];
        if (snprintf(child, sizeof(child), "%s\\%s", path, entry.cFileName) >= (int)sizeof(child)) continue;
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            uint8_t *found = find_hash_in_directory(child, expected_sha256, out_size, depth + 1);
            if (found) { FindClose(handle); return found; }
        } else {
            uint64_t size_on_disk = ((uint64_t)entry.nFileSizeHigh << 32) | entry.nFileSizeLow;
            if (size_on_disk > ZIP_MAX_ENTRY_SIZE) continue;
            size_t size = 0; uint8_t *data = read_regular_file(child, &size);
            if (!data) continue;
            uint8_t digest[32]; sha256_digest(data, size, digest);
            if (sha256_matches_hex(digest, expected_sha256)) {
                FindClose(handle); if (out_size) *out_size = size; return data;
            }
            free(data);
        }
    } while (FindNextFileA(handle, &entry));
    FindClose(handle);
    return NULL;
}
#endif

uint8_t *vfs_find_sha256(const DataVFS *vfs, const char expected_sha256[65], size_t *out_size) {
    if (!vfs || !vfs->initialized || !expected_sha256) return NULL;
    uint8_t *loose = find_hash_in_directory(vfs->data_path, expected_sha256, out_size, 0);
    if (loose) return loose;
    for (int i = 0; i < vfs->num_zips; i++) {
        uint8_t *data = zip_find_sha256(vfs->zip_paths[i], expected_sha256, out_size);
        if (data) return data;
    }
    return NULL;
}

bool vfs_file_exists(const DataVFS *vfs, const char *rel_path) {
    size_t size;
    uint8_t *data = vfs_read_file(vfs, rel_path, &size);
    if (data) {
        free(data);
        return true;
    }
    return false;
}
