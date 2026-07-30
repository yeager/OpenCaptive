#include "data_vfs.h"
#include "pl5_decoder.h"
#include "captive_scene_assets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SCREEN_W = 320, SCREEN_H = 200, TILE = 8, SHEETS = CAPTIVE_VIEW_SOURCE_COUNT };

typedef struct { uint64_t hash; uint16_t x, y; uint8_t sheet; } TileRef;

static uint64_t tile_hash(const uint32_t *pixels, int stride, int x, int y) {
    uint64_t hash = 1469598103934665603ULL;
    for (int row = 0; row < TILE; ++row) for (int col = 0; col < TILE; ++col) {
        uint32_t p = pixels[(y + row) * stride + x + col];
        for (int byte = 0; byte < 4; ++byte) {
            hash ^= (p >> (byte * 8)) & 255U;
            hash *= 1099511628211ULL;
        }
    }
    return hash;
}

static int compare_tile_ref(const void *a, const void *b) {
    const TileRef *left = a, *right = b;
    return left->hash < right->hash ? -1 : left->hash > right->hash;
}

static bool read_ppm(const char *path, uint32_t *pixels) {
    FILE *file = fopen(path, "rb");
    char magic[3] = {0}, token[32] = {0};
    int width = 0, height = 0, maximum = 0;
    if (!file || fscanf(file, "%2s", magic) != 1 || strcmp(magic, "P6") != 0 ||
        fscanf(file, "%31s", token) != 1 || (width = atoi(token)) != SCREEN_W ||
        fscanf(file, "%31s", token) != 1 || (height = atoi(token)) != SCREEN_H ||
        fscanf(file, "%31s", token) != 1 || (maximum = atoi(token)) != 255 || fgetc(file) == EOF) {
        if (file) fclose(file);
        return false;
    }
    for (size_t i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        uint8_t rgb[3];
        if (fread(rgb, 1, sizeof(rgb), file) != sizeof(rgb)) { fclose(file); return false; }
        pixels[i] = 0xff000000U | ((uint32_t)rgb[0] << 16) |
                    ((uint32_t)rgb[1] << 8) | rgb[2];
    }
    bool ok = fclose(file) == 0;
    return ok;
}

static bool same_tile(const uint32_t *target, const uint32_t *sheet,
                      int target_x, int target_y, int source_x, int source_y) {
    for (int y = 0; y < TILE; ++y)
        if (memcmp(target + (target_y + y) * SCREEN_W + target_x,
                   sheet + (source_y + y) * SCREEN_W + source_x,
                   TILE * sizeof(*target)) != 0) return false;
    return true;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <data-dir> <hash-verified-dos-reference.ppm>\n", argv[0]);
        return 2;
    }
    uint32_t target[SCREEN_W * SCREEN_H];
    if (!read_ppm(argv[2], target)) {
        fprintf(stderr, "reference must be a 320x200 binary PPM\n");
        return 1;
    }
    DataVFS vfs;
    PL5Image images[SHEETS] = {0};
    uint32_t *sheet_pixels[SHEETS] = {0};
    bool ok = vfs_init(&vfs, argv[1]);
    for (int sheet = 0; ok && sheet < SHEETS; ++sheet) {
        size_t size = 0;
        uint8_t *raw = vfs_find_sha256(&vfs, captive_view_source_hashes[sheet], &size);
        ok = raw && pl5_decode(raw, size, &images[sheet]);
        free(raw);
        if (ok) {
            sheet_pixels[sheet] = calloc(SCREEN_W * SCREEN_H, sizeof(uint32_t));
            ok = sheet_pixels[sheet] != NULL;
            for (size_t i = 0; ok && i < SCREEN_W * SCREEN_H; ++i)
                sheet_pixels[sheet][i] = images[sheet].palette[images[sheet].pixel_data[i] & 31U];
        }
    }
    if (!ok) {
        fprintf(stderr, "one or more hash-identified PL5 panel sheets were not available\n");
        for (int i = 0; i < SHEETS; ++i) { pl5_free(&images[i]); free(sheet_pixels[i]); }
        vfs_free(&vfs);
        return 1;
    }
    size_t capacity = (size_t)SHEETS * (SCREEN_W - TILE + 1) * (SCREEN_H - TILE + 1);
    TileRef *tiles = malloc(capacity * sizeof(*tiles));
    if (!tiles) return 1;
    size_t count = 0;
    for (int sheet = 0; sheet < SHEETS; ++sheet) for (int y = 0; y <= SCREEN_H - TILE; ++y)
        for (int x = 0; x <= SCREEN_W - TILE; ++x)
            tiles[count++] = (TileRef){tile_hash(sheet_pixels[sheet], SCREEN_W, x, y), x, y, sheet};
    qsort(tiles, count, sizeof(*tiles), compare_tile_ref);

    unsigned matched = 0, total = 0;
    for (int y = 55; y < 55 + 112; y += TILE) for (int x = 32; x < 32 + 144; x += TILE) {
        ++total;
        TileRef key = {.hash = tile_hash(target, SCREEN_W, x, y)};
        TileRef *first = bsearch(&key, tiles, count, sizeof(*tiles), compare_tile_ref);
        if (!first) { printf("target=%3d,%3d match=none\n", x, y); continue; }
        while (first > tiles && (first - 1)->hash == key.hash) --first;
        TileRef *match = NULL;
        for (TileRef *candidate = first; candidate < tiles + count && candidate->hash == key.hash; ++candidate)
            if (same_tile(target, sheet_pixels[candidate->sheet], x, y, candidate->x, candidate->y)) {
                match = candidate; break;
            }
        if (!match) { printf("target=%3d,%3d match=collision\n", x, y); continue; }
        ++matched;
        printf("target=%3d,%3d source=%s@%u,%u\n", x, y,
               captive_view_source_hashes[match->sheet], match->x, match->y);
    }
    printf("exact-tiles=%u/%u\n", matched, total);
    free(tiles);
    for (int i = 0; i < SHEETS; ++i) { pl5_free(&images[i]); free(sheet_pixels[i]); }
    vfs_free(&vfs);
    return matched ? 0 : 1;
}
