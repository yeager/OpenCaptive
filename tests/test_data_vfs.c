#include "data_vfs.h"
#include "sha256.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#define TEST_MKDIR(path) _mkdir(path)
#define TEST_RMDIR(path) _rmdir(path)
#define TEST_CHDIR(path) _chdir(path)
#define TEST_GETCWD(buf, size) _getcwd((buf), (int)(size))
#else
#include <sys/stat.h>
#include <unistd.h>
#define TEST_MKDIR(path) mkdir(path, 0755)
#define TEST_RMDIR(path) rmdir(path)
#define TEST_CHDIR(path) chdir(path)
#define TEST_GETCWD(buf, size) getcwd((buf), (size))
#endif

static bool create_test_root(char *path, size_t path_size) {
#ifdef _WIN32
    for (unsigned attempt = 0; attempt < 100U; ++attempt) {
        if (snprintf(path, path_size, "test-vfs-root-%lu-%u",
                     (unsigned long)_getpid(), attempt) >= (int)path_size)
            return false;
        if (TEST_MKDIR(path) == 0) return true;
    }
    return false;
#else
    if (snprintf(path, path_size, "test-vfs-root-XXXXXX") >= (int)path_size)
        return false;
    return mkdtemp(path) != NULL;
#endif
}

static void put16(FILE *f, unsigned value) {
    fputc(value & 0xff, f);
    fputc((value >> 8) & 0xff, f);
}

static void put32(FILE *f, unsigned value) {
    put16(f, value & 0xffff);
    put16(f, value >> 16);
}

static void write_stored_zip(const char *path, const char *name,
                             const unsigned char *data, unsigned data_size) {
    FILE *f = fopen(path, "wb");
    assert(f);
    unsigned name_size = (unsigned)strlen(name);
    uint32_t crc = crc32(0L, data, data_size);

    put32(f, 0x04034b50); put16(f, 20); put16(f, 0); put16(f, 0);
    put16(f, 0); put16(f, 0); put32(f, crc); put32(f, data_size); put32(f, data_size);
    put16(f, name_size); put16(f, 0);
    fwrite(name, 1, name_size, f);
    fwrite(data, 1, data_size, f);

    unsigned cd_offset = 30 + name_size + data_size;
    put32(f, 0x02014b50); put16(f, 20); put16(f, 20); put16(f, 0); put16(f, 0);
    put16(f, 0); put16(f, 0); put32(f, crc); put32(f, data_size); put32(f, data_size);
    put16(f, name_size); put16(f, 0); put16(f, 0); put16(f, 0); put16(f, 0);
    put32(f, 0); put32(f, 0);
    fwrite(name, 1, name_size, f);

    unsigned cd_size = 46 + name_size;
    put32(f, 0x06054b50); put16(f, 0); put16(f, 0); put16(f, 1); put16(f, 1);
    put32(f, cd_size); put32(f, cd_offset); put16(f, 0);
    assert(fclose(f) == 0);
}

static void write_empty_deflated_zip(const char *path, const char *name) {
    /* Raw DEFLATE representation of an empty stream. */
    const unsigned char compressed[] = {0x03, 0x00};
    FILE *f = fopen(path, "wb");
    assert(f);
    unsigned name_size = (unsigned)strlen(name);
    const unsigned crc = 0;

    put32(f, 0x04034b50); put16(f, 20); put16(f, 0); put16(f, 8);
    put16(f, 0); put16(f, 0); put32(f, crc); put32(f, sizeof(compressed));
    put32(f, 0); put16(f, name_size); put16(f, 0);
    fwrite(name, 1, name_size, f);
    fwrite(compressed, 1, sizeof(compressed), f);

    unsigned cd_offset = 30 + name_size + (unsigned)sizeof(compressed);
    put32(f, 0x02014b50); put16(f, 20); put16(f, 20); put16(f, 0);
    put16(f, 8); put16(f, 0); put16(f, 0); put32(f, crc);
    put32(f, sizeof(compressed));
    put32(f, 0); put16(f, name_size); put16(f, 0); put16(f, 0); put16(f, 0);
    put16(f, 0); put32(f, 0); put32(f, 0);
    fwrite(name, 1, name_size, f);

    unsigned cd_size = 46 + name_size;
    put32(f, 0x06054b50); put16(f, 0); put16(f, 0); put16(f, 1); put16(f, 1);
    put32(f, cd_size); put32(f, cd_offset); put16(f, 0);
    assert(fclose(f) == 0);
}

static void test_reads_prefixed_case_insensitive_zip_entry(void) {
    const unsigned char payload[] = { 1, 2, 3, 4 };
    write_stored_zip("test-vfs-assets.zip", "captIve/CAPICS/WALLA.PL5", payload, sizeof(payload));

    DataVFS vfs;
    assert(vfs_init(&vfs, "."));
    size_t size = 0;
    unsigned char *result = vfs_read_file(&vfs, "capics/walla.pl5", &size);
    assert(result);
    assert(size == sizeof(payload));
    assert(memcmp(result, payload, sizeof(payload)) == 0);
    free(result);
    uint8_t digest[32];
    sha256_digest(payload, sizeof(payload), digest);
    result = vfs_find_sha256(&vfs,
        "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a", &size);
    assert(result && size == sizeof(payload));
    free(result);
    /* The second lookup must also be served correctly from the persistent
       verification cache when the archive metadata is unchanged. */
    result = vfs_find_sha256(&vfs,
        "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a", &size);
    assert(result && size == sizeof(payload) && memcmp(result, payload, size) == 0);
    free(result);
    vfs_free(&vfs);
    /* A fresh VFS instance must compute the same deterministic signature and
       reuse the cache rather than rescanning the archive. */
    DataVFS second;
    assert(vfs_init(&second, "."));
    result = vfs_find_sha256(&second,
        "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a", &size);
    assert(result && size == sizeof(payload) && memcmp(result, payload, size) == 0);
    free(result);
    vfs_free(&second);
    assert(remove("test-vfs-assets.zip") == 0);
}

static void test_reads_zip_in_nested_data_directory(void) {
    const unsigned char payload[] = { 0x31, 0x41, 0x59, 0x26 };
    assert(TEST_MKDIR("test-vfs-nested") == 0);
    write_stored_zip("test-vfs-nested/nested-assets.zip", "nested.bin",
                     payload, sizeof(payload));

    DataVFS vfs;
    assert(vfs_init(&vfs, "."));
    size_t size = 0;
    uint8_t *result = vfs_read_file(&vfs, "nested.bin", &size);
    assert(result && size == sizeof(payload) &&
           memcmp(result, payload, sizeof(payload)) == 0);
    free(result);
    vfs_free(&vfs);
    assert(remove("test-vfs-nested/nested-assets.zip") == 0);
    assert(TEST_RMDIR("test-vfs-nested") == 0);
}

static void test_replacing_zip_invalidates_live_cache(void) {
    const unsigned char original[] = { 1, 2, 3, 4 };
    /* A size change makes the source fingerprint change on every supported
     * filesystem.  Same-size rewrites can retain identical observable
     * metadata on Windows when they happen within one filesystem timestamp
     * tick, which would make this cache test timing-dependent. */
    const unsigned char replacement[] = { 5, 6, 7, 8, 9 };
    const char *original_hash =
        "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a";
    const char *replacement_hash =
        "761ca8fd7dd51248e00a7dc1c746bbde94e51cb06aa67194843c495a863e0106";

    write_stored_zip("test-vfs-replace.zip", "payload.bin",
                     original, sizeof(original));
    DataVFS vfs;
    assert(vfs_init(&vfs, "."));
    size_t size = 0;
    uint8_t *result = vfs_find_sha256(&vfs, original_hash, &size);
    assert(result && size == sizeof(original));
    free(result);

    write_stored_zip("test-vfs-replace.zip", "payload.bin",
                     replacement, sizeof(replacement));
    result = vfs_find_sha256(&vfs, original_hash, &size);
    assert(result == NULL);
    result = vfs_find_sha256(&vfs, replacement_hash, &size);
    assert(result && size == sizeof(replacement) &&
           memcmp(result, replacement, size) == 0);
    free(result);
    vfs_free(&vfs);
    assert(remove("test-vfs-replace.zip") == 0);
}

static void test_rejects_overlong_archive_names(void) {
    char name[513];
    memset(name, 'a', sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    const char *test_dir = "test-vfs-overlong-dir";
    const char *test_zip = "test-vfs-overlong-dir/archive.zip";
    /* Keep this negative test hermetic.  The other VFS tests intentionally
     * scan the repository root, so an interrupted earlier run must not leave
     * a valid WALLA.PL5 fixture that makes this overlong-name assertion pass
     * for the wrong archive. */
    (void)remove(test_zip);
    (void)TEST_RMDIR(test_dir);
    assert(TEST_MKDIR(test_dir) == 0);
    write_stored_zip(test_zip, name, (const unsigned char *)"x", 1);

    DataVFS vfs;
    assert(vfs_init(&vfs, test_dir));
    size_t size = 0;
    assert(vfs_read_file(&vfs, "CAPICS/WALLA.PL5", &size) == NULL);
    vfs_free(&vfs);
    assert(remove(test_zip) == 0);
    assert(TEST_RMDIR(test_dir) == 0);
}

static void test_rejects_invalid_hash_text(void) {
    DataVFS vfs;
    assert(vfs_init(&vfs, "."));
    size_t size = 0;
    assert(vfs_find_sha256(&vfs, "../../outside", &size) == NULL);
    vfs_free(&vfs);
}

static void test_rejects_overlong_data_path(void) {
    char path[sizeof(((DataVFS *)0)->data_path) + 1U];
    memset(path, 'x', sizeof(path) - 1U);
    path[sizeof(path) - 1U] = '\0';

    DataVFS vfs;
    assert(!vfs_init(&vfs, path));
}

static void test_finds_hash_in_extracted_tree(void) {
    const unsigned char payload[] = { 9, 8, 7, 6 };
    const char *test_dir = "test-vfs-loose-dir";
    const char *test_file = "test-vfs-loose-dir/test-vfs-loose.bin";
    assert(TEST_MKDIR(test_dir) == 0);
    FILE *f = fopen(test_file, "wb");
    assert(f && fwrite(payload, 1, sizeof(payload), f) == sizeof(payload));
    assert(fclose(f) == 0);
    DataVFS vfs;
    assert(vfs_init(&vfs, test_dir));
    size_t size = 0;
    uint8_t *result = vfs_find_sha256(&vfs,
        "63d987d1c6d69751c17297f410f5b3547a65d096a8993b35bcb4f9cad054f176", &size);
    assert(result && size == sizeof(payload) && memcmp(result, payload, sizeof(payload)) == 0);
    free(result);

    /* Replacing a loose file must invalidate the metadata-backed cache; an
       old payload must never satisfy a hash lookup after the source changes. */
    const unsigned char replacement[] = { 5, 6, 7, 8 };
    f = fopen(test_file, "wb");
    assert(f && fwrite(replacement, 1, sizeof(replacement), f) == sizeof(replacement));
    assert(fclose(f) == 0);
    result = vfs_find_sha256(&vfs,
        "63d987d1c6d69751c17297f410f5b3547a65d096a8993b35bcb4f9cad054f176", &size);
    assert(result == NULL);
    free(result);
    result = vfs_find_sha256(&vfs,
        "55e5509f8052998294266ee5b50cb592938191fb5d67f73cac2e60b0276b1bdd", &size);
    assert(result && size == sizeof(replacement) && memcmp(result, replacement, size) == 0);
    free(result);
    vfs_free(&vfs);
    assert(remove(test_file) == 0);
    assert(TEST_RMDIR(test_dir) == 0);
}

static void test_negative_cache_invalidates_when_source_changes(void) {
    const char *test_dir = "test-vfs-negative-cache";
    const char *test_file = "test-vfs-negative-cache/payload.bin";
    const char *payload_hash =
        "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a";
    static const unsigned char missing_payload[] = "not-game-data";
    static const unsigned char matching_payload[] = { 1, 2, 3, 4 };
    assert(TEST_MKDIR(test_dir) == 0);
    FILE *f = fopen(test_file, "wb");
    assert(f && fwrite(missing_payload, 1, sizeof(missing_payload) - 1U, f) ==
                    sizeof(missing_payload) - 1U);
    assert(fclose(f) == 0);

    DataVFS vfs;
    assert(vfs_init(&vfs, test_dir));
    size_t size = 0;
    assert(vfs_find_sha256(&vfs, payload_hash, &size) == NULL);
    /* The repeated miss must be served by the persistent negative entry. */
    assert(vfs_find_sha256(&vfs, payload_hash, &size) == NULL);

    f = fopen(test_file, "wb");
    assert(f && fwrite(matching_payload, 1, sizeof(matching_payload), f) ==
                    sizeof(matching_payload));
    assert(fclose(f) == 0);
    uint8_t *result = vfs_find_sha256(&vfs, payload_hash, &size);
    assert(result && size == sizeof(matching_payload) &&
           memcmp(result, matching_payload, size) == 0);
    free(result);
    vfs_free(&vfs);
    assert(remove(test_file) == 0);
    assert(TEST_RMDIR(test_dir) == 0);
}

static void test_reads_and_hashes_empty_file(void) {
    FILE *f = fopen("test-vfs-empty.bin", "wb");
    assert(f && fclose(f) == 0);

    DataVFS vfs;
    assert(vfs_init(&vfs, "."));
    size_t size = 99;
    uint8_t *result = vfs_read_file(&vfs, "test-vfs-empty.bin", &size);
    assert(result && size == 0);
    free(result);

    result = vfs_find_sha256(&vfs,
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        &size);
    assert(result && size == 0);
    free(result);
    vfs_free(&vfs);
    assert(remove("test-vfs-empty.bin") == 0);
}

static void test_reads_empty_stored_zip_entry(void) {
    const unsigned char empty = 0;
    write_stored_zip("test-vfs-empty.zip", "empty.bin", &empty, 0);
    DataVFS vfs;
    assert(vfs_init(&vfs, "."));
    size_t size = 99;
    uint8_t *result = vfs_read_file(&vfs, "empty.bin", &size);
    assert(result && size == 0);
    free(result);
    vfs_free(&vfs);
    assert(remove("test-vfs-empty.zip") == 0);
}

static void test_reads_empty_deflated_zip_entry(void) {
    write_empty_deflated_zip("test-vfs-empty-deflated.zip", "empty.bin");
    DataVFS vfs;
    assert(vfs_init(&vfs, "."));
    size_t size = 99;
    uint8_t *result = vfs_read_file(&vfs, "empty.bin", &size);
    assert(result && size == 0);
    free(result);
    vfs_free(&vfs);
    assert(remove("test-vfs-empty-deflated.zip") == 0);
}

static void test_finds_hash_in_nested_zip(void) {
    const unsigned char payload[] = { 0xca, 0xfe, 0xba, 0xbe };
    write_stored_zip("test-vfs-inner.zip", "original-media.adf", payload, sizeof(payload));
    FILE *inner = fopen("test-vfs-inner.zip", "rb");
    assert(inner && fseek(inner, 0, SEEK_END) == 0);
    long inner_size = ftell(inner);
    assert(inner_size > 0 && fseek(inner, 0, SEEK_SET) == 0);
    unsigned char *inner_bytes = malloc((size_t)inner_size);
    assert(inner_bytes && fread(inner_bytes, 1, (size_t)inner_size, inner) == (size_t)inner_size);
    assert(fclose(inner) == 0);
    assert(remove("test-vfs-inner.zip") == 0);
    write_stored_zip("test-vfs-outer.zip", "preservation-copy.zip", inner_bytes, (unsigned)inner_size);
    free(inner_bytes);

    DataVFS vfs;
    assert(vfs_init(&vfs, "."));
    size_t size = 0;
    uint8_t *result = vfs_find_sha256(&vfs,
        "65ab12a8ff3263fbc257e5ddf0aa563c64573d0bab1f1115b9b107834cfa6971", &size);
    assert(result && size == sizeof(payload) && memcmp(result, payload, sizeof(payload)) == 0);
    free(result);
    vfs_free(&vfs);
    assert(remove("test-vfs-outer.zip") == 0);
}

int main(void) {
    char original_cwd[1024];
    char test_root[1024];
    assert(TEST_GETCWD(original_cwd, sizeof(original_cwd)) != NULL);
    assert(create_test_root(test_root, sizeof(test_root)));
    assert(TEST_CHDIR(test_root) == 0);

    vfs_free(NULL);
    test_reads_prefixed_case_insensitive_zip_entry();
    test_replacing_zip_invalidates_live_cache();
    test_reads_zip_in_nested_data_directory();
    test_rejects_overlong_archive_names();
    test_rejects_invalid_hash_text();
    test_rejects_overlong_data_path();
    test_finds_hash_in_extracted_tree();
    test_negative_cache_invalidates_when_source_changes();
    test_reads_and_hashes_empty_file();
    test_reads_empty_stored_zip_entry();
    test_reads_empty_deflated_zip_entry();
    test_finds_hash_in_nested_zip();

    assert(TEST_CHDIR(original_cwd) == 0);
    assert(TEST_RMDIR(test_root) == 0);
    puts("All data VFS tests passed");
    return 0;
}
