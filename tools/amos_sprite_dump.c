#include "amos_sprite.h"
#include "liberation_data.h"
#include "iso9660_reader.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    if (argc != 5) return fprintf(stderr, "usage: %s <data-dir> <resource-sha256> <index> <output.ppm>\n", argv[0]), 2;
    char *end; unsigned long index = strtoul(argv[3], &end, 10); if (*end || index > 65535) return 2;
    DataVFS vfs; LiberationData data = {0}; size_t size; uint8_t *bytes = NULL; AmosSprite sprite; uint32_t *pixels = NULL; FILE *out = NULL;
    bool ok = vfs_init(&vfs, argv[1]) && liberation_data_open(&data, &vfs);
    if (ok) bytes = iso_read_file_sha256(&data.iso, argv[2], &size);
    if (ok) ok = bytes && amos_sprite_get(bytes, size, index, &sprite);
    if (ok) pixels = calloc((size_t)sprite.width * sprite.height, 4);
    if (ok) ok = pixels && amos_sprite_decode_argb(&sprite, pixels, (size_t)sprite.width * sprite.height);
    if (ok) out = fopen(argv[4], "wb"); if (ok) ok = out && fprintf(out, "P6\n%u %u\n255\n", sprite.width, sprite.height) > 0;
    for (unsigned y = 0; ok && y < sprite.height; ++y) for (unsigned x = 0; x < sprite.width; ++x) { uint32_t p = pixels[(size_t)y * sprite.width + x]; uint8_t c[3] = {p >> 16, p >> 8, p}; if (fwrite(c, 1, 3, out) != 3) ok = false; }
    if (out && fclose(out)) ok = false; free(pixels); free(bytes); liberation_data_close(&data); vfs_free(&vfs); return ok ? 0 : 1;
}
