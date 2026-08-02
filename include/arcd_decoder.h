#ifndef ARCD_DECODER_H
#define ARCD_DECODER_H

#include <stdint.h>
#include <stddef.h>

#define ARCD_MAGIC 0x41726344  /* "ArcD" */

int arcd_decode(const uint8_t *src, size_t src_size,
                uint8_t *dst, size_t dst_size);

size_t arcd_decompressed_size(const uint8_t *src, size_t src_size);

#endif
