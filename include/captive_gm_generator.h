#ifndef CAPTIVE_GM_GENERATOR_H
#define CAPTIVE_GM_GENERATOR_H

#include <stdint.h>
#include <stddef.h>

/*
 * Native transcription of GM.EXE, Captive DOS's dungeon level generator (the
 * child program CAPPO.EXE execs).  See docs/CAPTIVE_GM_PORT_PLAN.md for the full
 * pipeline and docs/CAPTIVE_DOS_DUNGEON_RE.md for provenance.
 *
 * GM.EXE operates on a single ~0x6AAC-byte work segment (the relocated `0x2CF`
 * data segment).  We model it as a flat byte buffer with the SAME offsets as the
 * original so every transcribed routine reads/writes exactly where GM does; this
 * lets each pass be verified byte-for-byte against the real GM.EXE via the oracle
 * (opencaptive-re/gm_oracle.py).  Generation is deterministic per mission
 * parameter, so the transcription is exact, not approximate.  Nothing here is
 * synthetic: it reproduces the game's own generator.
 *
 * Build-out is incremental (see the port plan).  Implemented so far:
 *   - captive_gm_entry_setup : entry pointer-table + mission-param storage (0x08)
 *   - captive_gm_seed        : the seed/constant init (0x2F2..0x3B0)
 * The ~35-routine pass chain (0x3B1..0x45F) and the 0xEE generate loop are
 * transcribed pass-by-pass on top of this foundation.
 */

/* GM clears the first 0x6AAC bytes of its work segment at startup; the region
 * above that holds BAKED constant tables from GM.EXE's image that the passes read
 * (e.g. the mission table at 0x6D16).  The buffer therefore spans past 0x6AAC to
 * cover those tables (GM's data reaches ~0x6EE8). */
#define CAPTIVE_GM_CLEAR_SIZE 0x6AACu
#define CAPTIVE_GM_WORK_SIZE  0x10002u  /* full 64K segment: navigation may wrap 16-bit map offsets */

typedef struct {
    uint8_t b[CAPTIVE_GM_WORK_SIZE];
} CaptiveGmWork;

/*
 * Initialise the work segment to GM's post-load, post-clear state: zero the
 * [0, 0x6AAC) region GM clears, and install the baked constant tables (extracted
 * from GM.EXE, real game data) in the region above it.  Call before entry_setup.
 */
void captive_gm_init(CaptiveGmWork *w);

/* 16-bit little-endian word accessors at a work-segment offset (as GM's code,
 * which is DS-relative with DS = the work segment). */
static inline uint16_t captive_gm_wget(const CaptiveGmWork *w, uint16_t off) {
    return (uint16_t)(w->b[off] | (w->b[(uint16_t)(off + 1)] << 8));
}
static inline void captive_gm_wset(CaptiveGmWork *w, uint16_t off, uint16_t v) {
    w->b[off] = (uint8_t)(v & 0xFFu);
    w->b[(uint16_t)(off + 1)] = (uint8_t)(v >> 8);
}

/*
 * Entry setup, transcribed from GM_UNP.EXE 0x08..0xB5 (after the segment clear):
 * stores the four mission parameters at their fixed slots and initialises the
 * buffer pointer table (0x357E..0x3594, 0x3578/0x357A/0x357C).  `w` must be
 * zero-initialised first (GM clears the segment at 0x2C..0x38).
 *   ax = mission param (word[0:0x4F4]);  bx,cx,dx = word[0:0x4F6/0x4F8/0x4FA].
 */
void captive_gm_entry_setup(CaptiveGmWork *w, uint16_t ax, uint16_t bx,
                            uint16_t cx, uint16_t dx);

/*
 * Seed / constant init, transcribed from GM_UNP.EXE 0x2F2..0x3B0: computes
 * word[0x3560] from the mission param, clears the working buffers the pointer
 * table addresses, and writes the fixed generation seeds (0x1000/0x0FF8/0x8882/
 * 0x8881 etc.).  Must be called after captive_gm_entry_setup.
 * Returns 0 for the normal path, 1 if GM would branch to its 0x684 alt-path
 * (word[0x33DC] bit 1 set) — recorded so later passes can honour it.
 */
int captive_gm_seed(CaptiveGmWork *w);

/*
 * GM.EXE's generation RNG, transcribed from GM_UNP.EXE 0x1C6E.  The 16-bit state
 * lives at word[0x3074] (seeded to the mission param at entry, hence
 * deterministic).  Each call advances state = state*0x5E5 + 0x29 (mod 2^16) and
 * returns ror(state, 4) XOR 0x800.  This is the entropy source threaded through
 * the RNG-driven placement passes.
 */
uint16_t captive_gm_rng_next(CaptiveGmWork *w);

/*
 * Random map cell coordinate, transcribed from GM_UNP.EXE 0x1C97: draws one RNG
 * value r and packs a position as (y << 8) | x, with x = r & 0x3F (0..63) and
 * y = ror(r,6) & 0x1F (0..31).  Used by the RNG-driven placement passes.
 */
uint16_t captive_gm_rng_pos(CaptiveGmWork *w);

/*
 * Pass 0x14C9 (first of the generation pass chain): computes word[0x359A], a
 * cell-type/room selector.  For mission <= 9 it reads the baked table at 0x6D16;
 * otherwise it derives the value from two LCG steps (x*0x5E5+0x29) followed by the
 * high word of (x*3), mapped 0->3 / 1->5 / 2->6.  word[0x307C]==1 forces 8.
 */
void captive_gm_pass_14c9(CaptiveGmWork *w);

/*
 * Pass 0x45F: builds the 16-cell room grid (work[0..0xF]).  Lays an initial
 * open/filled pattern from the mission seed word (0x6D20 table), then grows N
 * regions (N = word[0x3082]) by RNG-driven random walks and fills the remainder
 * by copying region ids from neighbours.  Writes the region count to word[0x2E]
 * and word[0x33DA].  Must run after entry_setup + seed.
 */
void captive_gm_pass_45f(CaptiveGmWork *w);

/*
 * Pass 0x526: computes inter-region connection vectors (scaled col/row deltas
 * between adjacent regions, at work[0x10+]) and picks the entry cell (word[0x20]
 * = entry index, word[0x22] = its grid cell).  Must run after pass 0x45F.
 */
void captive_gm_pass_526(CaptiveGmWork *w);

/*
 * Pass 0x5D4: expands the 4x4 room grid into the 2048-word input map at
 * work[0x38..0x1038] (horizontal + vertical wall words between grid cells) — the
 * base layout the 0xEE driver later translates.  Must run after pass 0x526.
 */
void captive_gm_pass_5d4(CaptiveGmWork *w);

/*
 * Map-indexing / cell-validation primitives shared by the map-building passes,
 * transcribed from GM.EXE.  cl = x (column), ch = y (row) of a 64x32 map.
 */

/* GM 0x248E: byte offset of map cell (cl,ch) in a 64-wide word map = (ch*64+cl)*2. */
static inline uint16_t captive_gm_map_index(uint8_t cl, uint8_t ch) {
    return (uint16_t)((((uint16_t)ch << 6) + cl) << 1);
}

/* GM 0x2831: the 4x4 room-grid cell (work[0..0xF]) covering map position (cl,ch):
 * index = (((ch & 0x18) << 3) + cl) >> 4. */
static inline uint8_t captive_gm_grid_cell(const CaptiveGmWork *w,
                                           uint8_t cl, uint8_t ch) {
    uint16_t bp = (uint16_t)(((((uint16_t)(ch & 0x18u)) << 3) + cl) >> 4);
    return w->b[bp];
}

/* Result of captive_gm_cell_check (GM 0x1C1C). */
typedef enum {
    CAPTIVE_GM_CELL_VALID = 0,   /* in bounds, non-empty, < 0xFFCF (GM ax=0)      */
    CAPTIVE_GM_CELL_EMPTY = 1,   /* cell word == 0                (GM ax=1, ZF=0) */
    CAPTIVE_GM_CELL_BLOCKED = 2  /* out of bounds or word >= 0xFFCF (GM ax=1,ZF=1)*/
} CaptiveGmCellStatus;

/*
 * GM 0x1C1C: classify map cell (cl,ch) in the word map based at `map_off` (the
 * work-segment offset the original passes hold in DI, e.g. 0x1048).  Out-of-range
 * coordinates and sentinel values (>= 0xFFCF) are BLOCKED; a zero word is EMPTY;
 * anything else is VALID.
 */
CaptiveGmCellStatus captive_gm_cell_check(const CaptiveGmWork *w, uint16_t map_off,
                                          uint8_t cl, uint8_t ch);

/*
 * Pass 0x1CB5: the room-outline validator + anchor placement.  Fills the anchor
 * array at work[0x3430] with packed room records.  Run after pass 0x5D4.
 */
void captive_gm_pass_1cb5(CaptiveGmWork *w);

/*
 * Pass 0x1617: draws room outlines (0x2055) into the selector map for missions
 * with bit 1 set.  Run after pass 0x1CB5.
 */
void captive_gm_pass_1617(CaptiveGmWork *w);

/*
 * Pass 0xD12: the drunkard's-walk room/corridor placement machine (writes the
 * cell-type map).  Run after pass 0x1617.
 */
void captive_gm_pass_d12(CaptiveGmWork *w);
void captive_gm_pass_2589(CaptiveGmWork *w);
void captive_gm_pass_26be(CaptiveGmWork *w);
void captive_gm_pass_28b2(CaptiveGmWork *w);
void captive_gm_pass_29f6(CaptiveGmWork *w);
void captive_gm_pass_164c(CaptiveGmWork *w);
void captive_gm_pass_2888(CaptiveGmWork *w);
void captive_gm_pass_2940(CaptiveGmWork *w);
void captive_gm_pass_1314(CaptiveGmWork *w);
void captive_gm_pass_e12(CaptiveGmWork *w);
void captive_gm_pass_1736(CaptiveGmWork *w);
void captive_gm_pass_1806(CaptiveGmWork *w);
void captive_gm_pass_2a9d(CaptiveGmWork *w);
void captive_gm_pass_2abc(CaptiveGmWork *w);
void captive_gm_pass_a2a(CaptiveGmWork *w);
void captive_gm_pass_a2a_firstloop(CaptiveGmWork *w);
void captive_gm_pass_2595(CaptiveGmWork *w, uint16_t bpval, uint16_t axval);
void captive_gm_pass_9c3(CaptiveGmWork *w);
void captive_gm_pass_967(CaptiveGmWork *w);
void captive_gm_pass_f61(CaptiveGmWork *w);
void captive_gm_pass_2284(CaptiveGmWork *w);
void captive_gm_pass_22ba(CaptiveGmWork *w);
void captive_gm_pass_22eb(CaptiveGmWork *w);
void captive_gm_pass_2310(CaptiveGmWork *w);
void captive_gm_pass_2400(CaptiveGmWork *w);
void captive_gm_pass_242e(CaptiveGmWork *w);

/*
 * The final translate driver (GM 0xEE): converts the cell-type map at work[0x1048]
 * — gated by the selector map at work[0x38] and the aux map at work[0x2058] — into
 * the 64x32 output map at work[0x5A68] (and the second map at 0x6288), using
 * captive_gm_translate_cell.  Run after all the map-building passes.  The output
 * map is CAPTIVE_GM_OFF_OUTMAP (0x5A68), 2048 bytes, row-major 64-wide.
 */
void captive_gm_generate_output(CaptiveGmWork *w);

/* Offsets of the key work-segment fields (for callers/tests). */
#define CAPTIVE_GM_OFF_MISSION   0x3078u  /* mission param copy */
#define CAPTIVE_GM_OFF_OUTMAP    0x5A68u  /* output 64x32 map (ptr at 0x3578) */
#define CAPTIVE_GM_OFF_PTRTAB    0x357Eu  /* first pointer-table entry */

#endif /* CAPTIVE_GM_GENERATOR_H */
