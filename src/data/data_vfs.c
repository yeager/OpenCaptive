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

        uint8_t *comp_data = malloc(comp_size);
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
            strm.avail_out = uncomp_size;

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
    if (method == ZIP_METHOD_STORE) {
        if (comp_size != uncomp_size) { free(out); return NULL; }
        if (uncomp_size) memcpy(out, archive + data_offset, uncomp_size);
    } else if (method == ZIP_METHOD_DEFLATE) {
        z_stream strm = {0};
        strm.next_in = (Bytef *)(archive + data_offset);
        strm.avail_in = comp_size;
        strm.next_out = out;
        strm.avail_out = uncomp_size;
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
    if (!vfs) return false;
    memset(vfs, 0, sizeof(*vfs));
    if (!data_path || !data_path[0]) return false;
    strncpy(vfs->data_path, data_path, sizeof(vfs->data_path) - 1);
    vfs->data_path[sizeof(vfs->data_path) - 1] = '\0';
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

#ifdef _WIN32
static void cache_tree_metadata(SHA256Context *ctx, const char *path, int depth) {
    if (depth > 32) return;
    char pattern[1024];
    if (snprintf(pattern, sizeof(pattern), "%s\\*", path) >= (int)sizeof(pattern)) return;
    WIN32_FIND_DATAA entry;
    HANDLE handle = FindFirstFileA(pattern, &entry);
    if (handle == INVALID_HANDLE_VALUE) return;
    do {
        if (!strcmp(entry.cFileName, ".") || !strcmp(entry.cFileName, "..")) continue;
        char child[1024];
        if (snprintf(child, sizeof(child), "%s\\%s", path, entry.cFileName) >=
            (int)sizeof(child)) continue;
        struct stat st;
        if (stat(child, &st) != 0) continue;
        cache_meta_update(ctx, child, &st);
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            cache_tree_metadata(ctx, child, depth + 1);
    } while (FindNextFileA(handle, &entry));
    FindClose(handle);
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
        if (stat(vfs->zip_paths[i], &st) == 0) cache_meta_update(&ctx, vfs->zip_paths[i], &st);
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

static uint8_t *vfs_cache_read(const DataVFS *vfs, const char hash[65], size_t *out_size) {
    const char *home = vfs_cache_home();
    if (!home || !valid_sha256_text(hash)) return NULL;
    char meta[1200], data_path[1200], signature[65], stored[65];
    snprintf(meta, sizeof(meta), "%s/.cache/opencaptive/%s.meta", home, hash);
    snprintf(data_path, sizeof(data_path), "%s/.cache/opencaptive/%s.bin", home, hash);
    FILE *mf = fopen(meta, "rb");
    if (!mf) return NULL;
    if (vfs->cache_signature_valid)
        memcpy(signature, vfs->cache_signature, sizeof(vfs->cache_signature));
    else
        vfs_cache_signature(vfs, signature);
    size_t n = fread(stored, 1, sizeof(stored) - 1, mf);
    fclose(mf);
    stored[n] = '\0';
    if (n != strlen(signature) || memcmp(stored, signature, n) != 0) return NULL;
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

static void vfs_cache_write(const DataVFS *vfs, const char hash[65],
                            const uint8_t *data, size_t size) {
    const char *home = vfs_cache_home();
    if (!home || !valid_sha256_text(hash) || !data || size > ZIP_MAX_ENTRY_SIZE) return;
    char base[1024], meta[1200], data_path[1200], signature[65];
    snprintf(base, sizeof(base), "%s/.cache/opencaptive", home);
    char parent[1024];
    snprintf(parent, sizeof(parent), "%s/.cache", home);
    (void)mkdir(parent, 0755);
    (void)mkdir(base, 0755);
    snprintf(meta, sizeof(meta), "%s/%s.meta", base, hash);
    snprintf(data_path, sizeof(data_path), "%s/%s.bin", base, hash);
    if (vfs->cache_signature_valid)
        memcpy(signature, vfs->cache_signature, sizeof(vfs->cache_signature));
    else
        vfs_cache_signature(vfs, signature);
    FILE *df = fopen(data_path, "wb");
    if (!df) return;
    bool data_write_ok = fwrite(data, 1, size, df) == size;
    int data_close_result = fclose(df);
    if (!data_write_ok || data_close_result != 0) return;
    FILE *mf = fopen(meta, "wb");
    if (!mf) return;
    (void)fwrite(signature, 1, strlen(signature), mf);
    fclose(mf);
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
            if (size == 901120U || size == 1802240U) {
                uint8_t *adf_file = amiga_ofs_find_file_sha256(data, size,
                                                                expected_sha256, out_size);
                if (adf_file) { free(data); closedir(dir); return adf_file; }
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
            if (size == 901120U || size == 1802240U) {
                uint8_t *adf_file = amiga_ofs_find_file_sha256(data, size,
                                                                expected_sha256, out_size);
                if (adf_file) { free(data); FindClose(handle); return adf_file; }
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
    uint8_t *cached = vfs_cache_read(vfs, expected_sha256, out_size);
    if (cached) return cached;
    size_t found_size = 0;
    uint8_t *loose = find_hash_in_directory(vfs->data_path, expected_sha256, &found_size, 0);
    if (loose) {
        if (out_size) *out_size = found_size;
        vfs_cache_write(vfs, expected_sha256, loose, found_size);
        return loose;
    }
    for (int i = 0; i < vfs->num_zips; i++) {
        found_size = 0;
        uint8_t *data = zip_find_sha256(vfs->zip_paths[i], expected_sha256, &found_size);
        if (data) {
            if (out_size) *out_size = found_size;
            vfs_cache_write(vfs, expected_sha256, data, found_size);
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
