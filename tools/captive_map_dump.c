#include "captive_dos_map.h"
#include "captive_dos_descriptor.h"

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
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "usage: %s MEMDUMP.BIN [DS-segment-hex] "
                "[source-bank-segment-hex]\n", argv[0]);
        return 2;
    }
    uint16_t ds = 0x2942;
    if (argc >= 3) {
        char *end = NULL;
        unsigned long value = strtoul(argv[2], &end, 16);
        if (!end || *end || value > 0xFFFFUL) {
            fprintf(stderr, "invalid DS segment: %s\n", argv[2]);
            return 2;
        }
        ds = (uint16_t)value;
    }
    uint16_t source_bank_segment = 0x0824;
    if (argc >= 4) {
        char *end = NULL;
        unsigned long value = strtoul(argv[3], &end, 16);
        if (!end || *end || value > 0xFFFFUL) {
            fprintf(stderr, "invalid source-bank segment: %s\n", argv[3]);
            return 2;
        }
        source_bank_segment = (uint16_t)value;
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
    uint8_t current = state.cells[state.player_y * CAPTIVE_DOS_MAP_WIDTH +
                                   state.player_x];
    printf("DS:%04X player=(%u,%u) facing=%u raw=%02X\n", state.ds_segment,
           state.player_x, state.player_y, state.facing, current);
    CaptiveDosViewWindow window;
    if (captive_dos_view_window_build(&state, &window)) {
        puts("CAPPO raw 5x5 window:");
        for (int y = 0; y < 5; ++y) {
            for (int x = 0; x < 5; ++x) {
                if (window.outside[y][x]) fputs("..", stdout);
                else printf("%02X", window.raw[y][x]);
                if (x != 4) putchar(' ');
            }
            putchar('\n');
        }
        puts("CAPPO dispatch handlers:");
        for (int y = 0; y < 5; ++y) {
            for (int x = 0; x < 5; ++x) {
                if (window.outside[y][x]) fputs("------", stdout);
                else printf("%04X", captive_dos_cell_route_address(window.raw[y][x]));
                if (x != 4) putchar(' ');
            }
            putchar('\n');
        }
        puts("CAPPO draw-order records:");
        for (size_t i = 0; i < CAPTIVE_DOS_DISPATCH_RECORD_COUNT; ++i) {
            CaptiveDosDispatchRecord record;
            int x = 0, y = 0;
            if (!captive_dos_dispatch_record_read(
                    memory, 0x100000u, ds, i, &record))
                break;
            if (!captive_dos_dispatch_window_xy(record.window_index, &x, &y)) {
                printf("%02zu index=%02X invalid-window-index\n", i,
                       record.window_index);
                continue;
            }
            uint8_t raw = window.raw[y][x];
            uint16_t handler = (record.byte_at_5 & 0x08U)
                ? captive_dos_cell_route_address(raw)
                : captive_dos_cell_route_normal_address(raw);
            printf("%02zu index=%02X cell=%02X flags=%02X handler=%04X "
                   "w0=%04X w2=%04X b4=%02X b6=%02X\n", i,
                   record.window_index, raw, record.byte_at_5, handler,
                   record.word_at_0, record.word_at_2, record.byte_at_4,
                   record.byte_at_6);
            CaptiveDosDescriptorOperands operands;
            if (captive_dos_dispatch_descriptor_operands(
                    memory, 0x100000u, ds, &record, raw, &operands) &&
                operands.descriptor_count != 0) {
                printf("   descriptor-operands:");
                for (uint8_t n = 0; n < operands.descriptor_count; ++n)
                    printf(" %04X", operands.descriptor_id[n]);
                puts(" (handler-preconditions-pass)");
                for (uint8_t n = 0; n < operands.descriptor_count; ++n) {
                    CaptiveDosDescriptor descriptor;
                    if (!captive_dos_descriptor_read(
                            memory, 0x100000u, ds, source_bank_segment,
                            operands.descriptor_id[n], &descriptor)) {
                        printf("   descriptor %04X: unavailable\n",
                               operands.descriptor_id[n]);
                        continue;
                    }
                    const char *sentinel =
                        descriptor.width_bytes == 0 || descriptor.height == 0
                        ? "sentinel/non-drawable" : "drawable-record";
                    printf("   descriptor %04X: src=%04X dst=%04X "
                           "size=%ux%u flags=%02X bank=%u %s\n",
                           operands.descriptor_id[n], descriptor.source_offset,
                           descriptor.destination_offset,
                           descriptor.width_bytes, descriptor.height,
                           descriptor.flags, descriptor.source_bank, sentinel);
                }
            }
            CaptiveDos1c90Result guard =
                captive_dos_1c90_evaluate(&window, &record, raw);
            printf("   1C90-helper: %s\n",
                   guard == CAPTIVE_DOS_1C90_PASS ? "would-draw" :
                   guard == CAPTIVE_DOS_1C90_FAIL ? "no-draw" : "unknown");
        }
    }
    for (int y = 0; y < CAPTIVE_DOS_MAP_HEIGHT; ++y) {
        printf("%02d ", y);
        for (int x = 0; x < CAPTIVE_DOS_MAP_WIDTH; ++x)
            printf("%02X", state.cells[y * CAPTIVE_DOS_MAP_WIDTH + x]);
        putchar('\n');
    }
    free(memory);
    return 0;
}
