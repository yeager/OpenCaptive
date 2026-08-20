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
- 0x1314, 0xE12, 0x1736, 0x1806, 0x2A9D, 0x2ABC, 0xA2A  [0xE12..0xA2A LANDED, byte-exact]
- 0x2595, 0x9C3, 0x967, 0xF61  [LANDED, byte-exact]
- 0x2284, 0x22BA, 0x22EB, 0x2310, 0x2400, 0x242E
- 0x2595, 0x157E, 0x13E3, 0x237F, 0x23B4, 0x1460  [LANDED, byte-exact — PASS CHAIN COMPLETE]
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

## 0xD12 — the core placement machine (LANDED, byte-exact 2026-08-20)

The largest generator subsystem: a drunkard's-walk room/corridor placer.  Fully
transcribed as `captive_gm_pass_d12` and verified byte-for-byte against the real
GM.EXE — after 0xD12 the cell-type map (0x1048), selector map (0x38), and aux map
(0x2058) are byte-identical to GM for missions 1/2/3, and both RNG states match
(map1048 m1 550/0x42D6, m2 280/0x17FE, m3 191/0x10DE; `test_pass_d12`).

Verification method that closed the last mile: an instruction-level diff harness
(`gm_call.py` for isolated sub-routines; a Unicorn full-run hook tagging every
word[0x3074] draw by call-site and every 0x1048 write by cell) pinned each divergence
to a single routine.  Bugs found and fixed: carve's 0x25B5 must relocate the *stepped*
cursor; the 0x2055 mode-0 tail must reload word[0x6DE4] and step with the leftover
bp=0xFFFF; the 0x2055 draw scan must decrement word[0x33CE] per row; and the ws:0x6D5E
skip-pattern table had to be baked (it was reading zeros).

Note: 43 pre-0xD12 scratch bytes (word[0x3070], ws:0x6AB2..0x6AFF) are set by an earlier
pass and neither read nor written by 0xD12, so they do not affect its output; resolve
them when transcribing the passes after 0xD12.

Historical structure notes (offsets in NOTE/CS=0 space, +0x600 vs the CS=BASE disasm):

Structure (GM_UNP.EXE offsets):
- `0xD12` driver: sets word[0x3510]=0 (mode 0), reads the entry cell (word[0x20]/
  [0x22] from pass 0x526), budgets word[0x307E]=0x12C and word[0x3080]=0x384, then
  loops the dispatcher until word[0x6DE2] >= 0x3001 or a budget is exhausted.  Uses a
  SECOND RNG (`0x1C57`, state at word[0x355C]) alongside the main RNG (word[0x3074]).
- `0x1F00` / `0x1F29`: two entries to the same walker/dispatcher (ax&3 vs ax&7).  Both
  save (cl,ch,dh) to work[0x30]/[0x32]/[0x34], then dispatch by direction to the
  carve-block entries, or (by RNG thresholds on word[0x308A]) to `0x24A9` / `0x2055`.
- Carve block `0x1F7E..0x2054`: plain step (0x1F7E), or carve a cell — bounds check,
  region match (`0x1BD2`), neighbour count == 4 (`0x2681`), stamp selector (`0x26AE`),
  then write the room code into the cell-type map (codes 4/5, 0x35/0x36, 6, 0x37,
  +0x31), and post-process (`0x25B5`).
- `0x24A9`: feature/door placer (code 0x11 or 0x14) using `0x255D` (mark), `0x250D`
  (place satellites via the `0x1F00` walker), `0x2590` (cleanup 0xFFFE markers).
- Helpers: `0x2854` (step back by connection vector; bp from the ORIGINAL dh, then
  dh--), `0x286E` (step forward; dh++ then bp), `0x25A8` (restore cursor from
  work[0x30/32/34]).

Verification: `opencaptive-re/gm_call.py` invokes any GM sub-routine with a controlled
work-segment state and registers and reads back the result, so each transcribed
routine is checked against the real GM code in isolation before assembly (the same
method used for the 0x129 translator).  Full-pass target: map1048 non-zero/checksum
after 0xD12 — m1 550/0x42D6, m2 280/0x17FE, m3 191/0x10DE.
