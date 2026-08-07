#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define W 320
#define H 200

static void write_bmp(const char *path, const uint8_t *pixels, const uint32_t *pal) {
    int row_padded = (W * 3 + 3) & ~3;
    int file_size = 54 + row_padded * H;
    FILE *f = fopen(path, "wb");
    if (!f) return;
    uint8_t hdr[54] = {0};
    hdr[0]='B'; hdr[1]='M';
    hdr[2]=file_size; hdr[3]=file_size>>8; hdr[4]=file_size>>16; hdr[5]=file_size>>24;
    hdr[10]=54; hdr[14]=40;
    hdr[18]=W&0xFF; hdr[19]=W>>8;
    hdr[22]=H&0xFF; hdr[23]=H>>8;
    hdr[26]=1; hdr[28]=24;
    fwrite(hdr, 1, 54, f);
    uint8_t *row = calloc(row_padded, 1);
    for (int y = H-1; y >= 0; y--) {
        for (int x = 0; x < W; x++) {
            uint32_t c = pal[pixels[y*W+x] % 32];
            row[x*3] = c&0xFF; row[x*3+1] = (c>>8)&0xFF; row[x*3+2] = (c>>16)&0xFF;
        }
        fwrite(row, 1, row_padded, f);
    }
    free(row);
    fclose(f);
    printf("  Wrote %s\n", path);
}

static const uint32_t pal[32] = {
    0xFF000000, 0xFFAA0000, 0xFF00AA00, 0xFFAAAA00,
    0xFF0000AA, 0xFFAA00AA, 0xFF0055AA, 0xFFAAAAAA,
    0xFF555555, 0xFFFF5555, 0xFF55FF55, 0xFFFFFF55,
    0xFF5555FF, 0xFFFF55FF, 0xFF55FFFF, 0xFFFFFFFF,
    0xFF000000, 0xFF140000, 0xFF001400, 0xFF141400,
    0xFF000014, 0xFF140014, 0xFF001414, 0xFF141414,
    0xFF0A0A0A, 0xFF1E0A0A, 0xFF0A1E0A, 0xFF1E1E0A,
    0xFF0A0A1E, 0xFF1E0A1E, 0xFF0A1E1E, 0xFF1E1E1E,
};

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 1;
    uint8_t *data = malloc(40000);
    if (!data) { fclose(f); return 1; }
    memset(data, 0, 40000);
    fread(data, 1, 40000, f);
    fclose(f);

    uint8_t *pixels = calloc(W * H, 1);
    if (!pixels) { free(data); return 1; }
    const char *base = argc > 2 ? argv[2] : "test";
    char path[256];

    // Method 1: Atari ST interleaved (5 words per 16px group)
    {
        memset(pixels, 0, W*H);
        const uint8_t *src = data;
        for (int y = 0; y < H; y++) {
            for (int g = 0; g < 20; g++) {
                uint16_t p[5];
                for (int i = 0; i < 5; i++) { p[i] = (src[0]<<8)|src[1]; src+=2; }
                for (int b = 15; b >= 0; b--) {
                    uint8_t px = 0;
                    for (int i = 0; i < 5; i++) if (p[i] & (1<<b)) px |= (1<<i);
                    pixels[y*W + g*16 + (15-b)] = px;
                }
            }
        }
        snprintf(path, sizeof(path), "%s_m1_st_interleaved.bmp", base);
        write_bmp(path, pixels, pal);
    }

    // Method 2: Sequential planes (each 8000 bytes)
    {
        memset(pixels, 0, W*H);
        for (int plane = 0; plane < 5; plane++) {
            const uint8_t *src = data + plane * 8000;
            for (int y = 0; y < H; y++) {
                for (int byte_idx = 0; byte_idx < 40; byte_idx++) {
                    uint8_t val = src[y * 40 + byte_idx];
                    for (int bit = 7; bit >= 0; bit--) {
                        if (val & (1 << bit))
                            pixels[y*W + byte_idx*8 + (7-bit)] |= (1 << plane);
                    }
                }
            }
        }
        snprintf(path, sizeof(path), "%s_m2_sequential.bmp", base);
        write_bmp(path, pixels, pal);
    }

    // Method 3: Line-interleaved (5 planes per line, 40 bytes each)
    {
        memset(pixels, 0, W*H);
        for (int y = 0; y < H; y++) {
            for (int plane = 0; plane < 5; plane++) {
                const uint8_t *src = data + y * 200 + plane * 40;
                for (int byte_idx = 0; byte_idx < 40; byte_idx++) {
                    uint8_t val = src[byte_idx];
                    for (int bit = 7; bit >= 0; bit--) {
                        if (val & (1 << bit))
                            pixels[y*W + byte_idx*8 + (7-bit)] |= (1 << plane);
                    }
                }
            }
        }
        snprintf(path, sizeof(path), "%s_m3_line_interleaved.bmp", base);
        write_bmp(path, pixels, pal);
    }

    // Method 4: Chunky 4bpp (nibble-packed, 160 bytes/line, first 32000 bytes)
    {
        memset(pixels, 0, W*H);
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < 160; x++) {
                uint8_t val = data[y * 160 + x];
                pixels[y*W + x*2] = (val >> 4) & 0x0F;
                pixels[y*W + x*2 + 1] = val & 0x0F;
            }
        }
        snprintf(path, sizeof(path), "%s_m4_chunky4.bmp", base);
        write_bmp(path, pixels, pal);
    }

    // Method 5: Chunky 8bpp 200x200
    {
        memset(pixels, 0, W*H);
        for (int y = 0; y < 200; y++) {
            for (int x = 0; x < 200; x++) {
                pixels[y*W + x] = data[y * 200 + x] % 32;
            }
        }
        snprintf(path, sizeof(path), "%s_m5_chunky8_200x200.bmp", base);
        write_bmp(path, pixels, pal);
    }

    free(pixels);
    free(data);
    return 0;
}
