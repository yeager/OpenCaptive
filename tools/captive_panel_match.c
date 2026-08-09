#include "data_vfs.h"
#include "pl5_decoder.h"
#include "captive_scene_assets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SCREEN_W = 320, SCREEN_H = 200, TILE = 8, SHEETS = CAPTIVE_VIEW_SOURCE_COUNT };
enum { VIEW_X = 32, VIEW_Y = 55, VIEW_W = 144, VIEW_H = 112,
       VIEW_COLS = VIEW_W / TILE, VIEW_ROWS = VIEW_H / TILE };

typedef struct { uint64_t hash; uint16_t x, y; uint8_t sheet; } TileRef;
typedef struct { int sheet, source_x, source_y; bool present, used; } MatchedTile;

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

static bool masked_tile_match(const PL5Image *image, const uint32_t *sheet,
                              const uint32_t *target, int target_x,
                              int target_y, int source_x, int source_y,
                              int *opaque_out) {
    int opaque = 0;
    for (int y = 0; y < TILE; ++y) for (int x = 0; x < TILE; ++x) {
        uint8_t index = image->pixel_data[(source_y + y) * SCREEN_W + source_x + x] & 31U;
        if (index == 0) continue;
        ++opaque;
        if (sheet[(source_y + y) * SCREEN_W + source_x + x] !=
            target[(target_y + y) * SCREEN_W + target_x + x])
            return false;
    }
    if (opaque_out) *opaque_out = opaque;
    return opaque > 0;
}

/* Palette index zero is transparent in the original panel blitters.  A tile
 * made almost entirely of it cannot establish a source rectangle: identical
 * black areas occur in many sheets.  Require a meaningful number of opaque
 * source pixels before treating an exact RGB hit as recovery evidence. */
static bool tile_has_opaque_pixels(const PL5Image *image, int source_x, int source_y) {
    unsigned opaque = 0;
    for (int y = 0; y < TILE; ++y) for (int x = 0; x < TILE; ++x)
        opaque += (image->pixel_data[(source_y + y) * SCREEN_W + source_x + x] & 31U) != 0;
    return opaque >= 16;
}

static bool same_translation(const MatchedTile *tile, int sheet, int dx, int dy,
                             int target_x, int target_y) {
    return tile->present && tile->sheet == sheet &&
        target_x - tile->source_x == dx && target_y - tile->source_y == dy;
}

/* A rectangle emitted here is stronger evidence than a palette-colour match:
 * every included 8x8 tile has already been byte-compared with an original
 * source surface and all tiles share one affine source-to-destination copy. */
static void print_blit_candidates(MatchedTile matches[VIEW_ROWS][VIEW_COLS]) {
    for (int row = 0; row < VIEW_ROWS; ++row) for (int col = 0; col < VIEW_COLS; ++col) {
        MatchedTile *first = &matches[row][col];
        if (!first->present || first->used) continue;
        int target_x = VIEW_X + col * TILE, target_y = VIEW_Y + row * TILE;
        int dx = target_x - first->source_x, dy = target_y - first->source_y;
        int width = 1;
        while (col + width < VIEW_COLS && !matches[row][col + width].used &&
               same_translation(&matches[row][col + width], first->sheet, dx, dy,
                                target_x + width * TILE, target_y)) ++width;
        int height = 1;
        bool row_matches = true;
        while (row + height < VIEW_ROWS && row_matches) {
            for (int x = 0; x < width; ++x)
                if (matches[row + height][col + x].used ||
                    !same_translation(&matches[row + height][col + x], first->sheet, dx, dy,
                                      target_x + x * TILE, target_y + height * TILE)) {
                    row_matches = false;
                    break;
                }
            if (row_matches) ++height;
        }
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x)
            matches[row + y][col + x].used = true;
        printf("blit-candidate source=%s@%d,%d target=%d,%d size=%dx%d\n",
               captive_view_source_hashes[first->sheet], first->source_x, first->source_y,
               target_x, target_y, width * TILE, height * TILE);
    }
}

int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "usage: %s <data-dir> <hash-verified-dos-reference.ppm> [--masked]\n", argv[0]);
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
    if (argc == 4 && strcmp(argv[3], "--masked") == 0) {
        /* Search only real PL5 source pixels.  A later original blit may cover
         * transparent/source-zero pixels, so exact full-tile equality is too
         * strict for recovering the original command stream. */
        for (int y = VIEW_Y; y < VIEW_Y + VIEW_H; y += TILE) {
            for (int x = VIEW_X; x < VIEW_X + VIEW_W; x += TILE) {
                int best_sheet = -1, best_sx = -1, best_sy = -1, best_opaque = 0;
                for (int sheet = 0; sheet < SHEETS; ++sheet) {
                    for (int sy = 0; sy <= SCREEN_H - TILE; sy += TILE) {
                        for (int sx = 0; sx <= SCREEN_W - TILE; sx += TILE) {
                            int opaque = 0;
                            if (masked_tile_match(&images[sheet], sheet_pixels[sheet],
                                                  target, x, y, sx, sy, &opaque) &&
                                opaque > best_opaque) {
                                best_sheet = sheet;
                                best_sx = sx;
                                best_sy = sy;
                                best_opaque = opaque;
                            }
                        }
                    }
                }
                if (best_sheet >= 0 && best_opaque >= 16)
                    printf("masked target=%d,%d source=%s@%d,%d opaque=%d\n",
                           x, y, captive_view_source_hashes[best_sheet],
                           best_sx, best_sy, best_opaque);
            }
        }
        for (int i = 0; i < SHEETS; ++i) { pl5_free(&images[i]); free(sheet_pixels[i]); }
        vfs_free(&vfs);
        return 0;
    }
    size_t capacity = (size_t)SHEETS * (SCREEN_W - TILE + 1) * (SCREEN_H - TILE + 1);
    TileRef *tiles = malloc(capacity * sizeof(*tiles));
    if (!tiles) return 1;
    size_t count = 0;
    for (int sheet = 0; sheet < SHEETS; ++sheet) for (int y = 0; y <= SCREEN_H - TILE; ++y)
        for (int x = 0; x <= SCREEN_W - TILE; ++x)
            tiles[count++] = (TileRef){tile_hash(sheet_pixels[sheet], SCREEN_W, x, y), x, y, sheet};
    qsort(tiles, count, sizeof(*tiles), compare_tile_ref);

    MatchedTile matches[VIEW_ROWS][VIEW_COLS] = {0};
    unsigned matched = 0, total = 0;
    for (int y = VIEW_Y; y < VIEW_Y + VIEW_H; y += TILE) for (int x = VIEW_X; x < VIEW_X + VIEW_W; x += TILE) {
        ++total;
        TileRef key = {.hash = tile_hash(target, SCREEN_W, x, y)};
        TileRef *first = bsearch(&key, tiles, count, sizeof(*tiles), compare_tile_ref);
        if (!first) { printf("target=%3d,%3d match=none\n", x, y); continue; }
        while (first > tiles && (first - 1)->hash == key.hash) --first;
        TileRef *match = NULL;
        for (TileRef *candidate = first; candidate < tiles + count && candidate->hash == key.hash; ++candidate)
            if (tile_has_opaque_pixels(&images[candidate->sheet], candidate->x, candidate->y) &&
                same_tile(target, sheet_pixels[candidate->sheet], x, y, candidate->x, candidate->y)) {
                match = candidate; break;
            }
        if (!match) { printf("target=%3d,%3d match=collision\n", x, y); continue; }
        ++matched;
        matches[(y - VIEW_Y) / TILE][(x - VIEW_X) / TILE] =
            (MatchedTile){match->sheet, match->x, match->y, true, false};
        printf("target=%3d,%3d source=%s@%u,%u\n", x, y,
               captive_view_source_hashes[match->sheet], match->x, match->y);
    }
    print_blit_candidates(matches);
    printf("exact-tiles=%u/%u\n", matched, total);
    free(tiles);
    for (int i = 0; i < SHEETS; ++i) { pl5_free(&images[i]); free(sheet_pixels[i]); }
    vfs_free(&vfs);
    return matched ? 0 : 1;
}
