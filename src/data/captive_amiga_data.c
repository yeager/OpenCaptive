#include "captive_amiga_data.h"
#include "amiga_ofs.h"
#include "rnc_decoder.h"
#include "sha256.h"

#include <stdlib.h>

static const char amiga_adf_sha256[] =
    "ae7738b36ea5cab8b21250f406424de1bedd4d40d96c7e9155b974fdbdb27bf1";

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
        uint32_t raw_size = rnc_uncompressed_size(resource, (int)resource_size);
        if (!raw_size || raw_size > 1024U * 1024U) {
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
        if (i == 5U && !sha256_data_matches(raw, raw_size,
            "ee0f6d0ca51af38854274949192f0062ff58f4ba02bab47d96f02a4024012997"))
            valid = false;
        free(raw);
        free(resource);
    }
    free(adf);
    return valid;
}
