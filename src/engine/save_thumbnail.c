#include "save_thumbnail.h"
#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

/* Trailer format: 4-byte magic + SAVE_THUMB_PIXELS little-endian uint32
 * pixels (0xAARRGGBB), appended after whatever a save_load.c writer put
 * in the file. Self-describing so load_game() can tell an old save
 * (no trailer) apart from a save with a preview, without either format
 * needing a version bump. */
#define SAVE_THUMB_MAGIC 0x424D5443u /* "CTMB" */
#define SAVE_THUMB_BLOCK_SIZE (4 + (long)SAVE_THUMB_PIXELS * 4)

static bool write_u32_le(FILE *f, uint32_t value) {
    uint8_t bytes[4] = {(uint8_t)value, (uint8_t)(value >> 8),
                        (uint8_t)(value >> 16), (uint8_t)(value >> 24)};
    return f && fwrite(bytes, 1, sizeof(bytes), f) == sizeof(bytes);
}

static bool read_u32_le(FILE *f, uint32_t *value) {
    uint8_t bytes[4];
    if (!f || !value || fread(bytes, 1, sizeof(bytes), f) != sizeof(bytes)) return false;
    *value = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
             ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
    return true;
}

bool save_game_write_thumbnail(const char *path, const uint32_t *framebuffer,
                               int fb_width, int fb_height) {
    if (!path || !framebuffer || fb_width <= 0 || fb_height <= 0) return false;

    FILE *f = fopen(path, "r+b");
    if (!f) return false;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
    long file_end = ftell(f);
    if (file_end < 0) { fclose(f); return false; }

    /* Replace-not-append: drop a previously written trailer first so
     * repeated saves do not grow the file by one trailer every time. */
    if (file_end >= SAVE_THUMB_BLOCK_SIZE) {
        long maybe_trailer = file_end - SAVE_THUMB_BLOCK_SIZE;
        uint32_t existing_magic = 0;
        if (fseek(f, maybe_trailer, SEEK_SET) == 0 &&
            read_u32_le(f, &existing_magic) && existing_magic == SAVE_THUMB_MAGIC) {
            file_end = maybe_trailer;
        }
    }

    uint32_t pixels[SAVE_THUMB_PIXELS];
    for (int ty = 0; ty < SAVE_THUMB_H; ty++) {
        int sy = ty * fb_height / SAVE_THUMB_H;
        if (sy >= fb_height) sy = fb_height - 1;
        for (int tx = 0; tx < SAVE_THUMB_W; tx++) {
            int sx = tx * fb_width / SAVE_THUMB_W;
            if (sx >= fb_width) sx = fb_width - 1;
            pixels[ty * SAVE_THUMB_W + tx] = framebuffer[sy * fb_width + sx];
        }
    }

#ifdef _WIN32
    if (_chsize(_fileno(f), file_end) != 0) { fclose(f); return false; }
#else
    if (ftruncate(fileno(f), file_end) != 0) { fclose(f); return false; }
#endif
    bool ok = fseek(f, file_end, SEEK_SET) == 0 &&
              write_u32_le(f, SAVE_THUMB_MAGIC);
    for (int i = 0; ok && i < SAVE_THUMB_PIXELS; i++)
        ok = write_u32_le(f, pixels[i]);
    if (fclose(f) != 0) ok = false;
    return ok;
}

bool save_read_thumbnail(const char *path, uint32_t *out_pixels) {
    if (!path || !out_pixels) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    bool ok = false;
    if (fseek(f, 0, SEEK_END) == 0) {
        long file_end = ftell(f);
        if (file_end >= SAVE_THUMB_BLOCK_SIZE) {
            long trailer = file_end - SAVE_THUMB_BLOCK_SIZE;
            uint32_t magic = 0;
            if (fseek(f, trailer, SEEK_SET) == 0 &&
                read_u32_le(f, &magic) && magic == SAVE_THUMB_MAGIC) {
                ok = true;
                for (int i = 0; ok && i < SAVE_THUMB_PIXELS; i++)
                    ok = read_u32_le(f, &out_pixels[i]);
            }
        }
    }
    fclose(f);
    return ok;
}
