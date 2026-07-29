#ifndef RNC_DECODER_H
#define RNC_DECODER_H

#include <stdint.h>
#include <stdbool.h>

// RNC (Rob Northen Compression) decoder. Method 1 is forward bitstream;
// Liberation additionally uses the old Amiga method 2 backward bitstream.

int rnc_decode(const uint8_t *src, int src_len, uint8_t *dst, int dst_cap);
uint32_t rnc_uncompressed_size(const uint8_t *src, int src_len);
bool rnc_is_compressed(const uint8_t *data, int len);

#endif
