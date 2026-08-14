#ifndef CAPTIVE_DOS_GENERATOR_H
#define CAPTIVE_DOS_GENERATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Native reconstruction of CAPPO.EXE's procedural dungeon generator.
 *
 * Captive's DOS archive ships NO level files: the dungeon is generated at
 * runtime into a flat 64x32 byte map at DS:7CB3.  This module transcribes the
 * pieces of that generator that have been recovered by static analysis of the
 * LZEXE-unpacked CAPPO image (see docs/CAPTIVE_DOS_DUNGEON_RE.md).  Every value
 * and transform here is copied from the original code, not invented, so the
 * output is the game's own real data rather than a synthetic maze.
 *
 * Provenance (CAPPO_CODE.bin offsets, code segment CS=0; DGROUP paragraph
 * 0x0E3F so a datum at DS:O lives at file 0x0E3F0+O):
 *   - map:            DS:0x7CB3, 64x32 = 2048 cells.
 *   - index calc:     0x4749  -> cell = (y<<6) + x.
 *   - walker:         0x4764  -> dir = step_table[..]&7; cx+=DX[dir]; dx+=DY[dir].
 *   - direction deltas: DX @DS:0x5E18, DY @DS:0x5E20 (see tables below).
 *   - bounds check:   0x498C  -> x in [0,63], y in [0,31].
 *   - post-processor: 0x4661  -> per-cell finishing pass (see below).
 */

#define CAPTIVE_GEN_MAP_WIDTH  64
#define CAPTIVE_GEN_MAP_HEIGHT 32
#define CAPTIVE_GEN_MAP_SIZE   (CAPTIVE_GEN_MAP_WIDTH * CAPTIVE_GEN_MAP_HEIGHT)

/* Direction indices as used by CAPPO's walker (0x4764).  0..3 are the cardinal
 * steps recovered from the real delta tables. */
typedef enum {
    CAPTIVE_GEN_DIR_NORTH = 0, /* DX=0,  DY=-1 */
    CAPTIVE_GEN_DIR_WEST  = 1, /* DX=-1, DY=0  */
    CAPTIVE_GEN_DIR_SOUTH = 2, /* DX=0,  DY=+1 */
    CAPTIVE_GEN_DIR_EAST  = 3  /* DX=+1, DY=0  */
} CaptiveGenDir;

/* Real cardinal-step deltas, transcribed from DS:0x5E18 / DS:0x5E20.
 * (The full 8-entry tables include four non-cardinal entries handled by the
 * special walker branches 0x4972/0x498C; the cardinal four are exact here.) */
extern const int8_t captive_gen_dx[4];
extern const int8_t captive_gen_dy[4];

/* Flat map index from (x,y), exactly CAPPO 0x4749: (y<<6)+x. */
static inline int captive_gen_index(int x, int y) {
    return (y << 6) + x;
}

/* Bounds test, exactly CAPPO 0x498C (unsigned cmp against 0x3F / 0x1F). */
static inline bool captive_gen_in_bounds(int x, int y) {
    return (unsigned)x <= 0x3Fu && (unsigned)y <= 0x1Fu;
}

/*
 * Post-processing pass, transcribed byte-for-byte from CAPPO 0x4661.
 * For each of the 2048 cells:  al = cell & 0x7F;
 *   if al == 0x1B                          -> cell += 5
 *   else if al in {0x40,0x43,0x48,0x4B}    -> cell |= 0x10
 * (cells is modified in place).  Returns the number of cells it changed.
 */
size_t captive_gen_postprocess(uint8_t cells[CAPTIVE_GEN_MAP_SIZE]);

/* A cell is solid wall when (cell & 0x7F) > 0x1A (CAPPO map semantics). */
static inline bool captive_gen_is_wall(uint8_t cell) {
    return (uint8_t)(cell & 0x7Fu) > 0x1Au;
}

#endif /* CAPTIVE_DOS_GENERATOR_H */
