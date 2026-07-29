#include "data_vfs.h"
#include "sha256.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    put32(f, 0x04034b50); put16(f, 20); put16(f, 0); put16(f, 0);
    put16(f, 0); put16(f, 0); put32(f, 0); put32(f, data_size); put32(f, data_size);
    put16(f, name_size); put16(f, 0);
    fwrite(name, 1, name_size, f);
    fwrite(data, 1, data_size, f);

    unsigned cd_offset = 30 + name_size + data_size;
    put32(f, 0x02014b50); put16(f, 20); put16(f, 20); put16(f, 0); put16(f, 0);
    put16(f, 0); put16(f, 0); put32(f, 0); put32(f, data_size); put32(f, data_size);
    put16(f, name_size); put16(f, 0); put16(f, 0); put16(f, 0); put16(f, 0);
    put32(f, 0); put32(f, 0);
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
    vfs_free(&vfs);
    assert(remove("test-vfs-assets.zip") == 0);
}

static void test_rejects_overlong_archive_names(void) {
    char name[513];
    memset(name, 'a', sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    write_stored_zip("test-vfs-overlong.zip", name, (const unsigned char *)"x", 1);

    DataVFS vfs;
    assert(vfs_init(&vfs, "."));
    size_t size = 0;
    assert(vfs_read_file(&vfs, "CAPICS/WALLA.PL5", &size) == NULL);
    vfs_free(&vfs);
    assert(remove("test-vfs-overlong.zip") == 0);
}

static void test_finds_hash_in_extracted_tree(void) {
    const unsigned char payload[] = { 9, 8, 7, 6 };
    FILE *f = fopen("test-vfs-loose.bin", "wb");
    assert(f && fwrite(payload, 1, sizeof(payload), f) == sizeof(payload));
    assert(fclose(f) == 0);
    DataVFS vfs;
    assert(vfs_init(&vfs, "."));
    size_t size = 0;
    uint8_t *result = vfs_find_sha256(&vfs,
        "63d987d1c6d69751c17297f410f5b3547a65d096a8993b35bcb4f9cad054f176", &size);
    assert(result && size == sizeof(payload) && memcmp(result, payload, sizeof(payload)) == 0);
    free(result);
    vfs_free(&vfs);
    assert(remove("test-vfs-loose.bin") == 0);
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
    test_reads_prefixed_case_insensitive_zip_entry();
    test_rejects_overlong_archive_names();
    test_finds_hash_in_extracted_tree();
    test_finds_hash_in_nested_zip();
    puts("All data VFS tests passed");
    return 0;
}
