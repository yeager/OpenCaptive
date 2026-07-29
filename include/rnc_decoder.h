#ifndef RNC_DECODER_H
#define RNC_DECODER_H

#include <stdint.h>
#include <stdbool.h>

// RNC (Rob Northen Compression) decoder. It supports the modern forward
// RNC1 stream plus the older backward RNC1 and RNC2 Amiga stream layouts.

int rnc_decode(const uint8_t *src, int src_len, uint8_t *dst, int dst_cap);
uint32_t rnc_uncompressed_size(const uint8_t *src, int src_len);
bool rnc_is_compressed(const uint8_t *data, int len);

#endif
