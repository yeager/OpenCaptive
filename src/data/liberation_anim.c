#include "liberation_anim.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define PACK_DESCRIPTOR_SIZE 14U

static uint32_t read_be32(const uint8_t *data) {
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | data[3];
}

static uint16_t read_be16(const uint8_t *data) {
    return (uint16_t)((data[0] << 8) | data[1]);
}

static int grow_output(uint8_t **data, size_t *capacity, size_t required) {
    if (required <= *capacity) return 1;
    size_t next = *capacity ? *capacity : 4096U;
    while (next < required) {
        if (next > SIZE_MAX / 2U) {
            next = required;
            break;
        }
        next *= 2U;
    }
    uint8_t *grown = realloc(*data, next);
    if (!grown) return 0;
    *data = grown;
    *capacity = next;
    return 1;
}

static int pack_decode(const uint8_t *src, size_t src_size,
                       uint8_t **out_data, size_t *out_size,
                       LiberationPackProgress *progress,
                       int stop_after_first_workspace) {
    if (!out_data || !out_size || (!src && src_size != 0U)) return 0;
    *out_data = NULL;
    *out_size = 0;
    if (progress) memset(progress, 0, sizeof(*progress));

    uint8_t *output = NULL;
    size_t capacity = 0;
    /* The first longword reserves the original player's work buffer.  It is
       deliberately not part of the first RLE segment. */
    if (src_size < 8U || read_be32(src) == 0U) return 0;
    size_t in_pos = 4U;
    size_t out_pos = 0;
    int terminated = 0;

    while (in_pos + 4U <= src_size) {
        uint32_t segment_size = read_be32(src + in_pos);
        in_pos += 4U;
        if (segment_size == 0U) {
            terminated = 1;
            break;
        }
        if (segment_size < PACK_DESCRIPTOR_SIZE ||
            segment_size > SIZE_MAX - out_pos ||
            in_pos + PACK_DESCRIPTOR_SIZE > src_size ||
            !grow_output(&output, &capacity, out_pos + segment_size)) {
            free(output);
            if (progress) {
                progress->source_offset = in_pos;
                progress->decoded_size = out_pos;
            }
            return 0;
        }

        memcpy(output + out_pos, src + in_pos, PACK_DESCRIPTOR_SIZE);
        in_pos += PACK_DESCRIPTOR_SIZE;
        size_t segment_end = out_pos + segment_size;
        out_pos += PACK_DESCRIPTOR_SIZE;

        while (out_pos < segment_end) {
            if (in_pos >= src_size) {
                free(output);
                if (progress) {
                    progress->source_offset = in_pos;
                    progress->decoded_size = out_pos;
                }
                return 0;
            }
            uint8_t control = src[in_pos++];
            size_t count;
            if (control & 0x80U) {
                /* The CD32 player subtracts $7e and then executes DBF;
                   consequently the encoded count is ($80-$7e)+1 = 3 at
                   minimum. */
                count = (size_t)(control - 0x7eU) + 1U;
                if (in_pos >= src_size) {
                    free(output);
                    if (progress) {
                        progress->source_offset = in_pos;
                        progress->decoded_size = out_pos;
                    }
                    return 0;
                }
                /* The final encoded run may include a stream pad byte after
                   the declared record payload.  The original player advances
                   the input stream but its destination record is bounded by
                   the declared size, so mirror that boundary here. */
                size_t copy = count;
                if (copy > segment_end - out_pos) copy = segment_end - out_pos;
                memset(output + out_pos, src[in_pos++], copy);
                out_pos += copy;
            } else {
                count = (size_t)control + 1U;
                if (count > src_size - in_pos) {
                    free(output);
                    if (progress) {
                        progress->source_offset = in_pos;
                        progress->decoded_size = out_pos;
                    }
                    return 0;
                }
                size_t copy = count;
                if (copy > segment_end - out_pos) copy = segment_end - out_pos;
                memcpy(output + out_pos, src + in_pos, copy);
                in_pos += count;
                out_pos += copy;
            }
        }
        if (in_pos & 1U) in_pos++;
        if (in_pos > src_size) {
            free(output);
            if (progress) {
                progress->source_offset = in_pos;
                progress->decoded_size = out_pos;
            }
            return 0;
        }
        if (progress) {
            progress->records++;
            progress->source_offset = in_pos;
            progress->decoded_size = out_pos;
        }
        if (stop_after_first_workspace) {
            *out_data = output;
            *out_size = out_pos;
            return 1;
        }
    }

    /* The CD32 player tests a four-byte zero terminator in its cleared work
       allocation. A FORM chunk may retain one to three zero pad bytes before
       that probe crosses the chunk boundary, so recognise only that exact
       on-disk representation. */
    if (!terminated && in_pos == src_size) {
        terminated = 1;
    } else if (!terminated && src_size - in_pos < 4U) {
        int zero_padding = 1;
        for (size_t i = in_pos; i < src_size; ++i) {
            if (src[i] != 0U) zero_padding = 0;
        }
        if (zero_padding) terminated = 1;
    }
    if (!terminated) {
        free(output);
        if (progress) {
            progress->source_offset = in_pos;
            progress->decoded_size = out_pos;
        }
        return 0;
    }
    *out_data = output;
    *out_size = out_pos;
    return 1;
}

int liberation_pack_decode_with_progress(const uint8_t *src, size_t src_size,
                                         uint8_t **out_data, size_t *out_size,
                                         LiberationPackProgress *progress) {
    return pack_decode(src, src_size, out_data, out_size, progress, 0);
}

int liberation_pack_decode(const uint8_t *src, size_t src_size,
                           uint8_t **out_data, size_t *out_size) {
    return liberation_pack_decode_with_progress(src, src_size, out_data, out_size,
                                                NULL);
}

int liberation_pack_decode_workspace(const uint8_t *src, size_t src_size,
                                     uint8_t **out_data, size_t *out_size,
                                     LiberationPackProgress *progress) {
    return pack_decode(src, src_size, out_data, out_size, progress, 1);
}

int liberation_anim_decode_pack(const uint8_t *form, size_t form_size,
                                uint8_t **out_data, size_t *out_size) {
    if (!out_data || !out_size || !form || form_size < 12U ||
        memcmp(form, "FORM", 4) != 0 || memcmp(form + 8U, "ANIM", 4) != 0) {
        return 0;
    }
    *out_data = NULL;
    *out_size = 0;
    size_t pos = 12U;
    while (pos + 8U <= form_size) {
        const uint8_t *chunk = form + pos;
        uint32_t chunk_size = read_be32(chunk + 4U);
        pos += 8U;
        if (chunk_size > form_size - pos) return 0;
        if (memcmp(chunk, "PACK", 4) == 0) {
            return liberation_pack_decode(form + pos, chunk_size, out_data, out_size);
        }
        pos += chunk_size + (chunk_size & 1U);
    }
    return 0;
}

void liberation_anim_frame_free(LiberationAnimFrame *frame) {
    if (!frame) return;
    free(frame->bitplanes);
    memset(frame, 0, sizeof(*frame));
}

void liberation_anim_script_free(LiberationAnimScript *script) {
    if (!script) return;
    free(script->bytes);
    memset(script, 0, sizeof(*script));
}

int liberation_anim_extract_script(const uint8_t *form, size_t form_size,
                                   LiberationAnimScript *script) {
    if (!form || !script || form_size < 12U || memcmp(form, "FORM", 4) != 0 ||
        memcmp(form + 8, "ANIM", 4) != 0) return 0;
    liberation_anim_script_free(script);
    size_t pos = 12U;
    while (pos + 8U <= form_size) {
        const uint8_t *chunk = form + pos;
        uint32_t chunk_size = read_be32(chunk + 4U);
        pos += 8U;
        if (chunk_size > form_size - pos) return 0;
        if (memcmp(chunk, "SCPT", 4) == 0) {
            if (!chunk_size) return 0;
            script->bytes = malloc(chunk_size);
            if (!script->bytes) return 0;
            memcpy(script->bytes, form + pos, chunk_size);
            script->size = chunk_size;
            /* The verified player advances through SCPT with
               `next = current + be16(current)` and stops on a zero word.
               Validate the first sequence exactly as that traversal does;
               later sequences remain raw until their opcode dispatch is
               recovered. */
            size_t script_pos = 0;
            while (script_pos + 2U <= script->size) {
                uint16_t record_size = read_be16(script->bytes + script_pos);
                if (record_size == 0U) {
                    script->first_sequence_size = script_pos + 2U;
                    return 1;
                }
                if (record_size < 2U || record_size > script->size - script_pos) {
                    liberation_anim_script_free(script);
                    return 0;
                }
                if (script->first_sequence_records == UINT16_MAX) {
                    liberation_anim_script_free(script);
                    return 0;
                }
                script->first_sequence_records++;
                script_pos += record_size;
            }
            liberation_anim_script_free(script);
            return 0;
        }
        pos += chunk_size + (chunk_size & 1U);
    }
    return 0;
}

int liberation_anim_script_record_at(const LiberationAnimScript *script,
                                     uint16_t index,
                                     LiberationAnimScriptRecord *record) {
    if (!script || !script->bytes || !record ||
        index >= script->first_sequence_records) return 0;
    size_t pos = 0;
    for (uint16_t current = 0; current <= index; ++current) {
        if (pos + 2U > script->first_sequence_size) return 0;
        uint16_t size = read_be16(script->bytes + pos);
        if (size < 2U || size > script->first_sequence_size - pos) return 0;
        if (current == index) {
            record->bytes = script->bytes + pos;
            record->offset = pos;
            record->size = size;
            record->timing_multiplier = record->bytes[2];
            return 1;
        }
        pos += size;
    }
    return 0;
}

static int decode_first_pack_frame(const uint8_t *pack, size_t pack_size,
                                   LiberationAnimFrame *frame) {
    /* Prefer a complete PACK walk when every record is structurally
       decodable.  Keeping the proven first-record path below as a fallback
       allows partially understood later layers to remain non-blocking. */
    uint8_t *all_records = NULL;
    size_t all_records_size = 0;
    if (liberation_pack_decode_workspace(pack, pack_size, &all_records,
                                         &all_records_size, NULL) &&
        all_records_size >= PACK_DESCRIPTOR_SIZE) {
        uint16_t width_bytes = read_be16(all_records);
        uint16_t height = read_be16(all_records + 2U);
        uint8_t depth = all_records[4U];
        uint32_t raw_size = read_be32(all_records + 6U);
        if (width_bytes != 0U && height != 0U && depth != 0U && depth <= 8U &&
            raw_size == (uint32_t)width_bytes * height * depth &&
            raw_size <= all_records_size - PACK_DESCRIPTOR_SIZE) {
            uint8_t *raw = malloc(raw_size);
            if (raw) {
                memcpy(raw, all_records + PACK_DESCRIPTOR_SIZE, raw_size);
                free(all_records);
                frame->width = (uint16_t)(width_bytes * 8U);
                frame->height = height;
                frame->depth = depth;
                frame->bitplanes = raw;
                frame->bitplane_size = raw_size;
                return 1;
            }
        }
    }
    free(all_records);

    /* The first longword is the original player's work-buffer reservation.
       The first record starts immediately after it.  A full PACK stream can
       contain in-place layers after this record; they need the game's object
       interpreter and are deliberately not guessed here. */
    if (pack_size < 4U + 4U + PACK_DESCRIPTOR_SIZE) return 0;
    size_t pos = 4U;
    uint32_t record_size = read_be32(pack + pos);
    pos += 4U;
    if (record_size < PACK_DESCRIPTOR_SIZE ||
        pos + PACK_DESCRIPTOR_SIZE > pack_size) return 0;

    uint16_t width_bytes = read_be16(pack + pos);
    uint16_t height = read_be16(pack + pos + 2U);
    uint8_t depth = pack[pos + 4U];
    uint32_t raw_size = read_be32(pack + pos + 6U);
    if (width_bytes == 0U || height == 0U || depth == 0U || depth > 8U ||
        raw_size != (uint32_t)width_bytes * height * depth ||
        record_size != raw_size + PACK_DESCRIPTOR_SIZE) return 0;
    pos += PACK_DESCRIPTOR_SIZE;

    uint8_t *raw = malloc(raw_size);
    if (!raw) return 0;
    size_t written = 0;
    while (written < raw_size && pos < pack_size) {
        uint8_t control = pack[pos++];
        size_t count;
        if (control & 0x80U) {
            count = (size_t)(control - 0x7eU) + 1U;
            if (pos >= pack_size) { free(raw); return 0; }
            uint8_t value = pack[pos++];
            size_t copy = count;
            if (copy > raw_size - written) copy = raw_size - written;
            memset(raw + written, value, copy);
            written += copy;
        } else {
            count = (size_t)control + 1U;
            if (count > pack_size - pos) { free(raw); return 0; }
            size_t copy = count;
            if (copy > raw_size - written) copy = raw_size - written;
            memcpy(raw + written, pack + pos, copy);
            written += copy;
            pos += count;
        }
    }
    if (written != raw_size) { free(raw); return 0; }

    frame->width = (uint16_t)(width_bytes * 8U);
    frame->height = height;
    frame->depth = depth;
    frame->bitplanes = raw;
    frame->bitplane_size = raw_size;
    return 1;
}

int liberation_anim_decode_first_frame(const uint8_t *form, size_t form_size,
                                       LiberationAnimFrame *frame) {
    if (!form || !frame || form_size < 12U || memcmp(form, "FORM", 4) != 0 ||
        memcmp(form + 8, "ANIM", 4) != 0) return 0;
    liberation_anim_frame_free(frame);

    const uint8_t *pack = NULL;
    size_t pack_size = 0;
    int palette_found = 0;
    size_t pos = 12U;
    while (pos + 8U <= form_size) {
        const uint8_t *chunk = form + pos;
        uint32_t chunk_size = read_be32(chunk + 4U);
        pos += 8U;
        if (chunk_size > form_size - pos) return 0;
        if (memcmp(chunk, "PALL", 4) == 0 && chunk_size >= 64U) {
            for (size_t i = 0; i < 32U; i++) {
                uint16_t color = read_be16(form + pos + i * 2U);
                uint8_t r = (uint8_t)(((color >> 8) & 0x0fU) * 17U);
                uint8_t g = (uint8_t)(((color >> 4) & 0x0fU) * 17U);
                uint8_t b = (uint8_t)((color & 0x0fU) * 17U);
                frame->palette[i] = 0xff000000U | ((uint32_t)r << 16) |
                                    ((uint32_t)g << 8) | b;
            }
            palette_found = 1;
        } else if (memcmp(chunk, "PACK", 4) == 0) {
            pack = form + pos;
            pack_size = chunk_size;
        }
        pos += chunk_size + (chunk_size & 1U);
    }
    if (!palette_found || !pack || !decode_first_pack_frame(pack, pack_size, frame)) {
        liberation_anim_frame_free(frame);
        return 0;
    }
    return 1;
}

void liberation_anim_blit(const LiberationAnimFrame *frame, uint32_t *dst,
                          int dst_width, int dst_height, int dst_x, int dst_y) {
    if (!frame || !frame->bitplanes || !dst || frame->depth == 0U) return;
    size_t row_bytes = frame->width / 8U;
    for (uint16_t y = 0; y < frame->height; y++) {
        int out_y = dst_y + y;
        if (out_y < 0 || out_y >= dst_height) continue;
        for (uint16_t x = 0; x < frame->width; x++) {
            int out_x = dst_x + x;
            if (out_x < 0 || out_x >= dst_width) continue;
            size_t byte_index = x / 8U;
            uint8_t bit = (uint8_t)(7U - (x & 7U));
            uint8_t index = 0;
            for (uint8_t plane = 0; plane < frame->depth; plane++) {
                size_t offset = ((size_t)plane * frame->height + y) * row_bytes + byte_index;
                index |= (uint8_t)(((frame->bitplanes[offset] >> bit) & 1U) << plane);
            }
            dst[(size_t)out_y * dst_width + out_x] = frame->palette[index & 31U];
        }
    }
}
