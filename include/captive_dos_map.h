#ifndef CAPTIVE_DOS_MAP_H
#define CAPTIVE_DOS_MAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* CAPPO's flat dungeon map is 64x32 bytes at DS:7CB3.  This module is an
 * analysis boundary for a caller-owned DOSBox-X memory dump; it deliberately
 * exposes the original byte codes instead of guessing OpenCaptive CellTypes.
 * CAPPO.EXE: map access at 0x4936/0x4949, position at DS:5E80/5E82. */
#define CAPTIVE_DOS_MAP_WIDTH 64
#define CAPTIVE_DOS_MAP_HEIGHT 32
#define CAPTIVE_DOS_MAP_SIZE (CAPTIVE_DOS_MAP_WIDTH * CAPTIVE_DOS_MAP_HEIGHT)
#define CAPTIVE_DOS_MAP_OFFSET 0x7CB3u
#define CAPTIVE_DOS_POSITION_X_OFFSET 0x5E80u
#define CAPTIVE_DOS_POSITION_Y_OFFSET 0x5E82u
#define CAPTIVE_DOS_FACING_OFFSET 0x5E84u

typedef struct {
    uint16_t ds_segment;
    uint8_t cells[CAPTIVE_DOS_MAP_SIZE];
    uint8_t player_x;
    uint8_t player_y;
    uint8_t facing;
} CaptiveDosMapState;

/* Decode only the proven CAPPO fields from one complete 1 MiB dump. */
bool captive_dos_map_decode(const uint8_t *memory, size_t memory_size,
                            uint16_t ds_segment, CaptiveDosMapState *out);

/* Return the original unmasked CAPPO cell byte. */
bool captive_dos_map_cell(const CaptiveDosMapState *state, int x, int y,
                          uint8_t *out_raw);

#endif
