#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "rnc_decoder.h"

// RNC inspection decoder. The shared runtime decoder supports RNC1 and the
// old backwards RNC2 stream used by verified Liberation CD32 data.

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: rnc_decode <input> <output>\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", argv[1]); return 1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    if (size < 18 || data[0] != 'R' || data[1] != 'N' || data[2] != 'C') {
        fprintf(stderr, "Not an RNC file\n");
        free(data);
        return 1;
    }

    uint32_t unp_size = ((uint32_t)data[4] << 24) | ((uint32_t)data[5] << 16) |
                        ((uint32_t)data[6] << 8) | data[7];
    uint32_t packed_size = ((uint32_t)data[8] << 24) | ((uint32_t)data[9] << 16) |
                           ((uint32_t)data[10] << 8) | data[11];
    /* RNC1 uses both a forward 18-byte layout and the older backwards
       12-byte layout. Match the runtime discriminator before validating the
       claimed payload length. */
    size_t header_size = 18u;
    if (data[3] == 2 ||
        ((size_t)packed_size <= (size_t)size - 12u &&
         (data[(size_t)packed_size + 11u] & 0x80u)))
        header_size = 12u;
    if ((size_t)size < header_size) {
        fprintf(stderr, "Truncated RNC header\n");
        free(data);
        return 1;
    }
    if ((size_t)packed_size > (size_t)size - header_size) {
        fprintf(stderr, "Truncated RNC payload\n");
        free(data);
        return 1;
    }
    printf("RNC method %d, compressed %u -> uncompressed %u\n", data[3],
           packed_size, unp_size);

    uint8_t *output = calloc(unp_size + 4096, 1);
    int result = rnc_decode(data, (int)size, output, (int)unp_size);
    printf("Decoded %d bytes\n", result);

    if (result > 0) {
        f = fopen(argv[2], "wb");
        if (!f) { fprintf(stderr, "Cannot open %s for writing\n", argv[2]); free(output); free(data); return 1; }
        fwrite(output, 1, result, f);
        fclose(f);
        printf("Wrote %s\n", argv[2]);
    }

    free(output);
    free(data);
    return 0;
}
