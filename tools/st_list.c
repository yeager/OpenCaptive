#include "st_disk_reader.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_hash(const uint8_t digest[32]) {
    for (int i = 0; i < 32; i++) printf("%02x", digest[i]);
}

int main(int argc, char **argv) {
    bool hashes = argc == 3 && strcmp(argv[2], "--hash") == 0;
    const char *requested_hash = argc == 4 && strcmp(argv[2], "--read-sha") == 0
        ? argv[3] : NULL;
    if (argc != 2 && !hashes && !requested_hash) {
        fprintf(stderr, "usage: %s <st-image> [--hash | --read-sha <sha256>]\n", argv[0]);
        return 2;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) { fclose(f); return 1; }
    uint8_t *image = malloc((size_t)size);
    if (!image || fread(image, 1, (size_t)size, f) != (size_t)size) {
        free(image); fclose(f); return 1;
    }
    fclose(f);

    STDisk disk;
    if (!st_disk_open(&disk, image, (size_t)size)) {
        fprintf(stderr, "not a valid FAT12 Atari ST disk image\n");
        free(image);
        return 1;
    }
    STEntry entries[256];
    int count = st_disk_list_root(&disk, entries, 256);
    for (int i = 0; i < count; i++) {
        if (entries[i].is_dir) continue;
        uint32_t file_size = 0;
        uint8_t *data = st_disk_read_file(&disk, entries[i].cluster, entries[i].size, &file_size);
        if (!data || file_size != entries[i].size) { free(data); continue; }
        uint8_t digest[32];
        sha256_digest(data, file_size, digest);
        if (requested_hash && sha256_matches_hex(digest, requested_hash)) {
            fwrite(data, 1, file_size, stdout);
            free(data);
            free(image);
            return 0;
        }
        if (!requested_hash) {
            printf("%8u ", file_size);
            if (hashes) { print_hash(digest); printf(" "); }
            printf("%s\n", entries[i].name);
        }
        free(data);
    }
    free(image);
    return requested_hash ? 1 : 0;
}
