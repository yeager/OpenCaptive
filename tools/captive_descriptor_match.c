#include "captive_dos_descriptor.h"
#include "dos_vga_reference.h"
#include "pl5_decoder.h"
#include "sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    SCREEN_WIDTH = 320,
    SCREEN_HEIGHT = 200,
    VIEW_X = 32,
    VIEW_Y = 55,
    VIEW_WIDTH = 144,
    VIEW_HEIGHT = 112,
    DESCRIPTOR_SEGMENT = 0x2942,
    SOURCE_BANK_TABLE_SEGMENT = 0x0824,
    PACKED_SOURCE_STRIDE = 200,
};

static const char captured_renderer_sha256[] =
    "9003c4a8818cb97f8299ac90cfe51e90e535ab9a725545526fe75f14ddb8dd7e";

typedef struct {
    uint8_t *pixels;
    int width;
    int height;
    int opaque_count;
} DescriptorPanel;

static uint8_t *read_file(const char *path, size_t *out_size) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return NULL; }
    long length = ftell(file);
    if (length < 0 || fseek(file, 0, SEEK_SET) != 0) { fclose(file); return NULL; }
    uint8_t *bytes = malloc((size_t)length);
    if (!bytes || fread(bytes, 1, (size_t)length, file) != (size_t)length) {
        free(bytes);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *out_size = (size_t)length;
    return bytes;
}

static bool read_ppm(const char *path, uint32_t *pixels) {
    FILE *file = fopen(path, "rb");
    char magic[3] = {0};
    int width = 0, height = 0, maximum = 0;
    if (!file || fscanf(file, "%2s%d%d%d", magic, &width, &height, &maximum) != 4 ||
        strcmp(magic, "P6") != 0 || width != SCREEN_WIDTH ||
        height != SCREEN_HEIGHT || maximum != 255 || fgetc(file) == EOF) {
        if (file) fclose(file);
        return false;
    }
    for (size_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
        uint8_t rgb[3];
        if (fread(rgb, 1, sizeof(rgb), file) != sizeof(rgb)) {
            fclose(file);
            return false;
        }
        pixels[i] = UINT32_C(0xff000000) | ((uint32_t)rgb[0] << 16) |
                    ((uint32_t)rgb[1] << 8) | rgb[2];
    }
    return fclose(file) == 0;
}

static void unpack_group(const uint8_t source[5], uint8_t out[8]) {
    uint8_t s0 = source[0], s1 = source[1], s2 = source[2];
    uint8_t s3 = source[3], s4 = source[4];
    out[0] = s0 & 0x1f;
    out[1] = s1 & 0x1f;
    out[2] = ((s0 >> 5) & 0x07) | ((s1 >> 3) & 0x18);
    out[3] = ((s2 << 1) | ((s1 >> 5) & 0x01)) & 0x1f;
    out[4] = s3 & 0x1f;
    out[5] = ((s2 >> 6) & 0x03) | ((s3 >> 3) & 0x1c);
    out[6] = s4 & 0x1f;
    out[7] = ((s2 >> 4) & 0x03) | ((s4 >> 3) & 0x1c);
}

static bool panel_decode(const uint8_t *memory, size_t memory_size,
                         const CaptiveDosDescriptor *descriptor,
                         DescriptorPanel *out) {
    if (!memory || !descriptor || !out || descriptor->width_bytes == 0 ||
        descriptor->height == 0) return false;
    int width = descriptor->width_bytes * 8;
    int height = descriptor->height;
    size_t source_base = (size_t)descriptor->source_segment << 4;
    size_t source_last = source_base + descriptor->source_offset +
        (size_t)(height - 1) * PACKED_SOURCE_STRIDE +
        (size_t)descriptor->width_bytes * 5;
    if (source_last > memory_size) return false;
    uint8_t *pixels = malloc((size_t)width * (size_t)height);
    if (!pixels) return false;
    int opaque = 0;
    for (int y = 0; y < height; ++y) {
        const uint8_t *row = memory + source_base + descriptor->source_offset +
                             (size_t)y * PACKED_SOURCE_STRIDE;
        for (int group = 0; group < descriptor->width_bytes; ++group) {
            uint8_t decoded[8];
            unpack_group(row + group * 5, decoded);
            for (int x = 0; x < 8; ++x) {
                uint8_t pixel = decoded[x];
                pixels[y * width + group * 8 + x] = pixel;
                if (!(descriptor->flags & 0x04) || pixel != 0) ++opaque;
            }
        }
    }
    out->pixels = pixels;
    out->width = width;
    out->height = height;
    out->opaque_count = opaque;
    return opaque > 0;
}

static int panel_agreement(const DescriptorPanel *panel,
                           const CaptiveDosDescriptor *descriptor,
                           const uint32_t *frame, int target_x, int target_y) {
    int matched = 0;
    for (int y = 0; y < panel->height; ++y) for (int x = 0; x < panel->width; ++x) {
        int source_x = (descriptor->flags & 0x01) ? panel->width - 1 - x : x;
        uint8_t index = panel->pixels[y * panel->width + source_x];
        if ((descriptor->flags & 0x04) && index == 0) continue;
        if (frame[(target_y + y) * SCREEN_WIDTH + target_x + x] ==
            pl5_default_palette[index]) ++matched;
    }
    return matched;
}

static bool find_best(const DescriptorPanel *panel,
                      const CaptiveDosDescriptor *descriptor,
                      const uint32_t *frame, int *out_x, int *out_y,
                      int *out_agreement) {
    if (!panel || panel->width > VIEW_WIDTH || panel->height > VIEW_HEIGHT) return false;
    int best = -1, best_x = 0, best_y = 0;
    for (int y = VIEW_Y; y <= VIEW_Y + VIEW_HEIGHT - panel->height; ++y)
        for (int x = VIEW_X; x <= VIEW_X + VIEW_WIDTH - panel->width; ++x) {
            int agreement = panel_agreement(panel, descriptor, frame, x, y);
            if (agreement > best) {
                best = agreement;
                best_x = x;
                best_y = y;
            }
        }
    *out_x = best_x;
    *out_y = best_y;
    *out_agreement = best;
    return best >= 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <hash-verified-dos-memory> <vga-frame.ppm>\n", argv[0]);
        return 2;
    }
    size_t memory_size = 0;
    uint8_t *memory = read_file(argv[1], &memory_size);
    uint8_t digest[32];
    if (!memory || memory_size != DOS_VGA_MEMORY_SIZE ||
        (sha256_digest(memory, memory_size, digest),
         !sha256_matches_hex(digest, captured_renderer_sha256))) {
        fprintf(stderr, "memory image is not the hash-verified Captive renderer fixture\n");
        free(memory);
        return 1;
    }
    uint32_t frame[SCREEN_WIDTH * SCREEN_HEIGHT];
    uint32_t original[SCREEN_WIDTH * SCREEN_HEIGHT];
    if (!read_ppm(argv[2], frame) ||
        !dos_vga_reference_decode(memory, memory_size, original,
                                  SCREEN_WIDTH * SCREEN_HEIGHT) ||
        memcmp(frame, original, sizeof(frame)) != 0) {
        fprintf(stderr, "frame does not match the supplied hash-verified memory fixture\n");
        free(memory);
        return 1;
    }

    static const struct { uint16_t id; int x, y, agreement; } expected[] = {
        {0x010, 32, 64, 1511},
        {0x01a, 48, 64, 1356},
    };
    bool ok = true;
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i) {
        CaptiveDosDescriptor descriptor = {0};
        DescriptorPanel panel = {0};
        int x = 0, y = 0, agreement = 0;
        if (!captive_dos_descriptor_read(memory, memory_size, DESCRIPTOR_SEGMENT,
                                         SOURCE_BANK_TABLE_SEGMENT,
                                         expected[i].id, &descriptor) ||
            !panel_decode(memory, memory_size, &descriptor, &panel) ||
            !find_best(&panel, &descriptor, frame, &x, &y, &agreement)) {
            fprintf(stderr, "unable to decode descriptor %03x\n", expected[i].id);
            free(panel.pixels);
            ok = false;
            continue;
        }
        printf("id=%03x source-bank=%u source=%04x size=%dx%d flags=%02x "
               "target=%d,%d opaque=%d agreement=%d/%d\n",
               expected[i].id, descriptor.source_bank, descriptor.source_offset,
               panel.width, panel.height, descriptor.flags, x, y,
               panel.opaque_count, agreement, panel.opaque_count);
        if (x != expected[i].x || y != expected[i].y ||
            agreement != expected[i].agreement) ok = false;
        free(panel.pixels);
    }
    free(memory);
    return ok ? 0 : 1;
}
