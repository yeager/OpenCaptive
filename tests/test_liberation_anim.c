#include "liberation_anim.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void write_be32(uint8_t *dst, uint32_t value) {
    dst[0] = (uint8_t)(value >> 24);
    dst[1] = (uint8_t)(value >> 16);
    dst[2] = (uint8_t)(value >> 8);
    dst[3] = (uint8_t)value;
}

static void write_be16(uint8_t *dst, uint16_t value) {
    dst[0] = (uint8_t)(value >> 8);
    dst[1] = (uint8_t)value;
}

int main(void) {
    /* One 20-byte segment: 14-byte descriptor + six RLE-decoded bytes. */
    const uint8_t packed[] = {
        0, 0, 1, 0, /* original work-buffer allocation */
        0, 0, 0, 20,
        0, 0, 0, 6, 0, 1, 0, 1, 1, 0, 0, 0, 0, 6,
        2, 'A', 'B', 'C', 0x80, 'Z',
        0, /* word alignment */
        0, 0, 0, 0,
    };
    uint8_t *decoded = NULL;
    size_t decoded_size = 0;
    LiberationPackProgress progress = {0};
    assert(liberation_pack_decode_with_progress(packed, sizeof(packed),
                                                &decoded, &decoded_size,
                                                &progress));
    assert(decoded_size == 20U);
    assert(progress.records == 1U && progress.decoded_size == 20U);
    assert(memcmp(decoded, packed + 8, 14U) == 0);
    assert(memcmp(decoded + 14, "ABCZZZ", 6U) == 0);
    free(decoded);

    assert(liberation_pack_decode_workspace(packed, sizeof(packed), &decoded,
                                            &decoded_size, &progress));
    assert(decoded_size == 20U && progress.records == 1U);
    free(decoded);

    const uint8_t malformed[] = {0, 0, 1, 0, 0, 0, 0, 14, 0};
    assert(!liberation_pack_decode_with_progress(malformed, sizeof(malformed),
                                                 &decoded, &decoded_size,
                                                 &progress));

    uint8_t oversized[8] = {0, 0, 1, 0, 0x08, 0, 0, 0};
    assert(!liberation_pack_decode_with_progress(oversized, sizeof(oversized),
                                                 &decoded, &decoded_size,
                                                 &progress));

    uint8_t form[12 + 8 + 64 + 8 + 28 + 8 + 8] = {0};
    memcpy(form, "FORM", 4);
    write_be32(form + 4, sizeof(form) - 8U);
    memcpy(form + 8, "ANIM", 4);
    memcpy(form + 12, "PALL", 4);
    write_be32(form + 16, 64);
    write_be16(form + 20 + 2, 0x0f00); /* palette index 1: red */
    memcpy(form + 84, "PACK", 4);
    write_be32(form + 88, 28);
    write_be32(form + 92, 32); /* work buffer */
    write_be32(form + 96, 15); /* descriptor + one raw byte */
    write_be16(form + 100, 1); /* one byte = eight pixels */
    write_be16(form + 102, 1);
    form[104] = 1;
    form[105] = 1;
    write_be32(form + 106, 1);
    form[114] = 0;              /* one literal byte */
    form[115] = 0xa0;           /* palette indices 1,0,1,0... */
    /* Four zero bytes after the first record terminate the PACK stream. */
    memcpy(form + 120, "SCPT", 4);
    write_be32(form + 124, 8);
    write_be16(form + 128, 6);
    form[130] = 0x50;
    form[131] = 0xfd;
    form[132] = 0xff;
    form[133] = 0x00;
    /* Remaining two bytes are the zero-length sequence terminator. */

    LiberationAnimFrame frame = {0};
    assert(liberation_anim_decode_first_frame(form, sizeof(form), &frame));
    assert(frame.width == 8 && frame.height == 1 && frame.depth == 1);
    uint32_t pixels[8] = {0};
    liberation_anim_blit(&frame, pixels, 8, 1, 0, 0);
    assert(pixels[0] == 0xffff0000U && pixels[1] == 0xff000000U);
    assert(pixels[2] == 0xffff0000U && pixels[3] == 0xff000000U);
    liberation_anim_frame_free(&frame);

    /* A non-byte-aligned public frame must use a partial final byte per row,
       rather than reading the next row or past the allocation. */
    uint8_t partial_plane[4] = {0x80, 0x00, 0x00, 0x80};
    memset(&frame, 0, sizeof(frame));
    frame.width = 9;
    frame.height = 2;
    frame.depth = 1;
    frame.palette[0] = 0xff000000U;
    frame.palette[1] = 0xffffffffU;
    frame.bitplanes = partial_plane;
    frame.bitplane_size = sizeof(partial_plane);
    uint32_t partial_pixels[18] = {0};
    liberation_anim_blit(&frame, partial_pixels, 9, 2, 0, 0);
    assert(partial_pixels[0] == 0xffffffffU);
    assert(partial_pixels[8] == 0xff000000U);
    assert(partial_pixels[9 + 8] == 0xffffffffU);

    /* The manually constructed frame owns stack storage; clear it before
       passing the object to an API that frees owned decoder output. */
    memset(&frame, 0, sizeof(frame));
    uint8_t *all_pack = NULL;
    size_t all_pack_size = 0;
    assert(liberation_anim_decode_pack(form, sizeof(form), &all_pack,
                                       &all_pack_size));
    assert(all_pack_size == 15U && all_pack[14] == 0xa0U);
    free(all_pack);
    LiberationAnimScript script = {0};
    assert(liberation_anim_extract_script(form, sizeof(form), &script));
    assert(script.size == 8U && script.first_sequence_records == 1U &&
           script.first_sequence_size == 8U && script.bytes[2] == 0x50 &&
           script.bytes[3] == 0xfd && script.bytes[4] == 0xff);
    LiberationAnimScriptRecord record = {0};
    assert(liberation_anim_script_record_at(&script, 0, &record));
    assert(record.offset == 0U && record.size == 6U &&
           record.timing_multiplier == 0x50 && record.bytes[2] == 0x50);
    assert(!liberation_anim_script_record_at(&script, 1, &record));
    liberation_anim_script_free(&script);

    /* Chunks beyond the declared FORM extent must not be considered part of
       the animation merely because the backing buffer contains more bytes. */
    write_be32(form + 4, 76U); /* FORM ends immediately after PALL */
    assert(!liberation_anim_decode_first_frame(form, sizeof(form), &frame));
    assert(!liberation_anim_decode_pack(form, sizeof(form), &all_pack,
                                        &all_pack_size));
    assert(!liberation_anim_extract_script(form, sizeof(form), &script));
    return 0;
}
