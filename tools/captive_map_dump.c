#include "captive_dos_map.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool read_file(const char *path, uint8_t *buffer, size_t size) {
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    bool ok = fread(buffer, 1, size, file) == size;
    fclose(file);
    return ok;
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s MEMDUMP.BIN [DS-segment-hex]\n", argv[0]);
        return 2;
    }
    uint16_t ds = 0x2942;
    if (argc == 3) {
        char *end = NULL;
        unsigned long value = strtoul(argv[2], &end, 16);
        if (!end || *end || value > 0xFFFFUL) {
            fprintf(stderr, "invalid DS segment: %s\n", argv[2]);
            return 2;
        }
        ds = (uint16_t)value;
    }

    uint8_t *memory = malloc(0x100000u);
    if (!memory) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }
    if (!read_file(argv[1], memory, 0x100000u)) {
        fprintf(stderr, "could not read complete 1 MiB dump %s: %s\n",
                argv[1], errno ? strerror(errno) : "short file");
        free(memory);
        return 1;
    }

    CaptiveDosMapState state;
    if (!captive_dos_map_decode(memory, 0x100000u, ds, &state)) {
        fprintf(stderr, "dump does not contain a valid CAPPO map at DS:%04X\n",
                ds);
        free(memory);
        return 1;
    }
    free(memory);

    uint8_t current = state.cells[state.player_y * CAPTIVE_DOS_MAP_WIDTH +
                                   state.player_x];
    printf("DS:%04X player=(%u,%u) facing=%u raw=%02X\n", state.ds_segment,
           state.player_x, state.player_y, state.facing, current);
    for (int y = 0; y < CAPTIVE_DOS_MAP_HEIGHT; ++y) {
        printf("%02d ", y);
        for (int x = 0; x < CAPTIVE_DOS_MAP_WIDTH; ++x)
            printf("%02X", state.cells[y * CAPTIVE_DOS_MAP_WIDTH + x]);
        putchar('\n');
    }
    return 0;
}
