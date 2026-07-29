#include "anm_decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_bmp(const char *path, int width, int height,
                      const uint8_t *pixels, const uint32_t *palette) {
    int row_stride = width * 3;
    int row_padded = (row_stride + 3) & ~3;
    int pixel_size = row_padded * height;
    int file_size = 54 + pixel_size;

    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "Cannot write %s\n", path); return; }

    uint8_t header[54] = {0};
    header[0] = 'B'; header[1] = 'M';
    header[2] = file_size; header[3] = file_size >> 8;
    header[4] = file_size >> 16; header[5] = file_size >> 24;
    header[10] = 54;
    header[14] = 40;
    header[18] = width; header[19] = width >> 8;
    header[22] = height; header[23] = height >> 8;
    header[26] = 1;
    header[28] = 24;

    fwrite(header, 1, 54, f);

    uint8_t *row = calloc(row_padded, 1);
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            uint8_t idx = pixels[y * width + x];
            uint32_t color = palette[idx];
            row[x * 3 + 0] = color & 0xFF;
            row[x * 3 + 1] = (color >> 8) & 0xFF;
            row[x * 3 + 2] = (color >> 16) & 0xFF;
        }
        fwrite(row, 1, row_padded, f);
    }
    free(row);
    fclose(f);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: anm_extract <file.ANM> [output_prefix]\n");
        fprintf(stderr, "Extracts animation frames as BMP files\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", argv[1]); return 1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    ANMAnimation anim;
    if (!anm_decode(data, size, &anim)) {
        fprintf(stderr, "Failed to decode ANM (%ld bytes)\n", size);
        free(data);
        return 1;
    }

    const char *prefix = argc > 2 ? argv[2] : "frame";
    printf("Decoded %d frames (%dx%d)\n", anim.frame_count, anim.width, anim.height);

    for (int i = 0; i < anim.frame_count; i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s_%03d.bmp", prefix, i);
        write_bmp(path, anim.width, anim.height, anim.frames[i], anim.palette);
        printf("  Wrote %s\n", path);
    }

    anm_free(&anim);
    free(data);
    return 0;
}
