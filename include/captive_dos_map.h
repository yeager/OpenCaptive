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

/* CAPPO's copied neighbourhood at DS:12F1 contains five map bytes per row,
 * with three bytes of workspace padding between rows.  The raw window keeps
 * the original orientation transform and does not classify the bytes. */
typedef struct {
    uint8_t raw[5][5];
    bool outside[5][5];
    uint8_t facing;
} CaptiveDosViewWindow;

/* Decode only the proven CAPPO fields from one complete 1 MiB dump. */
bool captive_dos_map_decode(const uint8_t *memory, size_t memory_size,
                            uint16_t ds_segment, CaptiveDosMapState *out);

/* Return the original unmasked CAPPO cell byte. */
bool captive_dos_map_cell(const CaptiveDosMapState *state, int x, int y,
                          uint8_t *out_raw);

/* Build CAPPO's exact 5x5 raw neighbourhood from DS:7CB3 and DS:5E80/82.
 * CAPPO.EXE: 0x1818, orientation branches 0x1837/0x18BC/0x1925/0x199F. */
bool captive_dos_view_window_build(const CaptiveDosMapState *state,
                                   CaptiveDosViewWindow *out);

#endif
