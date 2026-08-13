#include "captive_dos_runtime.h"
#include "dos_vga_reference.h"

#include <stdio.h>
#include <stdlib.h>

static bool read_dump(const char *path, uint8_t *memory) {
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    bool ok = fread(memory, 1, DOS_VGA_MEMORY_SIZE, file) == DOS_VGA_MEMORY_SIZE;
    fclose(file);
    return ok;
}

static bool write_ppm(const char *path, const uint32_t *pixels) {
    FILE *file = fopen(path, "wb");
    if (!file) return false;
    bool ok = fprintf(file, "P6\n320 200\n255\n") > 0;
    for (size_t i = 0; ok && i < DOS_VGA_FRAME_SIZE; ++i) {
        uint8_t rgb[3] = {(uint8_t)(pixels[i] >> 16),
                          (uint8_t)(pixels[i] >> 8), (uint8_t)pixels[i]};
        ok = fwrite(rgb, 1, sizeof(rgb), file) == sizeof(rgb);
    }
    fclose(file);
    return ok;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s MEMDUMP.BIN output.ppm\n", argv[0]);
        return 2;
    }
    uint8_t *memory = (uint8_t *)malloc(DOS_VGA_MEMORY_SIZE);
    uint32_t *frame = (uint32_t *)malloc(DOS_VGA_FRAME_SIZE * sizeof(*frame));
    if (!memory || !frame || !read_dump(argv[1], memory)) {
        free(memory); free(frame); return 1;
    }
    bool ok = captive_dos_runtime_render(memory, DOS_VGA_MEMORY_SIZE,
                                         0x0E3F, 0x0824, frame, 320, 200) &&
              write_ppm(argv[2], frame);
    free(memory); free(frame);
    return ok ? 0 : 1;
}
