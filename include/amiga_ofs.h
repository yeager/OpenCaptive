#ifndef AMIGA_OFS_H
#define AMIGA_OFS_H

#include <stddef.h>
#include <stdint.h>

/* Finds a file stored on a standard Amiga OFS/FFS disk image by the digest of
 * its reconstructed contents. The filesystem's directory and file names are
 * never used as an identity decision. The returned buffer is malloc-owned. */
uint8_t *amiga_ofs_find_file_sha256(const uint8_t *adf, size_t adf_size,
                                    const char expected_sha256[65],
                                    size_t *out_size);

#endif
