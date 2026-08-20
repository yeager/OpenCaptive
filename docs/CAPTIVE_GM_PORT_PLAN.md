# Native port of GM.EXE (Captive DOS level generator) — plan

GM.EXE is the child program CAPPO.EXE execs (INT 21h AH=4Bh) to generate each
Captive dungeon level (see `CAPTIVE_DOS_DUNGEON_RE.md`).  Its generation is fully
DETERMINISTIC per mission parameter (fixed seeds 0x8882/0x8881, no entropy), so a
native C transcription can be verified byte-for-byte against the real GM.EXE for
every mission.  This document is the roadmap for that port.

## Verification oracle

`opencaptive-re/gm_oracle.py` runs the real (LZEXE-unpacked) GM_UNP.EXE headless
for a mission param and snapshots its work segment (absolute segment 0x2CF, 0x6AAC
bytes) at labelled breakpoints, with SHA fingerprints:

| label          | breakpoint | meaning                                   |
|----------------|-----------|-------------------------------------------|
| pre_seed       | 0x2F2     | after image load + pointer-table setup    |
| post_seed      | 0x3B1     | after the 0x2F2 seed/init                  |
| pre_generate   | 0xB9      | after the whole pass chain (0x3B1..0x45F) |
| post_generate  | 0x128     | after the 0xEE generate+translate loop    |

The native port must reproduce each snapshot exactly.  Each transcribed pass is
landed only once its post-state SHA matches the oracle for several mission params.

IMPORTANT: the work segment 0x2CF OVERLAPS GM's own loaded image, so at `pre_seed`
it already holds ~1459 non-zero bytes = BAKED CONSTANT TABLES from GM.EXE that the
passes read.  These tables are real game data and are extracted from GM_UNP.EXE's
image (offsets within the 0x2CF paragraph) into C `const` arrays — not invented.

## Memory model (work segment 0x2CF)

- input map:   words at 0x0038.. (2048 words, read by 0xEE at si+0x38)
- parallel arrays at si+0x1048, si+0x2058 (aux/flags used by 0xEE and 0x129)
- output map:  bytes at 0x5A68 (2048 = 64x32, di=word[0x3578]) — the level
- pointer table 0x357E..0x3594 -> buffer offsets 0x3DE2..0x6288 (set at entry)
- mission param at word[0x3078]=ax; word[0x307C]=bx; word[0x33E0]=cx; word[0x33DE]=dx
- seed constants written by 0x2F2: [0x3588]=0x1000,0xFF8,0x8882,0x8881;
  [0x358C]=0x800,0x7F8,0x8882,0x8881; and 0x353C=0xC7,0x3540=7,0x3542=4,0x3398=0x3F

## Generation pipeline (transcribe in order; verify each against the oracle)

Seed:
- `0x2F2`  seed/init from mission param (buffer clears + constant seeds) [DONE-DOC]

Pass chain (called from 0x3B1..0x45F, in order):
- 0x14C9, 0x45F, 0x526, 0x5D4        (early setup)
- 0x1CB5 (di=0x38, si=0x1048)         map-array pass
- 0x1617, 0xD12
- 0x2589, 0x26BE, 0x28B2, 0x29F6, 0x28B2, 0x2888, 0x164C, 0x2940
- 0x1314, 0xE12, 0x1736, 0x1806, 0x2A9D, 0x2ABC, 0xA2A
- 0x2595, 0x9C3, 0x967, 0xF61
- 0x2284, 0x22BA, 0x22EB, 0x2310, 0x2400, 0x242E
- 0x2595, 0x157E, 0x13E3, 0x237F, 0x23B4, 0x1460
- core placement state machine `0x1F00`+ (writers 0x1F2B/0x05E7/0x26B5;
  placers 0x1F95/0x1F8D/0x24A9/0x2055/0x1FB6/0x1FA6) — fills the input map

Generate:
- `0xEE`   2048-cell loop: per cell calls the translator `0x129` -> output map [DONE]

## Map-writing passes (which passes build the visible dungeon)

The 0xEE driver translates the cell-type map at work[0x1048] (gated by the selector
map at work[0x38], with aux at work[0x2058]) into the output map at 0x5A68.  A write
trace (opencaptive-re/whichpass.py) shows which passes touch those map regions:

- work[0x38] (selector):   0x5D4 [done], and many of the 0x1048 passes also update it.
- work[0x1048] (cell type): 0xD12, 0x26BE, 0x28B2, 0x29F6, 0x2940, 0xE12, 0x1736,
  0x1806, 0x2A9D, 0x2ABC, 0xA2A, 0x2595, 0x9C3, 0x967, 0xF61, 0x22BA, 0x22EB, 0x157E,
  0x1F00.
- work[0x2058] (aux):       0x26BE.

The remaining pass-chain routines (0x1CB5, 0x1617, 0x2589, 0x2888, 0x164C, 0x1314,
0x2284, 0x2310, 0x2400, 0x242E, 0x13E3, 0x237F, 0x23B4, 0x1460) build the supporting
data structures (region graph, connection lists, work buffers) the map passes read,
and must still be transcribed in order because the pipeline is stateful.

## Landed so far

- `src/data/captive_gm_translate.c` — the `0x129` per-cell code translator,
  verified against the full 256-entry GM.EXE ground truth (`test_captive_gm_translate`).
- `opencaptive-re/gm_oracle.py` — the byte-exact verification oracle.
- `src/data/captive_gm_generator.c` (`test_captive_gm_generator`), all oracle-verified:
  - `captive_gm_init` — baked-table loader (0x6D16/0x6D20/0x6D2E/0x6D36/0x6D3E).
  - `captive_gm_entry_setup` + `captive_gm_seed` — entry pointer table, mission
    params, seed constants (GM 0x08..0x3B0).
  - `captive_gm_rng_next` / `captive_gm_rng_pos` — the generation RNG (0x1C6E/0x1C97).
  - `captive_gm_pass_14c9` — room/cell selector.
  - `captive_gm_pass_45f` — the 16-cell room grid (region growing) + helpers.
  - `captive_gm_pass_526` — region-connection vectors + entry cell.
  - `captive_gm_pass_5d4` — the full 2048-word input map (selector map at 0x38).

## Next

1. Extract GM's baked constant tables (pre_seed 0x2CF image bytes) into a C blob.
2. Transcribe `0x2F2` seed; verify `post_seed` SHA.
3. Transcribe the pass chain routine-by-routine; verify `pre_generate` SHA.
4. Transcribe the `0xEE` driver over the input map using the existing translator;
   verify `post_generate` / `output_map@0x5A68` SHA.
5. Wire the output map -> `captive_dos_map_to_level` -> view_window/compositor,
   resolving the display-map cell semantics (wall test on GM output codes).
