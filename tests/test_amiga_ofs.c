#include "amiga_ofs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put_be32(unsigned char *at, unsigned value) {
    at[0] = (unsigned char)(value >> 24);
    at[1] = (unsigned char)(value >> 16);
    at[2] = (unsigned char)(value >> 8);
    at[3] = (unsigned char)value;
}

int main(void) {
    unsigned char adf[6 * 512] = {0};
    memcpy(adf, "DOS\0", 4);
    unsigned char *header = adf + 2 * 512;
    put_be32(header, 2);                 /* T_HEADER */
    put_be32(header + 4, 2);             /* own key */
    put_be32(header + 16, 3);            /* first data block */
    put_be32(header + 508, 0xfffffffdU); /* ST_FILE */
    unsigned char *data = adf + 3 * 512;
    put_be32(data, 8);                   /* T_DATA */
    put_be32(data + 4, 2);               /* parent file header */
    put_be32(data + 12, 4);              /* payload bytes */
    memcpy(data + 24, "test", 4);

    size_t size = 0;
    unsigned char *result = amiga_ofs_find_file_sha256(adf, sizeof(adf),
        "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08", &size);
    assert(result && size == 4 && memcmp(result, "test", 4) == 0);
    free(result);
    assert(!amiga_ofs_find_file_sha256(adf, sizeof(adf),
        "0000000000000000000000000000000000000000000000000000000000000000", &size));
    puts("All Amiga OFS tests passed");
    return 0;
}
