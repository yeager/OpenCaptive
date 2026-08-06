#include "liberation_anim.h"
#include "liberation_data.h"
#include "rnc_decoder.h"
#include "sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char presentation_bundle_sha256[] =
    "1d3a335d254c0eae919a712dd73bd41b24ed897bf145ed118ccf2277baa7a35f";

static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static int write_ppm(const char *path, const LiberationAnimFrame *frame) {
    if (!path || !frame || !frame->bitplanes) return 0;
    FILE *out = fopen(path, "wb");
    if (!out) return 0;
    fprintf(out, "P6\n%u %u\n255\n", frame->width, frame->height);
    size_t count = (size_t)frame->width * frame->height;
    uint32_t *pixels = calloc(count, sizeof(*pixels));
    if (!pixels) { fclose(out); return 0; }
    liberation_anim_blit(frame, pixels, frame->width, frame->height, 0, 0);
    for (size_t i = 0; i < count; ++i) {
        fputc((pixels[i] >> 16) & 255, out);
        fputc((pixels[i] >> 8) & 255, out);
        fputc(pixels[i] & 255, out);
    }
    free(pixels);
    return fclose(out) == 0;
}

static int write_bytes(const char *path, const uint8_t *bytes, size_t size) {
    if (!path || !bytes || !size) return 0;
    FILE *out = fopen(path, "wb");
    if (!out) return 0;
    int ok = fwrite(bytes, 1, size, out) == size;
    return fclose(out) == 0 && ok;
}

static void digest_hex(const uint8_t digest[32], char text[65]) {
    for (size_t i = 0; i < 32; ++i)
        snprintf(text + i * 2, 3, "%02x", digest[i]);
    text[64] = '\0';
}

static int find_pack(const uint8_t *form, size_t form_size,
                     const uint8_t **pack, size_t *pack_size) {
    if (!form || !pack || !pack_size || form_size < 12U ||
        memcmp(form, "FORM", 4) != 0 || memcmp(form + 8U, "ANIM", 4) != 0)
        return 0;
    for (size_t pos = 12U; pos + 8U <= form_size;) {
        uint32_t chunk_size = read_be32(form + pos + 4U);
        pos += 8U;
        if (chunk_size > form_size - pos) return 0;
        if (memcmp(form + pos - 8U, "PACK", 4) == 0) {
            *pack = form + pos;
            *pack_size = chunk_size;
            return 1;
        }
        pos += chunk_size + (chunk_size & 1U);
    }
    return 0;
}

int main(int argc, char **argv) {
    bool export_all = argc == 4 && strcmp(argv[1], "--all") == 0;
    if (!export_all && argc != 4 && argc != 5) {
        fprintf(stderr,
                "usage: %s <data-dir> <form-sha256> <output.ppm> [workspace.bin]\n"
                "       %s --all <data-dir> <existing-output-directory>\n", argv[0], argv[0]);
        return 2;
    }
    const char *data_path = export_all ? argv[2] : argv[1];
    const char *output_dir = export_all ? argv[3] : NULL;
    DataVFS vfs;
    LiberationData data = {0};
    if (!vfs_init(&vfs, data_path) || !liberation_data_open(&data, &vfs)) {
        fprintf(stderr, "verified Liberation CD32 data was not found\n");
        liberation_data_close(&data);
        vfs_free(&vfs);
        return 1;
    }
    size_t bundle_size = 0;
    uint8_t *bundle = iso_read_file_sha256(&data.iso, presentation_bundle_sha256,
                                            &bundle_size);
    int result = 1;
    unsigned exported = 0;
    for (size_t offset = 0; bundle && offset + 12U <= bundle_size; ++offset) {
        const uint8_t *rnc = bundle + offset;
        if (memcmp(rnc, "RNC\2", 4) != 0) continue;
        uint32_t raw_size = read_be32(rnc + 4U);
        uint32_t packed_size = read_be32(rnc + 8U);
        if (!raw_size || raw_size > 16U * 1024U * 1024U ||
            packed_size > bundle_size - offset - 12U) continue;
        uint8_t *form = malloc(raw_size);
        if (!form) break;
        int decoded = rnc_decode(rnc, (int)(packed_size + 12U), form, (int)raw_size);
        uint8_t digest[32];
        if (decoded == (int)raw_size) sha256_digest(form, raw_size, digest);
        bool selected = decoded == (int)raw_size &&
            (export_all || sha256_matches_hex(digest, argv[2]));
        if (selected) {
            LiberationAnimFrame frame = {0};
            char form_hash[65];
            char output_path[512];
            digest_hex(digest, form_hash);
            if (export_all) {
                int written = snprintf(output_path, sizeof(output_path), "%s/%s.ppm",
                                       output_dir, form_hash);
                if (written < 0 || (size_t)written >= sizeof(output_path)) {
                    free(form);
                    break;
                }
            }
            const char *ppm_path = export_all ? output_path : argv[3];
            int captured = liberation_anim_decode_first_frame(form, raw_size, &frame) &&
                write_ppm(ppm_path, &frame);
            if (captured && !export_all && argc == 5) {
                const uint8_t *pack = NULL;
                size_t pack_size = 0, workspace_size = 0;
                uint8_t *workspace = NULL;
                captured = find_pack(form, raw_size, &pack, &pack_size) &&
                    liberation_pack_decode_workspace(pack, pack_size, &workspace,
                                                      &workspace_size, NULL) &&
                    write_bytes(argv[4], workspace, workspace_size);
                free(workspace);
            }
            if (captured) {
                result = 0;
                ++exported;
            }
            liberation_anim_frame_free(&frame);
            free(form);
            if (!export_all) break;
            offset += packed_size + 11U;
            continue;
        }
        free(form);
        offset += packed_size + 11U;
    }
    free(bundle);
    liberation_data_close(&data);
    vfs_free(&vfs);
    if (result) fprintf(stderr, "verified FORM/ANIM resource was not captured\n");
    if (export_all && result == 0)
        printf("exported %u matching form occurrences to hash-identified first frames\n",
               exported);
    return result;
}
