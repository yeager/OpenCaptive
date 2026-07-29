#include "liberation_anim.h"
#include "liberation_data.h"
#include "rnc_decoder.h"
#include "sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This is the verified presentation bundle, not an ISO filename. */
static const char presentation_bundle_sha256[] =
    "1d3a335d254c0eae919a712dd73bd41b24ed897bf145ed118ccf2277baa7a35f";

static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static void hex_digest(const uint8_t digest[32], char hex[65]) {
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < 32; ++i) {
        hex[i * 2] = digits[digest[i] >> 4];
        hex[i * 2 + 1] = digits[digest[i] & 15U];
    }
    hex[64] = '\0';
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <data-dir>\n", argv[0]);
        return 2;
    }

    DataVFS vfs;
    LiberationData data = {0};
    if (!vfs_init(&vfs, argv[1]) || !liberation_data_open(&data, &vfs)) {
        fprintf(stderr, "verified Liberation CD32 data was not found\n");
        liberation_data_close(&data);
        vfs_free(&vfs);
        return 1;
    }

    size_t bundle_size = 0;
    uint8_t *bundle = iso_read_file_sha256(&data.iso, presentation_bundle_sha256,
                                            &bundle_size);
    if (!bundle) {
        fprintf(stderr, "verified presentation bundle was not found\n");
        liberation_data_close(&data);
        vfs_free(&vfs);
        return 1;
    }

    for (size_t offset = 0; offset + 12U <= bundle_size; ++offset) {
        const uint8_t *rnc = bundle + offset;
        if (memcmp(rnc, "RNC\2", 4) != 0) continue;
        uint32_t raw_size = read_be32(rnc + 4U);
        uint32_t packed_size = read_be32(rnc + 8U);
        if (!raw_size || raw_size > 16U * 1024U * 1024U ||
            packed_size > bundle_size - offset - 12U) continue;

        uint8_t *form = malloc(raw_size);
        if (!form) break;
        int decoded = rnc_decode(rnc, (int)(packed_size + 12U), form, (int)raw_size);
        if (decoded != (int)raw_size || raw_size < 12U ||
            memcmp(form, "FORM", 4) != 0 || memcmp(form + 8, "ANIM", 4) != 0) {
            free(form);
            continue;
        }

        uint8_t digest[32];
        char hex[65];
        sha256_digest(form, raw_size, digest);
        hex_digest(digest, hex);
        LiberationAnimFrame frame = {0};
        LiberationAnimScript script = {0};
        int first_frame = liberation_anim_decode_first_frame(form, raw_size, &frame);
        int has_script = liberation_anim_extract_script(form, raw_size, &script);
        uint8_t *all_pack_records = NULL;
        size_t all_pack_records_size = 0;
        LiberationPackProgress pack_progress = {0};
        /* Keep a strict diagnostic here instead of deriving success from the
           first-frame API, which deliberately has a narrow fallback. */
        const uint8_t *pack = NULL;
        size_t pack_size = 0;
        for (size_t chunk_pos = 12U; chunk_pos + 8U <= raw_size;) {
            uint32_t chunk_size = read_be32(form + chunk_pos + 4U);
            chunk_pos += 8U;
            if (chunk_size > raw_size - chunk_pos) break;
            if (memcmp(form + chunk_pos - 8U, "PACK", 4) == 0) {
                pack = form + chunk_pos;
                pack_size = chunk_size;
                break;
            }
            chunk_pos += chunk_size + (chunk_size & 1U);
        }
        int full_pack = pack && liberation_pack_decode_with_progress(
            pack, pack_size, &all_pack_records, &all_pack_records_size,
            &pack_progress);
        uint8_t *workspace = NULL;
        size_t workspace_size = 0;
        LiberationPackProgress workspace_progress = {0};
        int has_workspace = pack && liberation_pack_decode_workspace(
            pack, pack_size, &workspace, &workspace_size, &workspace_progress);
        printf("offset=%zu form_sha256=%s first_frame=", offset, hex);
        if (first_frame)
            printf("%ux%u depth=%u", frame.width, frame.height, frame.depth);
        else
            printf("unavailable");
        if (full_pack)
            printf(" PACK=%zu bytes", all_pack_records_size);
        else
            printf(" PACK=%zu bytes unavailable after=%zu records/%zu bytes@%zu",
                   pack_size,
                   pack_progress.records, pack_progress.decoded_size,
                   pack_progress.source_offset);
        if (has_workspace)
            printf(" workspace=%zu bytes source=%zu/%zu", workspace_size,
                   workspace_progress.source_offset, pack_size);
        if (has_script)
            printf(" SCPT=%zu bytes first_sequence=%u records/%zu bytes\n",
                   script.size, script.first_sequence_records,
                   script.first_sequence_size);
        else
            puts(" SCPT=unavailable");
        liberation_anim_frame_free(&frame);
        liberation_anim_script_free(&script);
        free(all_pack_records);
        free(workspace);
        free(form);
        offset += packed_size + 11U;
    }

    free(bundle);
    liberation_data_close(&data);
    vfs_free(&vfs);
    return 0;
}
