#include "data_vfs.h"
#include "pl5_decoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SCREEN_W = 320, SCREEN_H = 200, TILE = 8 };
static const char *const sheet_hashes[] = {
    "47ad15b4a593c37880d0306b6a0f51b7a9f20615cf6a188f23716d5b48315524",
    "43833e4a8df622f84d53698a76c6d18f910c1cca79c6b89cbfacc563f695356c",
    "8b7301fc6c302fd673a81d23e7a99d715aa02d5b404c1e1edea19ceccccc9681",
    "519d3ef4494f0e868479a90c8a47249b840598e382c7ba3272f417ce3daf5936",
    "7edb8ee856a91e835ea86dda00af49fda3dae730d694bd7234b7fa96d711e296",
    /* The view is not limited to WALLA-E.  The original scene can layer
       enemy, animation, door, roof and object sheets on top of a wall. */
    "4a2bc840d184ff07657f56e630b0293f1d5d7cdbf1d00e4505a1a69dcf721667",
    "2c8db6bfbec2b463856ab4cd9a313f9fbf20be408a2e278d94d498653562f754",
    "0b0d6ee225493c92b534b50e893d9c27e423ce0a4298e1789682b8cf222b7adc",
    "1f1b89e7692dc7c01f9d649677c820e79076304e8bc79835683e14484d68bb5b",
    "fed16e510697e17123d474c08687de548076b26a55f08f1d00fd17e3fcdf9410",
    "63ffa6901b59d463b050088065503d386ca2f3813ed91d8e0833320f9df2fe11",
    "48df42e6906bfd167981f19e89149aa4c5791297b6e92f3a87470b59e8d0f1f3",
    "303c540f9e88ca9a8e736541b3c6f9a9cb9817b8640b5133ca7721e6db667e1d",
    "4edc60eb7d530ed6a7b11673d26831eb6701f131df3f9e291d882f5b78c2de25",
    "978d18857d5ffcf6fb7b91fb22c02b85079db0171caeac3d290a69b276cf098f",
    "dec7143f063c98459ab2f267ed135204cdee1b521eda9810b219e8c10e05c7e8",
    "d7338db4df839f0b1090234f6b3e30db1ab43c936be5479d007f865a0175cc32",
    "21db7daf64cff3b0cae19c3e7eb2057762df9110055e7253175024ecb146fb6b",
};
#define SHEETS ((int)(sizeof(sheet_hashes) / sizeof(sheet_hashes[0])))

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
        uint8_t *raw = vfs_find_sha256(&vfs, sheet_hashes[sheet], &size);
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
               sheet_hashes[match->sheet], match->x, match->y);
    }
    printf("exact-tiles=%u/%u\n", matched, total);
    free(tiles);
    for (int i = 0; i < SHEETS; ++i) { pl5_free(&images[i]); free(sheet_pixels[i]); }
    vfs_free(&vfs);
    return matched ? 0 : 1;
}
