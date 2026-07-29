#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t state[8];
    uint64_t bit_count;
    uint8_t buffer[64];
    size_t buffer_len;
} SHA256Context;

void sha256_init(SHA256Context *ctx);
void sha256_update(SHA256Context *ctx, const uint8_t *data, size_t len);
void sha256_final(SHA256Context *ctx, uint8_t digest[32]);
void sha256_digest(const uint8_t *data, size_t len, uint8_t digest[32]);
int sha256_matches_hex(const uint8_t digest[32], const char expected[65]);

#endif
