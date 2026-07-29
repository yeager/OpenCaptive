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

int main(void) {
    test_reads_prefixed_case_insensitive_zip_entry();
    test_rejects_overlong_archive_names();
    puts("All data VFS tests passed");
    return 0;
}
