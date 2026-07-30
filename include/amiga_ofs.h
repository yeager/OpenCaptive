#ifndef AMIGA_OFS_H
#define AMIGA_OFS_H

#include <stddef.h>
#include <stdint.h>

/* Visits each reconstructible OFS file by its bytes and SHA-256 digest.
 * The filesystem name is deliberately never exposed: callers that inventory
 * original media must make every identity decision from content alone. Return
 * false from the visitor to stop the traversal. */
typedef int (*AmigaOFSHashVisitor)(const uint8_t *data, size_t size,
                                   const uint8_t digest[32], void *context);

int amiga_ofs_visit_file_hashes(const uint8_t *adf, size_t adf_size,
                                AmigaOFSHashVisitor visitor, void *context);

/* Finds a file stored on a standard Amiga OFS/FFS disk image by the digest of
 * its reconstructed contents. The filesystem's directory and file names are
 * never used as an identity decision. The returned buffer is malloc-owned. */
uint8_t *amiga_ofs_find_file_sha256(const uint8_t *adf, size_t adf_size,
                                    const char expected_sha256[65],
                                    size_t *out_size);

#endif
