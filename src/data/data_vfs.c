#include "data_vfs.h"
#include "amiga_ofs.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#ifdef _WIN32
#include <windows.h>
#include <sys/stat.h>
#define stat _stat64
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
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
#define ZIP_MAX_NESTING 3

static int compare_zip_paths(const void *left, const void *right);
static void vfs_cache_signature(const DataVFS *vfs, char out[65]);
static bool cache_source_signature(const char *path, char out[65]);

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool valid_sha256_text(const char *hash) {
    if (!hash || strlen(hash) != 64) return false;
    for (size_t i = 0; i < 64; ++i) {
        char c = hash[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F'))) return false;
    }
    return true;
}

static bool safe_relative_path(const char *path) {
    if (!path || !path[0] || path[0] == '/' || path[0] == '\\' ||
        strchr(path, ':')) return false;
    const char *segment = path;
    for (const char *p = path;; p++) {
        if (*p != '/' && *p != '\\' && *p != '\0') continue;
        size_t length = (size_t)(p - segment);
        if (length == 2 && segment[0] == '.' && segment[1] == '.') return false;
        if (*p == '\0') break;
        segment = p + 1;
    }
    return true;
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

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long zip_size = ftell(f);
    if (zip_size < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }

    if (zip_size < 22) { fclose(f); return NULL; }

    // Find End of Central Directory (scan backwards)
    long search_start = zip_size - 22;
    if (search_start > zip_size - 65557) search_start = zip_size - 65557;
    if (search_start < 0) search_start = 0;

    uint8_t eocd_buf[65557];
    long search_len = zip_size - search_start;
    if (fseek(f, search_start, SEEK_SET) != 0) { fclose(f); return NULL; }
    if (fread(eocd_buf, 1, (size_t)search_len, f) != (size_t)search_len) {
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

    if (fseek(f, eocd_offset, SEEK_SET) != 0) { fclose(f); return NULL; }
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
    if (fseek(f, cd_offset, SEEK_SET) != 0) { fclose(f); return NULL; }
    uint8_t cd_hdr[46];

    for (uint16_t i = 0; i < num_entries; i++) {
        if (fread(cd_hdr, 1, 46, f) != 46) break;
        if (read_u32(cd_hdr) != ZIP_CENTRAL_SIG) break;

        uint16_t method = read_u16(cd_hdr + 10);
        uint32_t expected_crc = read_u32(cd_hdr + 16);
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
        if (fseek(f, extra_len + comment_len, SEEK_CUR) != 0) break;

        if (name[read_len - 1] == '/') continue; // directory

        if (!path_matches(name, read_len, rel_path)) continue;

        if (method == ZIP_METHOD_STORE && comp_size != uncomp_size) break;

        // Found match — read from local file header
        if ((uint64_t)local_offset + 30 > (uint64_t)zip_size) break;
        if (fseek(f, local_offset, SEEK_SET) != 0) break;
        uint8_t local_hdr[30];
        if (fread(local_hdr, 1, 30, f) != 30) break;
        if (read_u32(local_hdr) != ZIP_LOCAL_SIG) break;

        uint16_t local_name_len = read_u16(local_hdr + 26);
        uint16_t local_extra_len = read_u16(local_hdr + 28);
        long data_offset = ftell(f);
        if (data_offset < 0 || (uint64_t)data_offset + local_name_len + local_extra_len + comp_size >
            (uint64_t)zip_size) break;
        if (fseek(f, local_name_len + local_extra_len, SEEK_CUR) != 0) break;

        uint8_t *comp_data = malloc(comp_size ? comp_size : 1U);
        if (!comp_data) break;
        if (fread(comp_data, 1, comp_size, f) != comp_size) {
            free(comp_data);
            break;
        }
        fclose(f);

        if (method == ZIP_METHOD_STORE) {
            if (crc32(0L, comp_data, comp_size) != expected_crc) {
                free(comp_data);
                return NULL;
            }
            *out_size = comp_size;
            return comp_data;
        }

        if (method == ZIP_METHOD_DEFLATE) {
            /* Keep an addressable allocation for a valid empty entry; some
             * malloc implementations are permitted to return NULL for
             * malloc(0). */
            uint8_t *out = malloc(uncomp_size ? uncomp_size : 1U);
            if (!out) { free(comp_data); return NULL; }

            z_stream strm = {0};
            strm.next_in = comp_data;
            strm.avail_in = comp_size;
            strm.next_out = out;
            /* zlib needs a non-zero output window even for a valid empty
             * deflate stream.  Keep the allocation addressable and verify
             * that the stream still produced exactly the declared size. */
            strm.avail_out = uncomp_size ? uncomp_size : 1U;

            if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) {
                free(comp_data);
                free(out);
                return NULL;
            }
            int ret = inflate(&strm, Z_FINISH);
            bool output_complete = ret == Z_STREAM_END &&
                                   strm.total_out == uncomp_size;
            inflateEnd(&strm);
            free(comp_data);

            if (!output_complete) {
                free(out);
                return NULL;
            }
            if (crc32(0L, out, uncomp_size) != expected_crc) {
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

static uint8_t *zip_extract_memory_entry(const uint8_t *archive, size_t archive_size,
                                         uint32_t local_offset, uint32_t comp_size,
                                         uint32_t uncomp_size, uint16_t method,
                                         uint32_t expected_crc,
                                         size_t *out_size) {
    if (!archive || local_offset > archive_size || archive_size - local_offset < 30U ||
        comp_size > ZIP_MAX_ENTRY_SIZE || uncomp_size > ZIP_MAX_ENTRY_SIZE ||
        read_u32(archive + local_offset) != ZIP_LOCAL_SIG) return NULL;
    const uint8_t *local = archive + local_offset;
    uint16_t name_len = read_u16(local + 26U);
    uint16_t extra_len = read_u16(local + 28U);
    size_t data_offset = (size_t)local_offset + 30U + name_len + extra_len;
    if (data_offset > archive_size || comp_size > archive_size - data_offset) return NULL;
    uint8_t *out = malloc(uncomp_size ? uncomp_size : 1U);
    if (!out) return NULL;
    if (method == ZIP_METHOD_STORE) {
        if (comp_size != uncomp_size) { free(out); return NULL; }
        if (uncomp_size) memcpy(out, archive + data_offset, uncomp_size);
    } else if (method == ZIP_METHOD_DEFLATE) {
        z_stream strm = {0};
        strm.next_in = (Bytef *)(archive + data_offset);
        strm.avail_in = comp_size;
        strm.next_out = out;
        strm.avail_out = uncomp_size ? uncomp_size : 1U;
        int init_result = inflateInit2(&strm, -MAX_WBITS);
        if (init_result != Z_OK) {
            free(out);
            return NULL;
        }
        int inflate_result = inflate(&strm, Z_FINISH);
        bool valid = inflate_result == Z_STREAM_END &&
                     strm.total_out == uncomp_size;
        inflateEnd(&strm);
        if (!valid) {
            free(out);
            return NULL;
        }
    } else {
        free(out);
        return NULL;
    }
    if (crc32(0L, out, uncomp_size) != expected_crc) {
        free(out);
        return NULL;
    }
    if (out_size) *out_size = uncomp_size;
    return out;
}

static uint8_t *zip_find_sha256_memory(const uint8_t *archive, size_t archive_size,
                                        const char expected_sha256[65],
                                        size_t *out_size, unsigned depth) {
    if (!archive || archive_size < 22U || depth > ZIP_MAX_NESTING) return NULL;
    size_t start = archive_size > 65557U ? archive_size - 65557U : 0U;
    size_t eocd = SIZE_MAX;
    for (size_t pos = archive_size - 22U + 1U; pos-- > start;) {
        if (read_u32(archive + pos) == ZIP_END_SIG) { eocd = pos; break; }
    }
    if (eocd == SIZE_MAX || archive_size - eocd < 22U) return NULL;
    const uint8_t *end = archive + eocd;
    uint16_t entries = read_u16(end + 10U);
    uint32_t cd_size = read_u32(end + 12U), cd_offset = read_u32(end + 16U);
    if (cd_offset > archive_size || cd_size > archive_size - cd_offset) return NULL;

    size_t pos = cd_offset;
    for (uint16_t i = 0; i < entries; ++i) {
        if (archive_size - pos < 46U || read_u32(archive + pos) != ZIP_CENTRAL_SIG) break;
        const uint8_t *hdr = archive + pos;
        uint16_t method = read_u16(hdr + 10U);
        uint32_t expected_crc = read_u32(hdr + 16U);
        uint32_t comp_size = read_u32(hdr + 20U), uncomp_size = read_u32(hdr + 24U);
        uint16_t name_len = read_u16(hdr + 28U), extra_len = read_u16(hdr + 30U);
        uint16_t comment_len = read_u16(hdr + 32U);
        uint32_t local_offset = read_u32(hdr + 42U);
        size_t record_size = 46U + name_len + extra_len + comment_len;
        if (!name_len || name_len > ZIP_MAX_ENTRY_NAME || record_size > archive_size - pos)
            break;
        const uint8_t *name = hdr + 46U;
        pos += record_size;
        if (name[name_len - 1U] == '/') continue;

        size_t size = 0;
        uint8_t *data = zip_extract_memory_entry(archive, archive_size, local_offset,
                                                  comp_size, uncomp_size, method,
                                                  expected_crc, &size);
        if (!data) continue;
        uint8_t digest[32];
        sha256_digest(data, size, digest);
        if (sha256_matches_hex(digest, expected_sha256)) {
            if (out_size) *out_size = size;
            return data;
        }
        if (depth < ZIP_MAX_NESTING && size >= 4U &&
            read_u32(data) == ZIP_LOCAL_SIG) {
            uint8_t *nested = zip_find_sha256_memory(data, size, expected_sha256,
                                                      out_size, depth + 1U);
            if (nested) { free(data); return nested; }
        }
        if (size == 901120U || size == 1802240U) {
            uint8_t *adf_file = amiga_ofs_find_file_sha256(data, size,
                                                            expected_sha256, out_size);
            if (adf_file) { free(data); return adf_file; }
        }
        free(data);
    }
    return NULL;
}

static uint8_t *zip_find_sha256(const char *zip_path, const char expected_sha256[65],
                                size_t *out_size) {
    if (!zip_path || !expected_sha256) return NULL;
    FILE *f = fopen(zip_path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long zip_size = ftell(f);
    if (zip_size < 22 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    long start = zip_size > 65557 ? zip_size - 65557 : 0;
    uint8_t tail[65557];
    long tail_len = zip_size - start;
    if (fseek(f, start, SEEK_SET) != 0) { fclose(f); return NULL; }
    if (fread(tail, 1, (size_t)tail_len, f) != (size_t)tail_len) { fclose(f); return NULL; }
    long eocd = -1;
    for (long i = tail_len - 22; i >= 0; i--)
        if (read_u32(tail + i) == ZIP_END_SIG) { eocd = start + i; break; }
    if (eocd < 0) { fclose(f); return NULL; }
    if (fseek(f, eocd, SEEK_SET) != 0) { fclose(f); return NULL; }
    uint8_t end[22];
    if (fread(end, 1, sizeof(end), f) != sizeof(end)) { fclose(f); return NULL; }
    uint16_t entries = read_u16(end + 10);
    uint32_t cd_size = read_u32(end + 12), cd_offset = read_u32(end + 16);
    if ((uint64_t)cd_offset + cd_size > (uint64_t)zip_size) { fclose(f); return NULL; }
    if (fseek(f, cd_offset, SEEK_SET) != 0) { fclose(f); return NULL; }
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
        /* Preservation archives commonly package an ADF or disk image in a
         * second ZIP.  Descend only by content and only when the extracted
         * entry itself has a ZIP signature; no archive or entry name is used
         * to identify game data. */
        if (size >= 4U && read_u32(data) == ZIP_LOCAL_SIG) {
            uint8_t *nested = zip_find_sha256_memory(data, size, expected_sha256,
                                                      out_size, 1U);
            if (nested) { free(data); fclose(f); return nested; }
        }
        /* Amiga ADF disk images (880KB DD or 1760KB HD) */
        if (size == 901120U || size == 1802240U) {
            uint8_t *adf_file = amiga_ofs_find_file_sha256(data, size,
                                                            expected_sha256, out_size);
            if (adf_file) { free(data); fclose(f); return adf_file; }
        }
        free(data);
    }
    fclose(f);
    return NULL;
}

static bool has_zip_extension(const char *path) {
    if (!path) return false;
    size_t len = strlen(path);
    if (len < 4) return false;
    const char *ext = path + len - 4;
    return (ext[0] == '.' &&
            (ext[1] == 'z' || ext[1] == 'Z') &&
            (ext[2] == 'i' || ext[2] == 'I') &&
            (ext[3] == 'p' || ext[3] == 'P'));
}

static void append_zip_path(DataVFS *vfs, const char *path) {
    if (!vfs || !path || vfs->num_zips >= VFS_MAX_ZIPS) return;
    if (snprintf(vfs->zip_paths[vfs->num_zips],
                 sizeof(vfs->zip_paths[vfs->num_zips]), "%s", path) >=
        (int)sizeof(vfs->zip_paths[vfs->num_zips])) return;
    vfs->num_zips++;
}

#ifdef _WIN32
static void scan_for_zips_recursive(DataVFS *vfs, const char *path, int depth) {
    if (!vfs || !path || depth > 32 || vfs->num_zips >= VFS_MAX_ZIPS) return;
    char pattern[1024];
    if (snprintf(pattern, sizeof(pattern), "%s\\*", path) >= (int)sizeof(pattern)) return;
    WIN32_FIND_DATAA entry;
    HANDLE handle = FindFirstFileA(pattern, &entry);
    if (handle == INVALID_HANDLE_VALUE) return;
    do {
        if (!strcmp(entry.cFileName, ".") || !strcmp(entry.cFileName, "..")) continue;
        /* The persistent verification cache may live below the selected data
         * root (for example when the user chooses HOME).  It is an
         * implementation detail, not game data, and must not make the source
         * signature change after every cache write. */
        if (!strcmp(entry.cFileName, ".cache")) continue;
        char child[1024];
        if (snprintf(child, sizeof(child), "%s\\%s", path, entry.cFileName) >=
            (int)sizeof(child)) continue;
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            scan_for_zips_recursive(vfs, child, depth + 1);
        else if (has_zip_extension(child))
            append_zip_path(vfs, child);
    } while (FindNextFileA(handle, &entry) && vfs->num_zips < VFS_MAX_ZIPS);
    FindClose(handle);
}
#else
static void scan_for_zips_recursive(DataVFS *vfs, const char *path, int depth) {
    if (!vfs || !path || depth > 32 || vfs->num_zips >= VFS_MAX_ZIPS) return;
    DIR *dir = opendir(path);
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && vfs->num_zips < VFS_MAX_ZIPS) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        if (!strcmp(entry->d_name, ".cache")) continue;
        char child[1024];
        if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) >=
            (int)sizeof(child)) continue;
        struct stat st;
        if (stat(child, &st) != 0) continue;
        if (S_ISDIR(st.st_mode))
            scan_for_zips_recursive(vfs, child, depth + 1);
        else if (S_ISREG(st.st_mode) && has_zip_extension(child))
            append_zip_path(vfs, child);
    }
    closedir(dir);
}
#endif

static void scan_for_zips(DataVFS *vfs) {
    if (!vfs) return;
    vfs->num_zips = 0;
    scan_for_zips_recursive(vfs, vfs->data_path, 0);
}

bool vfs_init(DataVFS *vfs, const char *data_path) {
    if (!vfs) return false;
    memset(vfs, 0, sizeof(*vfs));
    if (!data_path || !data_path[0]) return false;
    size_t data_path_len = strlen(data_path);
    if (data_path_len >= sizeof(vfs->data_path)) return false;
    memcpy(vfs->data_path, data_path, data_path_len + 1U);
    scan_for_zips(vfs);
    qsort(vfs->zip_paths, (size_t)vfs->num_zips, sizeof(vfs->zip_paths[0]),
          compare_zip_paths);
    vfs_cache_signature(vfs, vfs->cache_signature);
    vfs->cache_signature_valid = true;
    vfs->initialized = true;
    printf("VFS: data path = %s, found %d ZIP archive(s)\n",
           vfs->data_path, vfs->num_zips);
    return true;
}

void vfs_free(DataVFS *vfs) {
    if (!vfs) return;
    memset(vfs, 0, sizeof(*vfs));
}

uint8_t *vfs_read_file(const DataVFS *vfs, const char *rel_path, size_t *out_size) {
    if (!vfs || !vfs->initialized || !safe_relative_path(rel_path) || !out_size) return NULL;

    // Try loose file on disk first
    char full_path[1024];
    if (snprintf(full_path, sizeof(full_path), "%s/%s", vfs->data_path, rel_path)
            >= (int)sizeof(full_path)) return NULL;
    FILE *f = fopen(full_path, "rb");
    if (f) {
        if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
        long size = ftell(f);
        if (size < 0 || (uint64_t)size > ZIP_MAX_ENTRY_SIZE ||
            fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
        /* Keep an addressable allocation for an empty file.  A NULL return
         * from malloc(0) must not make an existing empty asset look missing. */
        uint8_t *data = malloc((size_t)size ? (size_t)size : 1U);
        if (data) {
            if (fread(data, 1, (size_t)size, f) == (size_t)size) {
                fclose(f);
                *out_size = (size_t)size;
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
    uint8_t *data = malloc((size_t)length ? (size_t)length : 1U);
    if (!data || fread(data, 1, (size_t)length, f) != (size_t)length) {
        free(data); fclose(f); return NULL;
    }
    fclose(f);
    if (out_size) *out_size = (size_t)length;
    return data;
}

/* Persist verified hash payloads. Hash only cheap filesystem metadata: this
 * invalidates the cache when a loose asset is replaced, without reading and
 * hashing every asset again. */
static void cache_meta_update(SHA256Context *ctx, const char *path,
                              const struct stat *st) {
#ifdef _WIN32
    /* _stat64 exposes second-resolution times on Windows.  Two archive
     * replacements in the same second could therefore reuse a stale cache
     * entry.  The file-information API provides the native 100 ns timestamp
     * together with the file identity, while still avoiding a content hash. */
    HANDLE handle = CreateFileA(path, FILE_READ_ATTRIBUTES,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE |
                                 FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
                                 FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (handle != INVALID_HANDLE_VALUE) {
        BY_HANDLE_FILE_INFORMATION info;
        if (GetFileInformationByHandle(handle, &info)) {
            ULARGE_INTEGER write_time;
            write_time.HighPart = info.ftLastWriteTime.dwHighDateTime;
            write_time.LowPart = info.ftLastWriteTime.dwLowDateTime;
            unsigned long long file_index =
                ((unsigned long long)info.nFileIndexHigh << 32) |
                info.nFileIndexLow;
            unsigned long long file_size =
                ((unsigned long long)info.nFileSizeHigh << 32) |
                info.nFileSizeLow;
            char line[1200];
            int n = snprintf(line, sizeof(line), "%s|%llu|%llu|%lu|%llu|%lu\n",
                             path, file_size,
                             (unsigned long long)write_time.QuadPart,
                             info.dwVolumeSerialNumber, file_index,
                             info.dwFileAttributes);
            CloseHandle(handle);
            if (n > 0 && n < (int)sizeof(line)) {
                sha256_update(ctx, (const uint8_t *)line, (size_t)n);
                return;
            }
        } else {
            CloseHandle(handle);
        }
    }
#endif
    char line[1200];
    long mtime_nsec = 0;
#if defined(__APPLE__)
    mtime_nsec = st->st_mtimespec.tv_nsec;
#elif !defined(_WIN32)
    mtime_nsec = st->st_mtim.tv_nsec;
#endif
    int n = snprintf(line, sizeof(line), "%s|%lld|%lld|%ld|%llu|%lld\n", path,
                     (long long)st->st_size, (long long)st->st_mtime,
                     mtime_nsec, (unsigned long long)st->st_ino,
                     (long long)st->st_ctime);
    if (n > 0) sha256_update(ctx, (const uint8_t *)line, (size_t)n);
}

static bool cache_source_signature(const char *path, char out[65]) {
    if (!path || !path[0] || !out) return false;
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) return false;
    SHA256Context ctx;
    uint8_t digest[32];
    sha256_init(&ctx);
    cache_meta_update(&ctx, path, &st);
    sha256_final(&ctx, digest);
    for (int i = 0; i < 32; ++i)
        snprintf(out + i * 2, 3, "%02x", digest[i]);
    out[64] = '\0';
    return true;
}

#ifdef _WIN32
typedef struct {
    char path[1024];
    struct stat st;
    bool is_directory;
} CacheTreeEntry;

static int compare_cache_tree_entries(const void *left, const void *right) {
    const CacheTreeEntry *a = (const CacheTreeEntry *)left;
    const CacheTreeEntry *b = (const CacheTreeEntry *)right;
    return strcmp(a->path, b->path);
}

static void cache_tree_metadata(SHA256Context *ctx, const char *path, int depth) {
    if (depth > 32) return;
    char pattern[1024];
    if (snprintf(pattern, sizeof(pattern), "%s\\*", path) >= (int)sizeof(pattern)) return;
    WIN32_FIND_DATAA entry;
    HANDLE handle = FindFirstFileA(pattern, &entry);
    if (handle == INVALID_HANDLE_VALUE) return;
    CacheTreeEntry *entries = NULL;
    size_t entry_count = 0;
    size_t entry_capacity = 0;
    do {
        if (!strcmp(entry.cFileName, ".") || !strcmp(entry.cFileName, "..")) continue;
        char child[1024];
        if (snprintf(child, sizeof(child), "%s\\%s", path, entry.cFileName) >=
            (int)sizeof(child)) continue;
        struct stat st;
        if (stat(child, &st) != 0) continue;
        if (entry_count == entry_capacity) {
            size_t new_capacity = entry_capacity ? entry_capacity * 2U : 32U;
            CacheTreeEntry *grown = realloc(entries,
                                            new_capacity * sizeof(*entries));
            if (!grown) {
                free(entries);
                FindClose(handle);
                return;
            }
            entries = grown;
            entry_capacity = new_capacity;
        }
        snprintf(entries[entry_count].path, sizeof(entries[entry_count].path), "%s", child);
        entries[entry_count].st = st;
        entries[entry_count].is_directory =
            (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        entry_count++;
    } while (FindNextFileA(handle, &entry));
    FindClose(handle);

    qsort(entries, entry_count, sizeof(*entries), compare_cache_tree_entries);
    for (size_t i = 0; i < entry_count; ++i) {
        cache_meta_update(ctx, entries[i].path, &entries[i].st);
        if (entries[i].is_directory)
            cache_tree_metadata(ctx, entries[i].path, depth + 1);
    }
    free(entries);
}
#else
static void cache_tree_metadata(SHA256Context *ctx, const char *path, int depth) {
    if (depth > 32) return;
    struct dirent **entries = NULL;
    int entry_count = scandir(path, &entries, NULL, alphasort);
    if (entry_count < 0) return;
    for (int i = 0; i < entry_count; ++i) {
        struct dirent *entry = entries[i];
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            free(entry);
            continue;
        }
        /* Keep the internal persistent cache out of source metadata when the
         * selected data root contains the user's cache directory. */
        if (!strcmp(entry->d_name, ".cache")) {
            free(entry);
            continue;
        }
        char child[1024];
        if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) >=
            (int)sizeof(child)) {
            free(entry);
            continue;
        }
        struct stat st;
        if (stat(child, &st) != 0) {
            free(entry);
            continue;
        }
        cache_meta_update(ctx, child, &st);
        if (S_ISDIR(st.st_mode)) cache_tree_metadata(ctx, child, depth + 1);
        free(entry);
    }
    free(entries);
}
#endif

static int compare_zip_paths(const void *left, const void *right) {
    return strcmp((const char *)left, (const char *)right);
}

static void vfs_cache_signature(const DataVFS *vfs, char out[65]) {
    SHA256Context ctx;
    uint8_t digest[32];
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t *)vfs->data_path,
                  strlen(vfs->data_path) + 1);
    cache_tree_metadata(&ctx, vfs->data_path, 0);
    for (int i = 0; i < vfs->num_zips; ++i) {
        struct stat st;
        if (stat(vfs->zip_paths[i], &st) == 0)
            cache_meta_update(&ctx, vfs->zip_paths[i], &st);
    }
    sha256_final(&ctx, digest);
    for (int i = 0; i < 32; ++i) snprintf(out + i * 2, 3, "%02x", digest[i]);
    out[64] = '\0';
}

static const char *vfs_cache_home(void) {
    const char *home = getenv("HOME");
    /* Windows normally exposes the user directory as USERPROFILE rather
     * than HOME.  Without this fallback the scanner cache silently becomes
     * process-local on a standard Windows installation. */
    if ((!home || !home[0])) home = getenv("USERPROFILE");
    return home && home[0] ? home : NULL;
}

static bool replace_cache_file(const char *temporary_path, const char *path) {
#ifdef _WIN32
    return MoveFileExA(temporary_path, path,
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(temporary_path, path) == 0;
#endif
}

static unsigned long cache_process_id(void) {
#ifdef _WIN32
    return (unsigned long)GetCurrentProcessId();
#else
    return (unsigned long)getpid();
#endif
}

static uint8_t *vfs_cache_read(const char hash[65], const char signature[65],
                               size_t *out_size) {
    const char *home = vfs_cache_home();
    if (!home || !valid_sha256_text(hash)) return NULL;
    char meta[1200], data_path[1200], stored[4096];
    snprintf(meta, sizeof(meta), "%s/.cache/opencaptive/%s.meta", home, hash);
    snprintf(data_path, sizeof(data_path), "%s/.cache/opencaptive/%s.bin", home, hash);
    FILE *mf = fopen(meta, "rb");
    if (!mf) return NULL;
    size_t n = fread(stored, 1, sizeof(stored) - 1, mf);
    fclose(mf);
    stored[n] = '\0';
    /* v2 metadata stores the source path and its filesystem fingerprint.  Old
     * one-line entries are intentionally ignored so a loose-file cache entry
     * can never bypass source validation after an upgrade. */
    if (strncmp(stored, "v2\n", 3) != 0) return NULL;
    char *signature_line = stored + 3;
    char *source_path = strchr(signature_line, '\n');
    if (!source_path) return NULL;
    *source_path++ = '\0';
    if (strcmp(signature_line, signature) != 0) return NULL;
    char *source_signature = strchr(source_path, '\n');
    if (!source_signature) return NULL;
    *source_signature++ = '\0';
    char *end = strchr(source_signature, '\n');
    if (end) *end = '\0';
    /* Every v2 entry must identify the exact loose file or archive that
     * supplied it.  Older entries for archive data had an empty source path;
     * reject those instead of returning a payload without a source-level
     * freshness check. */
    if (!source_path[0]) return NULL;
    char current_source_signature[65];
    if (!cache_source_signature(source_path, current_source_signature) ||
        strcmp(source_signature, current_source_signature) != 0)
        return NULL;
    size_t cached_size = 0;
    uint8_t *cached = read_regular_file(data_path, &cached_size);
    if (!cached) return NULL;
    uint8_t digest[32];
    sha256_digest(cached, cached_size, digest);
    if (!sha256_matches_hex(digest, hash)) {
        free(cached);
        return NULL;
    }
    if (out_size) *out_size = cached_size;
    return cached;
}

static void vfs_cache_write(const char hash[65], const uint8_t *data, size_t size,
                            const char signature[65], const char *source_path) {
    const char *home = vfs_cache_home();
    if (!home || !valid_sha256_text(hash) || !data || size > ZIP_MAX_ENTRY_SIZE) return;
    char base[1024], meta[1200], data_path[1200];
    snprintf(base, sizeof(base), "%s/.cache/opencaptive", home);
    char parent[1024];
    snprintf(parent, sizeof(parent), "%s/.cache", home);
    (void)mkdir(parent, 0755);
    (void)mkdir(base, 0755);
    snprintf(meta, sizeof(meta), "%s/%s.meta", base, hash);
    snprintf(data_path, sizeof(data_path), "%s/%s.bin", base, hash);
    char data_tmp[1250], meta_tmp[1250];
    unsigned long process_id = cache_process_id();
    if (snprintf(data_tmp, sizeof(data_tmp), "%s.tmp.%lu", data_path,
                 process_id) >=
            (int)sizeof(data_tmp) ||
        snprintf(meta_tmp, sizeof(meta_tmp), "%s.tmp.%lu", meta,
                 process_id) >=
            (int)sizeof(meta_tmp)) return;

    FILE *df = fopen(data_tmp, "wb");
    if (!df) return;
    bool data_write_ok = fwrite(data, 1, size, df) == size;
    int data_close_result = fclose(df);
    if (!data_write_ok || data_close_result != 0) {
        remove(data_tmp);
        return;
    }
    if (!replace_cache_file(data_tmp, data_path)) {
        remove(data_tmp);
        return;
    }

    char source_signature[65] = {0};
    if (source_path && (strchr(source_path, '\n') || strchr(source_path, '\r') ||
                        !cache_source_signature(source_path, source_signature))) {
        /* Never persist a loose-file payload without the source fingerprint;
         * otherwise an unusual path or a race during the write would turn
         * this back into an unvalidated cache entry. */
        remove(meta_tmp);
        return;
    }
    char metadata[2400];
    int metadata_length = snprintf(metadata, sizeof(metadata), "v2\n%s\n%s\n%s\n",
                                   signature, source_path ? source_path : "",
                                   source_path ? source_signature : "");
    if (metadata_length < 0 || metadata_length >= (int)sizeof(metadata)) {
        remove(meta_tmp);
        return;
    }
    FILE *mf = fopen(meta_tmp, "wb");
    if (!mf) return;
    bool meta_write_ok = fwrite(metadata, 1, (size_t)metadata_length, mf) ==
                         (size_t)metadata_length;
    int meta_close_result = fclose(mf);
    if (!meta_write_ok || meta_close_result != 0) {
        remove(meta_tmp);
        return;
    }
    if (!replace_cache_file(meta_tmp, meta)) remove(meta_tmp);
}

#ifndef _WIN32
static uint8_t *find_hash_in_directory(const char *path, const char expected_sha256[65],
                                       size_t *out_size, int depth,
                                       char *source_path, size_t source_path_size) {
    if (depth > 32) return NULL;
    DIR *dir = opendir(path);
    if (!dir) return NULL;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        if (!strcmp(entry->d_name, ".cache")) continue;
        char child[1024];
        if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) >= (int)sizeof(child)) continue;
        struct stat st;
        if (stat(child, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            uint8_t *found = find_hash_in_directory(child, expected_sha256, out_size,
                                                    depth + 1, source_path,
                                                    source_path_size);
            if (found) { closedir(dir); return found; }
        } else if (S_ISREG(st.st_mode) && (uint64_t)st.st_size <= ZIP_MAX_ENTRY_SIZE) {
            size_t size = 0; uint8_t *data = read_regular_file(child, &size);
            if (!data) continue;
            uint8_t digest[32]; sha256_digest(data, size, digest);
            if (sha256_matches_hex(digest, expected_sha256)) {
                if (source_path && source_path_size)
                    snprintf(source_path, source_path_size, "%s", child);
                closedir(dir); if (out_size) *out_size = size; return data;
            }
            if (size == 901120U || size == 1802240U) {
                uint8_t *adf_file = amiga_ofs_find_file_sha256(data, size,
                                                                expected_sha256, out_size);
                if (adf_file) {
                    if (source_path && source_path_size)
                        snprintf(source_path, source_path_size, "%s", child);
                    free(data); closedir(dir); return adf_file;
                }
            }
            free(data);
        }
    }
    closedir(dir);
    return NULL;
}
#else
static uint8_t *find_hash_in_directory(const char *path, const char expected_sha256[65],
                                       size_t *out_size, int depth,
                                       char *source_path, size_t source_path_size) {
    if (depth > 32) return NULL;
    char pattern[1024];
    if (snprintf(pattern, sizeof(pattern), "%s\\*", path) >= (int)sizeof(pattern)) return NULL;
    WIN32_FIND_DATAA entry;
    HANDLE handle = FindFirstFileA(pattern, &entry);
    if (handle == INVALID_HANDLE_VALUE) return NULL;
    do {
        if (!strcmp(entry.cFileName, ".") || !strcmp(entry.cFileName, "..")) continue;
        if (!strcmp(entry.cFileName, ".cache")) continue;
        char child[1024];
        if (snprintf(child, sizeof(child), "%s\\%s", path, entry.cFileName) >= (int)sizeof(child)) continue;
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            uint8_t *found = find_hash_in_directory(child, expected_sha256, out_size,
                                                    depth + 1, source_path,
                                                    source_path_size);
            if (found) { FindClose(handle); return found; }
        } else {
            uint64_t size_on_disk = ((uint64_t)entry.nFileSizeHigh << 32) | entry.nFileSizeLow;
            if (size_on_disk > ZIP_MAX_ENTRY_SIZE) continue;
            size_t size = 0; uint8_t *data = read_regular_file(child, &size);
            if (!data) continue;
            uint8_t digest[32]; sha256_digest(data, size, digest);
            if (sha256_matches_hex(digest, expected_sha256)) {
                if (source_path && source_path_size)
                    snprintf(source_path, source_path_size, "%s", child);
                FindClose(handle); if (out_size) *out_size = size; return data;
            }
            if (size == 901120U || size == 1802240U) {
                uint8_t *adf_file = amiga_ofs_find_file_sha256(data, size,
                                                                expected_sha256, out_size);
                if (adf_file) {
                    if (source_path && source_path_size)
                        snprintf(source_path, source_path_size, "%s", child);
                    free(data); FindClose(handle); return adf_file;
                }
            }
            free(data);
        }
    } while (FindNextFileA(handle, &entry));
    FindClose(handle);
    return NULL;
}
#endif

uint8_t *vfs_find_sha256(const DataVFS *vfs, const char expected_sha256[65], size_t *out_size) {
    if (!vfs || !vfs->initialized || !valid_sha256_text(expected_sha256)) return NULL;
    char signature[65];
    /* The VFS signature is fixed when the instance is opened.  Cache entries
     * carry their own source fingerprint, so probing the complete metadata
     * tree for every hash would only make a multi-file startup scan walk the
     * entire data root repeatedly. */
    if (!vfs->cache_signature_valid) {
        vfs_cache_signature(vfs, signature);
    } else {
        memcpy(signature, vfs->cache_signature, sizeof(signature));
    }
    uint8_t *cached = vfs_cache_read(expected_sha256, signature, out_size);
    if (cached) return cached;
    size_t found_size = 0;
    char source_path[1024] = {0};
    uint8_t *loose = find_hash_in_directory(vfs->data_path, expected_sha256,
                                             &found_size, 0, source_path,
                                             sizeof(source_path));
    if (loose) {
        if (out_size) *out_size = found_size;
        vfs_cache_write(expected_sha256, loose, found_size, signature,
                        source_path[0] ? source_path : NULL);
        return loose;
    }
    for (int i = 0; i < vfs->num_zips; i++) {
        found_size = 0;
        uint8_t *data = zip_find_sha256(vfs->zip_paths[i], expected_sha256, &found_size);
        if (data) {
            if (out_size) *out_size = found_size;
            vfs_cache_write(expected_sha256, data, found_size, signature,
                            vfs->zip_paths[i]);
            return data;
        }
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
