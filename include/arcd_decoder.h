#ifndef ARCD_DECODER_H
#define ARCD_DECODER_H

#include <stdint.h>
#include <stddef.h>

#define ARCD_MAGIC 0x41726344  /* "ArcD" */
/* ArcD is used for small Liberation text tables.  Reject corrupt advertised
 * sizes before callers allocate the output buffer. */
#define ARCD_MAX_DECOMPRESSED_SIZE (64U * 1024U * 1024U)

int arcd_decode(const uint8_t *src, size_t src_size,
                uint8_t *dst, size_t dst_size);

size_t arcd_decompressed_size(const uint8_t *src, size_t src_size);

#endif
