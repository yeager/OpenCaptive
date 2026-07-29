#ifndef LIBERATION_ANIM_H
#define LIBERATION_ANIM_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t depth;
    uint32_t palette[32];
    uint8_t *bitplanes;
    size_t bitplane_size;
} LiberationAnimFrame;

/* Raw SCPT instruction chunk from a verified FORM/ANIM container.  The
 * bytecode is retained verbatim; decoding its opcodes is deliberately a
 * separate step from container parsing. */
typedef struct {
    uint8_t *bytes;
    size_t size;
    size_t first_sequence_size;
    uint16_t first_sequence_records;
} LiberationAnimScript;

typedef struct {
    const uint8_t *bytes;
    size_t offset;
    uint16_t size;
    uint8_t timing_multiplier;
} LiberationAnimScriptRecord;

/* Structural progress from a PACK walk.  It is populated on both success
 * and failure so reverse-engineering tools can report an actual boundary
 * rather than treating a first-frame fallback as full playback support. */
typedef struct {
    size_t records;
    size_t source_offset;
    size_t decoded_size;
} LiberationPackProgress;

/* Decode the PACK chunk used inside Liberation's FORM/ANIM resources.
 *
 * This is the byte-run format implemented by the verified CD32 playback
 * module.  The stream is a sequence of big-endian segment lengths followed
 * by a 14-byte descriptor and byte-run payload; a zero length terminates the
 * sequence.  The caller owns *out_data and must free it with free().
 */
int liberation_pack_decode(const uint8_t *src, size_t src_size,
                           uint8_t **out_data, size_t *out_size);
int liberation_pack_decode_with_progress(const uint8_t *src, size_t src_size,
                                         uint8_t **out_data, size_t *out_size,
                                         LiberationPackProgress *progress);

/* Decode the first complete PACK workspace.  Original FORM/ANIM resources
 * contain one RLE-built work area followed by format-specific trailing data;
 * this API deliberately returns only that verified work area. */
int liberation_pack_decode_workspace(const uint8_t *src, size_t src_size,
                                     uint8_t **out_data, size_t *out_size,
                                     LiberationPackProgress *progress);

/* Decode every PACK record from a FORM/ANIM resource. This strict diagnostic
 * is separate from first-frame decoding, which may use a first-record
 * fallback while later presentation layers remain under investigation. */
int liberation_anim_decode_pack(const uint8_t *form, size_t form_size,
                                uint8_t **out_data, size_t *out_size);

/* Decodes the first complete planar frame from a verified FORM/ANIM stream.
 * The caller owns frame->bitplanes and releases it with
 * liberation_anim_frame_free(). */
int liberation_anim_decode_first_frame(const uint8_t *form, size_t form_size,
                                       LiberationAnimFrame *frame);
void liberation_anim_frame_free(LiberationAnimFrame *frame);
int liberation_anim_extract_script(const uint8_t *form, size_t form_size,
                                   LiberationAnimScript *script);
void liberation_anim_script_free(LiberationAnimScript *script);
int liberation_anim_script_record_at(const LiberationAnimScript *script,
                                     uint16_t index,
                                     LiberationAnimScriptRecord *record);
void liberation_anim_blit(const LiberationAnimFrame *frame, uint32_t *dst,
                          int dst_width, int dst_height, int dst_x, int dst_y);

#endif
