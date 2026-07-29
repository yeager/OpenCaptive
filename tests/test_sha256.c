#include "sha256.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    uint8_t digest[32];
    sha256_digest((const uint8_t *)"abc", 3, digest);
    assert(sha256_matches_hex(digest, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    assert(!sha256_matches_hex(digest, "0000000000000000000000000000000000000000000000000000000000000000"));
    puts("All SHA-256 tests passed");
    return 0;
}
