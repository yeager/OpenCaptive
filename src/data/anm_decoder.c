#include "anm_decoder.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define ANM_MAX_FRAMES 4096

// ANM file layout:
//   [0..767]   768-byte VGA palette (256 colors, 6-bit RGB)
//   [768..769]  Big-endian word: offset past commands section
//   [770..cmd_end-1] Commands data
//   [cmd_end..EOF] Frames stored backwards from end of file
//
// Frame storage (read from end of file backwards):
//   Last 2 bytes: big-endian word = size of last frame's packed data
//   Preceding 'size' bytes: packed frame data
//   Repeat for each frame until reaching cmd_end
//
// Frame decode (XOR delta against previous frame buffer):
//   byte != 0: XOR with frame[pos], advance pos
//   byte == 0: read next byte as skip count (0 = end of frame)

static void decode_frame(const uint8_t *packed, int packed_len,
                         uint8_t *frame, int frame_size) {
    int si = 0;
    int pos = 0;
    while (si < packed_len && pos < frame_size) {
        uint8_t cmd = packed[si++];
        if (cmd != 0) {
            frame[pos] ^= cmd;
            pos++;
        } else {
            if (si >= packed_len) break;
            uint8_t skip = packed[si++];
            if (skip == 0) break;
            pos += skip;
        }
    }
}

bool anm_decode(const uint8_t *data, size_t size, ANMAnimation *out) {
    if (!data || !out) return false;
    if (size < ANM_PALETTE_SIZE + 4 || size > (size_t)INT_MAX) return false;

    memset(out, 0, sizeof(*out));
    out->width = ANM_WIDTH;
    out->height = ANM_HEIGHT;

    memcpy(out->vga_palette, data, ANM_PALETTE_SIZE);
    for (int i = 0; i < 256; i++) {
        uint8_t r = (data[i * 3 + 0] & 0x3F) << 2;
        uint8_t g = (data[i * 3 + 1] & 0x3F) << 2;
        uint8_t b = (data[i * 3 + 2] & 0x3F) << 2;
        out->palette[i] = 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }

    int cmd_end = (int)data[768] | ((int)data[769] << 8);
    if (cmd_end < ANM_PALETTE_SIZE + 2 || cmd_end > (int)size) return false;

    int frame_count = 0;
    int pos = (int)size;
    while (pos > cmd_end) {
        int fs_off = pos - 2;
        if (fs_off < cmd_end) break;
        int fsize = (int)data[fs_off] | ((int)data[fs_off + 1] << 8);
        if (fsize < 2 || fs_off - (fsize - 2) < cmd_end) break;
        if (frame_count >= ANM_MAX_FRAMES) return false;
        frame_count++;
        pos = fs_off - (fsize - 2);
    }

    if (frame_count == 0 || pos != cmd_end) return false;

    out->frame_count = frame_count;
    out->frames = calloc(frame_count, sizeof(uint8_t *));
    if (!out->frames) return false;

    int *frame_offsets = calloc(frame_count, sizeof(int));
    int *frame_sizes = calloc(frame_count, sizeof(int));
    if (!frame_offsets || !frame_sizes) {
        free(frame_offsets);
        free(frame_sizes);
        anm_free(out);
        return false;
    }

    pos = (int)size;
    for (int i = frame_count - 1; i >= 0; i--) {
        int fsize = (int)data[pos - 2] | ((int)data[pos - 1] << 8);
        int packed = fsize - 2;
        frame_offsets[i] = pos - 2 - packed;
        frame_sizes[i] = packed;
        pos = frame_offsets[i];
    }

    uint8_t *buf = calloc(ANM_FRAME_PIXELS, 1);
    if (!buf) {
        free(frame_offsets);
        free(frame_sizes);
        anm_free(out);
        return false;
    }

    for (int i = 0; i < frame_count; i++) {
        decode_frame(data + frame_offsets[i], frame_sizes[i],
                     buf, ANM_FRAME_PIXELS);

        out->frames[i] = malloc(ANM_FRAME_PIXELS);
        if (!out->frames[i]) {
            free(buf);
            free(frame_offsets);
            free(frame_sizes);
            anm_free(out);
            return false;
        }
        memcpy(out->frames[i], buf, ANM_FRAME_PIXELS);
    }

    free(buf);
    free(frame_offsets);
    free(frame_sizes);
    return true;
}

void anm_free(ANMAnimation *anim) {
    if (!anim) return;
    if (anim->frames) {
        for (int i = 0; i < anim->frame_count; i++) {
            free(anim->frames[i]);
        }
        free(anim->frames);
        anim->frames = NULL;
    }
    anim->frame_count = 0;
}
