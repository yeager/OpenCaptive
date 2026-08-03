#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "stb_image.h"

#include <stdio.h>
#include <stdlib.h>

unsigned char *load_png_file(const char *path, int *w, int *h) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *buf = malloc(len);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, len, f);
    fclose(f);
    int comp;
    unsigned char *pixels = stbi_load_from_memory(buf, (int)len, w, h, &comp, 4);
    free(buf);
    return pixels;
}
