#ifndef CAPTIVE_DOS_MAP_LOAD_H
#define CAPTIVE_DOS_MAP_LOAD_H

#include <stdint.h>

#include "game_state.h"

/*
 * Convert a captured Captive DOS map (the 64x32 byte grid at DS:0x7CB3, e.g.
 * pulled from a DOSBox-X savestate by opencaptive-re/capsnap/extract_map.py)
 * into the engine's DungeonLevel so the existing captive_view_window +
 * compositor can render the REAL, game-generated dungeon.
 *
 * Cell semantics used here are the ones confirmed by static RE of CAPPO (see
 * docs/CAPTIVE_DOS_DUNGEON_RE.md): a cell is solid/drawn when (byte & 0x7F) >
 * 0x1A (this is exactly the holomap renderer's test at 0x4BAF4), otherwise it
 * is open floor.  Finer cell-type distinctions (doors, teleporters, stairs)
 * require more cell-semantic RE and are intentionally NOT guessed here; the
 * conversion is deliberately conservative so it never invents structure the
 * original data does not encode.
 *
 * `bytes` must point to CAPTIVE_DOS_RAW_MAP_SIZE (2048) bytes, row-major with a
 * 64-wide stride (index = y*64 + x), matching CAPPO's layout.
 */
#define CAPTIVE_DOS_RAW_MAP_SIZE 2048

void captive_dos_map_to_level(DungeonLevel *out, const uint8_t *bytes);

/* True when a captured cell byte is a solid/drawn wall: (byte & 0x7F) > 0x1A. */
static inline int captive_dos_cell_is_wall(uint8_t cell) {
    return (uint8_t)(cell & 0x7Fu) > 0x1Au;
}

#endif /* CAPTIVE_DOS_MAP_LOAD_H */
