#include "pl5_decoder.h"
#include <stdlib.h>
#include <string.h>

// Captive DOS game palette (32 colors, ARGB8888)
// Extracted from CaptiveTools.jar by oFF_rus and verified against game output
const uint32_t pl5_default_palette[PL5_COLORS] = {
    0xFF000000, // 0: black
    0xFFB0B0F0, // 1: light blue
    0xFF909090, // 2: gray
    0xFF606060, // 3: dark gray
    0xFF404040, // 4: darker gray
    0xFF202020, // 5: near black
    0xFF50F050, // 6: bright green
    0xFF005000, // 7: dark green
    0xFF600000, // 8: dark red
    0xFFA00000, // 9: red
    0xFFD0C000, // 10: yellow
    0xFFE07070, // 11: pink
    0xFF600060, // 12: dark magenta
    0xFFA000A0, // 13: magenta
    0xFFF000F0, // 14: bright magenta
    0xFFF08000, // 15: orange
    0xFF000000, // 16: black
    0xFFFCFCFC, // 17: white
    0xFF000000, // 18: black
    0xFFB08050, // 19: tan/brown
    0xFF805030, // 20: medium brown
    0xFF604020, // 21: brown
    0xFF503000, // 22: dark brown
    0xFF302020, // 23: very dark brown
    0xFFF00000, // 24: bright red
    0xFFD0C000, // 25: yellow (same as 10)
    0xFFC0C0C0, // 26: silver
    0xFF001070, // 27: navy blue
    0xFF2030A0, // 28: medium blue
    0xFF6060C0, // 29: blue
    0xFF108010, // 30: green
    0xFF30C030, // 31: bright green
};

// PL5 pixel format: custom 5-bit packing
// Every 5 input bytes encode 8 pixels (40 bits -> 8 x 5-bit indices)
// The bit arrangement is NOT a standard MSB/LSB bitstream.
// Byte layout for each 5-byte group -> 8 pixels:
//   pixel[0] = src[0] & 0x1F
//   pixel[1] = src[1] & 0x1F
//   pixel[2] = (src[0] >> 5) & 0x07 | (src[1] >> 3) & 0x18
//   pixel[3] = (src[2] << 1 | (src[1] >> 5) & 1) & 0x1F
//   pixel[4] = src[3] & 0x1F
//   pixel[5] = (src[2] >> 6) & 0x03 | (src[3] >> 3) & 0x1C
//   pixel[6] = src[4] & 0x1F
//   pixel[7] = (src[2] >> 4) & 0x03 | (src[4] >> 3) & 0x1C

bool pl5_decode(const uint8_t *data, size_t size, PL5Image *out) {
    if (!data || !out) return false;
    if (size != PL5_FILE_SIZE) return false;

    memset(out, 0, sizeof(*out));
    out->width = PL5_WIDTH;
    out->height = PL5_HEIGHT;
    out->data_size = PL5_PIXEL_COUNT;

    out->pixel_data = calloc(out->data_size, 1);
    if (!out->pixel_data) return false;

    memcpy(out->palette, pl5_default_palette, sizeof(pl5_default_palette));

    int si = 0, di = 0;
    while (si + 4 < (int)size) {
        uint8_t s0 = data[si];
        uint8_t s1 = data[si + 1];
        uint8_t s2 = data[si + 2];
        uint8_t s3 = data[si + 3];
        uint8_t s4 = data[si + 4];

        out->pixel_data[di + 0] = s0 & 0x1F;
        out->pixel_data[di + 1] = s1 & 0x1F;
        out->pixel_data[di + 2] = ((s0 >> 5) & 0x07) | ((s1 >> 3) & 0x18);
        out->pixel_data[di + 3] = ((s2 << 1) | ((s1 >> 5) & 1)) & 0x1F;
        out->pixel_data[di + 4] = s3 & 0x1F;
        out->pixel_data[di + 5] = ((s2 >> 6) & 0x03) | ((s3 >> 3) & 0x1C);
        out->pixel_data[di + 6] = s4 & 0x1F;
        out->pixel_data[di + 7] = ((s2 >> 4) & 0x03) | ((s4 >> 3) & 0x1C);

        si += 5;
        di += 8;
    }

    return true;
}

void pl5_free(PL5Image *img) {
    if (img && img->pixel_data) {
        free(img->pixel_data);
        img->pixel_data = NULL;
    }
}
