#ifndef CAPTIVE_DOS_GENERATOR_H
#define CAPTIVE_DOS_GENERATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Native reconstruction of CAPPO.EXE's dungeon map primitives.
 *
 * Captive's DOS archive ships NO level files: the dungeon lives at runtime in a
 * flat 64x32 byte map at DS:7CB3.  This module transcribes the map-mutation
 * PRIMITIVES recovered by static analysis of the LZEXE-unpacked CAPPO image
 * (see docs/CAPTIVE_DOS_DUNGEON_RE.md).  Every value and transform here is
 * copied byte-for-byte from the original code, not invented, so anything built
 * from them is the game's own real data rather than a synthetic maze.
 *
 * SCOPE / HONESTY: these are the confirmed primitives (the cell writer, the
 * plus-brush, the post-processor, the cardinal deltas, the index and bounds).
 * They are NOT yet the whole level generator: the routine 0x46CC that drives a
 * walk with these primitives is a DGROUP dispatch-table handler (at DS:0xCAFD)
 * whose exact role (level builder vs. per-entity walk) is still unconfirmed.
 * The parallel array at DS:0x84D3 (= map+0x820) is a per-cell FLAGS/visibility
 * byte (bit 0x80 = explored, set by the auto-map reveal at 0x6580), NOT a maze
 * direction field.  Which subsystem builds the map's initial structure (the
 * carve phase machine, the 8 RNG "diggers" at 0x454A, or another) is not yet
 * cleanly isolated.  This module therefore exposes verified building blocks; it
 * does not claim to reproduce a full per-level dungeon yet.
 *
 * Provenance (CAPPO_CODE.bin offsets, code segment CS=0; DGROUP paragraph
 * 0x0E3F so a datum at DS:O lives at file 0x0E3F0+O):
 *   - map:            DS:0x7CB3, 64x32 = 2048 cells.
 *   - index calc:     0x4749  -> cell = (y<<6) + x.
 *   - walker:         0x4764  -> dir = flags[i]&7; cx+=DX[dir]; dx+=DY[dir].
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

/*
 * CAPPO's pseudo-random generator, exactly the LCG at 0x44EF..0x44FA:
 *   state = state * 0x05E5 + 0x0029   (mod 2^16)
 * The 16-bit state lives at DS:0x9322.  captive_gen_rng_next advances *state
 * in place and returns the new value.
 */
#define CAPTIVE_GEN_RNG_MUL 0x05E5u
#define CAPTIVE_GEN_RNG_ADD 0x0029u

static inline uint16_t captive_gen_rng_next(uint16_t *state) {
    *state = (uint16_t)(*state * CAPTIVE_GEN_RNG_MUL + CAPTIVE_GEN_RNG_ADD);
    return *state;
}

/*
 * Direction (0..3) derived from the RNG state, exactly CAPPO 0x4605:
 *   ax = state; rol al,1; rol al,1; ax &= 3
 * i.e. two left-rotates of the LOW byte, then the low two bits — which selects
 * bits 6 and 7 of the low byte (wrapped) as a 0..3 direction index.
 */
static inline int captive_gen_rng_dir(uint16_t state) {
    uint8_t al = (uint8_t)(state & 0xFFu);
    al = (uint8_t)((al << 2) | (al >> 6)); /* rol al,1 twice */
    return al & 3;
}

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

/* A cell is solid wall when (cell & 0x7F) > 0x1A (CAPPO map semantics,
 * confirmed by the holomap renderer at CAPPO 0xBAF4: `cmp byte[si],0x1A; jbe`
 * skips non-walls). */
static inline bool captive_gen_is_wall(uint8_t cell) {
    return (uint8_t)(cell & 0x7Fu) > 0x1Au;
}

/* The value the generator's walk stamps into each visited cell.  The writer
 * (CAPPO 0x4736) does al = (map[i] & 0x80) | 0x26 — it forces the low 7 bits
 * to 0x26 while preserving the high (marker) bit.  0x26 is a *drawn* cell
 * (0x26 > 0x1A), so the drunkard's walk lays down map structure rather than
 * clearing floor; the precise wall/feature meaning of 0x26 vs the surrounding
 * bytes is still being pinned down (see docs/CAPTIVE_DOS_DUNGEON_RE.md). */
#define CAPTIVE_GEN_WALK_STAMP 0x26u

/*
 * Stamp one cell, exactly CAPPO 0x4736:
 *   if x in [0,0x3F] and y in [0,0x1F]:
 *       map[(y<<6)+x] = (map[(y<<6)+x] & 0x80) | 0x26
 * Out-of-bounds coordinates are ignored (the original returns without writing).
 */
void captive_gen_carve_cell(uint8_t cells[CAPTIVE_GEN_MAP_SIZE], int x, int y);

/*
 * Stamp the plus-shaped brush the generator applies at each walk step,
 * exactly CAPPO 0x4706..0x472A: the centre cell plus its W, E, N and S
 * neighbours are each stamped (via captive_gen_carve_cell, so every cell is
 * individually bounds-checked).
 */
void captive_gen_carve_plus(uint8_t cells[CAPTIVE_GEN_MAP_SIZE], int x, int y);

#endif /* CAPTIVE_DOS_GENERATOR_H */
