#include "captive_amiga_data.h"
#include "amiga_ofs.h"
#include "amiga_hunk.h"
#include "amiga_planar.h"
#include "rnc_decoder.h"
#include "sha256.h"

#include <limits.h>
#include <stdlib.h>

static const char amiga_adf_sha256[] =
    "ae7738b36ea5cab8b21250f406424de1bedd4d40d96c7e9155b974fdbdb27bf1";

/* This separate module is selected by content, not its disk-directory name.
 * Its preserved Amiga-HUNK shape and the original MapGen limits in its code
 * make it the recovery source for a future native MapGen port. */
static const char amiga_mapgen_sha256[] =
    "bb5a96b9041e98e5b65a36b5645f2bfe0cbecf68c07479d35f7b4f76ed191118";
static const char amiga_mapgen_raw_sha256[] =
    "b746eb7619a7746eacab2d0a0b2b4b7c42ab34177a22974248d710acf3047ee0";

/* These are content identities for the original main module and every base
 * interior panel set. They deliberately are not archive entry names. */
static const char *const required_ofs_sha256[] = {
    "ba21cc1ddf4baf07fe9a23dee2751da18dbddea7961aac40b344aa93b9fd5aba",
    "47acd3e0404a3909c9b2fd4d15d360dc45e68f75ac21d38541699ceba7db6c32",
    "3b29c6d22469aa0f60313184be3af4192ef847e80349250de8dcd42a48f6486d",
    "9633cb132aff527ba1f85a936043b44e0cd96a87ca825af57822076b40507631",
    "ec75769325075e750d2d8fdbffb375d459461dc407420af4d6d0a489557e2614",
    "160f5776db29beb21365a32844a8f140bcc08a17a8db5807be919d5151299e34",
};

static bool sha256_data_matches(const uint8_t *data, size_t size, const char expected[65]) {
    uint8_t digest[32];
    sha256_digest(data, size, digest);
    return sha256_matches_hex(digest, expected);
}

static bool verify_mapgen_payload(const uint8_t *adf, size_t adf_size) {
    size_t compressed_size = 0;
    uint8_t *compressed = amiga_ofs_find_file_sha256(adf, adf_size,
                                                      amiga_mapgen_sha256,
                                                      &compressed_size);
    if (!compressed) return false;
    if (compressed_size > (size_t)INT_MAX) {
        free(compressed);
        return false;
    }
    uint32_t raw_size = rnc_uncompressed_size(compressed, (int)compressed_size);
    if (raw_size != 29340U || raw_size > (uint32_t)INT_MAX) {
        free(compressed);
        return false;
    }
    uint8_t *raw = malloc(raw_size);
    bool valid = raw && rnc_decode(compressed, (int)compressed_size, raw,
                                   (int)raw_size) == (int)raw_size &&
        sha256_data_matches(raw, raw_size, amiga_mapgen_raw_sha256);
    AmigaHunkInfo hunk;
    if (valid) {
        valid = amiga_hunk_parse(raw, raw_size, &hunk) &&
            hunk.hunk_count == 1U && hunk.code_count == 1U &&
            hunk.data_count == 0U && hunk.bss_count == 0U &&
            hunk.first_code_offset == 32U && hunk.first_code_size == 29304U;
    }
    free(raw);
    free(compressed);
    return valid;
}

bool captive_amiga_data_verify(const DataVFS *vfs) {
    if (!vfs || !vfs->initialized) return false;
    size_t adf_size = 0;
    uint8_t *adf = vfs_find_sha256(vfs, amiga_adf_sha256, &adf_size);
    if (!adf) return false;
    bool valid = true;
    for (size_t i = 0; valid && i < sizeof(required_ofs_sha256) / sizeof(required_ofs_sha256[0]); ++i) {
        size_t resource_size = 0;
        uint8_t *resource = amiga_ofs_find_file_sha256(adf, adf_size,
                                                        required_ofs_sha256[i],
                                                        &resource_size);
        if (!resource) { valid = false; break; }
        if (resource_size > (size_t)INT_MAX) {
            free(resource);
            valid = false;
            break;
        }
        uint32_t raw_size = rnc_uncompressed_size(resource, (int)resource_size);
        if (!raw_size || raw_size > 1024U * 1024U) {
            free(resource);
            valid = false;
            break;
        }
        if (raw_size > (uint32_t)INT_MAX) {
            free(resource);
            valid = false;
            break;
        }
        uint8_t *raw = malloc(raw_size);
        if (!raw || rnc_decode(resource, (int)resource_size, raw, (int)raw_size) != (int)raw_size) {
            free(raw);
            free(resource);
            valid = false;
            break;
        }
        /* The known Fed7-E output has a pinned post-decompression hash. It
         * catches a decoder which merely accepts the header but emits wrong
         * pixels, while remaining independent of the file's original name. */
        if (i == 5U) {
            uint8_t pixels[AMIGA_PLANAR_WIDTH * AMIGA_PLANAR_HEIGHT];
            if (!sha256_data_matches(raw, raw_size,
                "ee0f6d0ca51af38854274949192f0062ff58f4ba02bab47d96f02a4024012997") ||
                !amiga_planar_decode_320x200_5(raw, raw_size, pixels) ||
                !sha256_data_matches(pixels, sizeof(pixels),
                "d2efa8a9cbbbaa45e49c82465765836ba173676645e810fda5cd23ef85bd3431"))
                valid = false;
        }
        free(raw);
        free(resource);
    }
    if (valid) valid = verify_mapgen_payload(adf, adf_size);
    free(adf);
    return valid;
}
