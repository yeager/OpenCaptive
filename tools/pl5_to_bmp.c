#include "pl5_decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_bmp(const char *path, const PL5Image *img) {
    int row_stride = img->width * 3;
    int row_padded = (row_stride + 3) & ~3;
    int pixel_size = row_padded * img->height;
    int file_size = 54 + pixel_size;

    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "Cannot write %s\n", path); return; }

    uint8_t header[54] = {0};
    // BMP header
    header[0] = 'B'; header[1] = 'M';
    header[2] = file_size; header[3] = file_size >> 8;
    header[4] = file_size >> 16; header[5] = file_size >> 24;
    header[10] = 54;
    // DIB header
    header[14] = 40;
    header[18] = img->width; header[19] = img->width >> 8;
    header[22] = img->height; header[23] = img->height >> 8;
    header[26] = 1;
    header[28] = 24;

    fwrite(header, 1, 54, f);

    uint8_t *row = calloc(row_padded, 1);
    if (!row) { fclose(f); return; }
    for (int y = img->height - 1; y >= 0; y--) {
        for (int x = 0; x < (int)img->width; x++) {
            uint8_t idx = img->pixel_data[y * img->width + x];
            uint32_t color = img->palette[idx % 32];
            row[x * 3 + 0] = color & 0xFF;
            row[x * 3 + 1] = (color >> 8) & 0xFF;
            row[x * 3 + 2] = (color >> 16) & 0xFF;
        }
        fwrite(row, 1, row_padded, f);
    }
    free(row);
    fclose(f);
    printf("Wrote %s (%dx%d)\n", path, img->width, img->height);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: pl5_to_bmp <file.PL5> [output.bmp]\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", argv[1]); return 1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) { fprintf(stderr, "Empty file\n"); fclose(f); return 1; }
    uint8_t *data = malloc(size);
    if (!data) { fprintf(stderr, "Out of memory\n"); fclose(f); return 1; }
    fread(data, 1, size, f);
    fclose(f);

    PL5Image img;
    if (!pl5_decode(data, size, &img)) {
        fprintf(stderr, "Failed to decode PL5\n");
        free(data);
        return 1;
    }

    const char *outpath = argc > 2 ? argv[2] : "output.bmp";
    write_bmp(outpath, &img);

    pl5_free(&img);
    free(data);
    return 0;
}
