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

/* First-stage CAPPO raw-cell dispatch.  These names intentionally describe
 * code addresses, not guessed gameplay meanings. */
typedef enum {
    CAPTIVE_DOS_CELL_ROUTE_NONE = 0,
    CAPTIVE_DOS_CELL_ROUTE_1C90,
    CAPTIVE_DOS_CELL_ROUTE_1D17,
    CAPTIVE_DOS_CELL_ROUTE_1B06,
    CAPTIVE_DOS_CELL_ROUTE_1DF2,
    CAPTIVE_DOS_CELL_ROUTE_1E35,
    CAPTIVE_DOS_CELL_ROUTE_1DFC,
    CAPTIVE_DOS_CELL_ROUTE_1E13,
    CAPTIVE_DOS_CELL_ROUTE_1DC9,
    CAPTIVE_DOS_CELL_ROUTE_1D72,
    CAPTIVE_DOS_CELL_ROUTE_212F,
    CAPTIVE_DOS_CELL_ROUTE_2701,
    CAPTIVE_DOS_CELL_ROUTE_2171,
    CAPTIVE_DOS_CELL_ROUTE_21A0,
    CAPTIVE_DOS_CELL_ROUTE_26F8,
    CAPTIVE_DOS_CELL_ROUTE_2103,
    CAPTIVE_DOS_CELL_ROUTE_206A,
    CAPTIVE_DOS_CELL_ROUTE_20C7,
    CAPTIVE_DOS_CELL_ROUTE_201C,
    CAPTIVE_DOS_CELL_ROUTE_218C,
    CAPTIVE_DOS_CELL_ROUTE_445A,
    CAPTIVE_DOS_CELL_ROUTE_2065,
    CAPTIVE_DOS_CELL_ROUTE_21A9,
    CAPTIVE_DOS_CELL_ROUTE_272E,
} CaptiveDosCellRoute;

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

/* Apply CAPPO's proven `raw & 0x7f` dispatch gate from 0x1A93. */
CaptiveDosCellRoute captive_dos_cell_route(uint8_t raw);

/* Apply the non-overlay branch at 0x1AC0.  The overlay branch is exposed by
 * captive_dos_cell_route(); CAPPO selects it when DL bit 3 is set at 0x1ABB.
 * Keeping both branches explicit prevents a raw cell code from being given a
 * false universal visual meaning. */
CaptiveDosCellRoute captive_dos_cell_route_normal(uint8_t raw);

/* Return the original CAPPO handler address for a dispatch route.  Zero is
 * reserved for the proven no-route case; addresses are labels from the
 * unpacked CAPPO.EXE, not semantic guesses. */
uint16_t captive_dos_cell_route_address(uint8_t raw);

uint16_t captive_dos_cell_route_normal_address(uint8_t raw);

#endif
