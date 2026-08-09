#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
#include "stb_image.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "png_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

unsigned char *load_png_file(const char *path, int *w, int *h) {
    if (!path || !w || !h) return NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long len = ftell(f);
    if (len <= 0 || len > INT_MAX || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f); return NULL;
    }
    unsigned char *buf = malloc(len);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)len, f) != (size_t)len) {
        free(buf); fclose(f); return NULL;
    }
    fclose(f);
    int comp;
    unsigned char *pixels = stbi_load_from_memory(buf, (int)len, w, h, &comp, 4);
    free(buf);
    return pixels;
}
