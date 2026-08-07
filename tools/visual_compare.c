#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int width, height; uint8_t *pixels; } PpmImage;

static void image_free(PpmImage *image) {
    free(image->pixels);
    memset(image, 0, sizeof(*image));
}

static int read_number(FILE *file, int *out) {
    int c;
    do {
        c = fgetc(file);
        if (c == '#') while (c != '\n' && c != EOF) c = fgetc(file);
    } while (c != EOF && (c == ' ' || c == '\n' || c == '\r' || c == '\t'));
    if (c < '0' || c > '9') return 0;
    int value = 0;
    do {
        if (value > (INT32_MAX - (c - '0')) / 10) return 0;
        value = value * 10 + c - '0';
        c = fgetc(file);
    } while (c >= '0' && c <= '9');
    if (c != EOF) ungetc(c, file);
    *out = value;
    return 1;
}

static int image_read(const char *path, PpmImage *image) {
    FILE *file = fopen(path, "rb");
    char magic[2];
    int max_value = 0;
    int ok = file && fread(magic, 1, sizeof(magic), file) == sizeof(magic) &&
        memcmp(magic, "P6", 2) == 0 && read_number(file, &image->width) &&
        read_number(file, &image->height) && read_number(file, &max_value) &&
        image->width > 0 && image->height > 0 && max_value == 255 && fgetc(file) != EOF;
    size_t pixels = (size_t)image->width * (size_t)image->height;
    if (ok && pixels <= SIZE_MAX / 3U) image->pixels = malloc(pixels * 3U);
    else ok = 0;
    if (ok) ok = image->pixels && fread(image->pixels, 3U, pixels, file) == pixels;
    if (file) fclose(file);
    if (!ok) {
        fprintf(stderr, "invalid P6 image: %s%s%s\n", path,
                file ? "" : ": ", file ? "" : strerror(errno));
        image_free(image);
    }
    return ok;
}

static int image_write(const char *path, const PpmImage *image) {
    FILE *file = fopen(path, "wb");
    size_t pixels = (size_t)image->width * (size_t)image->height;
    int ok = file && fprintf(file, "P6\n%d %d\n255\n", image->width, image->height) > 0 &&
        fwrite(image->pixels, 3U, pixels, file) == pixels;
    if (file && (!ok || fclose(file) != 0)) { if (!ok) fclose(file); ok = 0; }
    if (!ok) fprintf(stderr, "could not write %s\n", path);
    return ok;
}

int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "usage: %s <original.ppm> <candidate.ppm> [diff.ppm]\n", argv[0]);
        return 2;
    }
    PpmImage original = {0}, candidate = {0}, diff = {0};
    if (!image_read(argv[1], &original) || !image_read(argv[2], &candidate)) goto fail;
    if (original.width != candidate.width || original.height != candidate.height) {
        fprintf(stderr, "dimension mismatch: original=%dx%d candidate=%dx%d\n",
                original.width, original.height, candidate.width, candidate.height);
        goto fail;
    }
    size_t pixels = (size_t)original.width * (size_t)original.height;
    if (argc == 4) {
        diff.width = original.width; diff.height = original.height;
        diff.pixels = malloc(pixels * 3U);
        if (!diff.pixels) goto fail;
    }
    size_t exact = 0;
    uint64_t total_error = 0;
    for (size_t pixel = 0; pixel < pixels; ++pixel) {
        unsigned maximum = 0;
        for (size_t channel = 0; channel < 3U; ++channel) {
            uint8_t a = original.pixels[pixel * 3U + channel];
            uint8_t b = candidate.pixels[pixel * 3U + channel];
            unsigned error = a > b ? (unsigned)(a - b) : (unsigned)(b - a);
            total_error += error;
            if (error > maximum) maximum = error;
        }
        if (!maximum) exact++;
        if (diff.pixels) {
            diff.pixels[pixel * 3U] = (uint8_t)maximum;
            diff.pixels[pixel * 3U + 1U] = 0;
            diff.pixels[pixel * 3U + 2U] = 0;
        }
    }
    printf("native=%dx%d exact_pixels=%zu/%zu (%.4f%%) mean_absolute_error=%.6f\n",
           original.width, original.height, exact, pixels, 100.0 * exact / pixels,
           (double)total_error / (pixels * 3U));
    if (diff.pixels && !image_write(argv[3], &diff)) goto fail;
    image_free(&original); image_free(&candidate); image_free(&diff);
    return 0;
fail:
    image_free(&original); image_free(&candidate); image_free(&diff);
    return 1;
}
