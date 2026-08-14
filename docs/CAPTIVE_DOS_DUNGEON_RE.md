# Captive-1 native dungeon engine — reverse-engineering notes

Goal: render Captive-1's first-person dungeon natively, reading data directly
from the archive (no DOSBox, no unpacking at runtime). DOSBox previously supplied
the live dungeon state; the native engine must generate/hold it itself.

## Status: milestone 1 started (2026-08-14)
Existing scaffolding (already in the tree): `captive_view_window.c` (the 19
visible-cell first-person projection + occlusion), `captive_dos_map.c` (decodes
the 64x32 map from a memory image at DS:7CB3), `captive_compositor.c` (panel/
index blitter). What is MISSING is the native source of the dungeon map + game
state (previously a DOSBox memory dump).

## CAPPO analysis setup
- CAPPO.EXE is LZEXE-91 packed; unpack with `~/bin/unlzexe` -> CAPPO_UNP.EX
  (144556 bytes). Extract for ANALYSIS ONLY (never at runtime).
- Imported into Ghidra as Old-style DOS MZ, x86:LE:16:Real Mode
  (project /Volumes/Extern-disk/opencaptive-re/ghidraproj, program CAPPO_UNP.EX).
  Scripts in /Volumes/Extern-disk/opencaptive-re/cappo_scripts/*.java.

## Map (DS:7CB3) findings
The 64x32 dungeon map lives at offset 0x7cb3 in CAPPO's data segment. Ghidra
resolved its access sites:
- `FUN_1000_2aba`: map index calculator — given CL=x(<0x40) BL=y(<0x20) it forms
  `(y<<6)+x` (= y*64+x), confirming a 64-wide row stride.
- `FUN_1000_1618`: copies the 5x5 neighbourhood around the player
  (DAT_5e80=x, DAT_5e82=y, facing DAT_5e84&3) from the map into the render
  buffer at 0x12f1 — the source the first-person projection consumes.
- Other refs at 0x6624, 0xbaf5, 0xbdea read cells.
NEXT: find the routine that FILLS 0x7cb3 (Captive's procedural level generator,
seeded per level) — it writes 2048 bytes, likely via ES:DI=map + a fill loop.
Then reimplement it natively in C so the map has a real, archive-derived source.

## Milestones (multi-session)
1. [in progress] Locate + RE the native map source (generator vs data file).
2. Reimplement the map generation/state natively (C), archive-driven.
3. Wire native map -> captive_view_window projection -> compositor.
4. Movement, doors, monsters, combat, items.
This is a from-scratch dungeon-engine reimplementation comparable in scope to
the Liberation engine; it is genuinely multi-session work.

## More map findings (2026-08-14, milestone 1 cont.)
- `FUN_1000_baca` is a MAP RENDERER (overhead/automap): it iterates the full
  64x32 map at 0x7cb3, and for each cell `> 0x1a` calls `FUN_1000_bb33` (wall
  graphic lookup) and plots it into a 160-wide screen buffer across three rows
  (offsets +0, +0xa0, +0x140 = 160-px stride). So cell value `> 0x1a` = a
  solid/wall cell; `<= 0x1a` = open/other. This gives the cell-value semantics
  and the plot layout for the native renderer.
- `FUN_1000_65ff` is just a bounds check (x<0x40, y<0x20).
- The GENERATOR that fills 0x7cb3 is still to be located: it writes via ES:DI
  with no immediate 0x7cb3, so trace fns that set DI=0x7cb3 then STOSB/loop, or
  the level-load caller chain above FUN_152f (the per-frame render).

## Map pipeline further RE'd (2026-08-14)
- `FUN_1000_4661` = map POST-PROCESSOR: iterates exactly 0x800 (2048) bytes of
  the map at 0x7cb3 and transforms cells — `(cell & 0x7f) == 0x1b` -> `cell += 5`;
  cell in {0x40,0x43,0x48,0x4b} -> `cell |= 0x10`. Runs after generation.
- `FUN_1000_491c` = a small state/flag reset (not map).
Cell-value semantics gathered so far: `> 0x1a` = solid/wall (rendered);
`0x1b`, `0x40`, `0x43`, `0x48`, `0x4b` are distinct cell types transformed by
the post-processor; high bit (0x80) is a per-cell flag masked off on read.
- The generator + the post-processor's caller are reached via computed/indirect
  calls (no static callers — same jump-table wall as the Liberation m68k binary),
  so the level-setup chain needs data-flow or dynamic tracing to pin down.

## Realistic path
The map STRUCTURE and cell semantics are now substantially recovered. Remaining
for a native map source: pin the generator (dynamic trace, or emulate the
level-init), reimplement it in C, then state/movement/monsters/combat and wire to
captive_view_window + compositor. This is a genuine multi-session engine build;
milestone 1 (map format + access + render + post-process) is largely mapped.

## Confirmed: dungeon is procedurally generated (no level data files)
The Captive DOS archive contains ONLY graphics (CAPICS/*.PL5: WALLA-E, DOORS1/2,
OBJECTS, ROOFS, ALIEN1-6, SHOP1/2, ICONS, GAMESCRN), animations (ANIMS/*.ANM),
sound (SOUND/*.MID), and the executables — NO level/map data files. So Captive's
dungeon (its famous ~65000 levels) is PROCEDURALLY GENERATED from a seed, not
stored. Consequence for the native engine:
- The wall/door/object GRAPHICS are in the archive and read directly (good).
- The MAP LAYOUT must come from reimplementing CAPPO's procedural generator,
  which is behind computed dispatch (no static caller) -> requires DYNAMIC
  tracing (run CAPPO under a debugger AS AN RE TOOL — never inside OpenCaptive —
  breakpoint the map post-processor FUN_4661 / the map fill, and backtrace the
  level-init to recover the generation algorithm), then reimplement in C.
This is the concrete multi-session path; milestone 1's static analysis is done
(map format, cells, access, render, post-process, and generation-is-procedural).

## Dynamic-tracing environment ready (for the generator pass)
DOSBox-X 2026.07.02 (at /opt/homebrew/bin/dosbox-x) has the heavy debugger
(`-break-start` breaks into the interactive debugger at startup). This is an RE
TOOL only — never shipped inside OpenCaptive. Captive DOS files extracted for RE
at /Volumes/Extern-disk/opencaptive-re/captive_run/ (CAPPO.EXE + CAPICS/*.PL5).
Generator-trace recipe (next pass): launch CAPPO under dosbox-x -break-start,
set a memory-write breakpoint on the map at DS:7CB3 (BPM), continue, drive CAPPO
past character creation into the first dungeon so a level generates, and when the
breakpoint fires read the code address writing the map -> the procedural
generator; backtrace to the level-init/seed. Then reimplement in C from the
recovered algorithm. This is an interactive, multi-turn RE pass; environment and
recipe are set up so it can be driven directly.

## Dynamic-trace attempt blocked (2026-08-14)
Attempted the generator trace: launched CAPPO under DOSBox-X (RE tool, via a
.app wrapper so computer-use can observe it) and tried to run it. Obstacle:
synthetic keyboard input to DOSBox-X does not register reliably via computer-use
(typed characters do not reach the emulator; only bare Return gets through), so
CAPPO cannot be driven to a level, and the interactive debugger cannot be
commanded (BPM etc.). So BOTH paths to the generator are currently blocked for
autonomous execution: static = computed-dispatch wall (no caller); dynamic =
can't drive the emulator's keyboard. Options to unblock next pass: (a) DOSBox-X
with a scripted/mapper input file or a debugger command script (-c / a debugger
startup script) instead of live keystrokes; (b) the user drives CAPPO to a level
and I read the memory/backtrace; (c) heavier static analysis of the level-init
dispatch table. Milestone-1 static analysis stands complete regardless.

## BREAKTHROUGH: generator IS statically recoverable (2026-08-14, supersedes "blocked")
The earlier "computed-dispatch wall / needs dynamic tracing" conclusion was
WRONG. A byte-search of the unpacked image (CAPPO_UNP.EX) for real 16-bit loads
of the map address 0x7cb3 (patterns `bf b3 7c` mov di, `be b3 7c` mov si,
`81 c3 b3 7c` add bx, `8d bf b3 7c` lea di) — as opposed to the false-positive
`b3 7c` = `mov bl,0x7c` — pinpoints every map-access site directly. No dynamic
trace, no emulator, no debugger needed. Key facts recovered statically:

- Memory model = SMALL. Code seg loads at CS=0 (file 1024). DGROUP (data seg) is
  at paragraph 0x0e3f: startup does `mov ax,0x0e3f; mov ds,ax` at many sites.
  So a DS-relative datum at offset O lives at CAPPO_CODE.bin[0x0e3f0 + O]
  (CAPPO_CODE.bin = CAPPO_UNP.EX[1024:]). This is how to read all data tables.
- Map: DS:0x7cb3, 64x32 = 2048 bytes (matches cx=0x800 loop below).
- Direction delta tables (read from DGROUP, REAL values):
    DX @DS:0x5e18 (8 signed words) = [0,-1, 0, 1, -1, 0, 1, 0]
    DY @DS:0x5e20 (8 signed words) = [-1, 0, 1, 0, 512, 2054, 6, 520]
  Directions 0..3 are the cardinal steps: 0=N(0,-1) 1=W(-1,0) 2=S(0,1) 3=E(1,0).
- Generator/walker function tree (code offsets in CAPPO_CODE.bin, CS=0):
    0x4749  index calc: cx=word[0x931a] (packed pos: low=x, high=y);
            bx = (y<<6)+x  -> current map cell index.
    0x4764  step: bl=byte[bx-0x7b2d] (a direction/step table), dir = bl&7;
            dir==5 -> call 0x4972 (special); dir==4 -> jump 0x4786 (special);
            else cx += DX[dir], dx += DY[dir]  (walk one cell).
    0x46cc  top loop: increments word[0x931c]; runs 13 iterations (cmp 0x0d),
            each calling 0x4749 then 0x4764 (build a corridor/segment), with a
            sign check that sets word[0x931e]=3 and word[0x92f6]=3 on wrap.
    0x4661  post-processor (already documented): cx=0x800 cells, `and al,0x7f`,
            0x1b -> +5, {0x40,0x43,0x48,0x4b} -> or 0x10.
    0x4707  applies 0x4736 to a plus/neighbourhood pattern (cx/bx +/-1, +/-0x40).
- Runtime-computed seed vars (0 in static image): word[0x931a] (pos),
  word[0x931c] (iter counter), word[0x8d79], byte[0x8cf5]/[0x8cf7]. The seed
  SOURCE (fixed constant vs level number vs timer) is the one remaining unknown
  for exact reproduction; the generation ALGORITHM itself is now recoverable
  purely statically by continuing to disassemble 0x4749/0x4764/0x46cc/0x4972 and
  the step table at DS:(bx-0x7b2d).

REMAINING static-RE steps (tractable, no emulator): (1) disassemble 0x4972 and
0x4786 (the dir 4/5 special cases); (2) read the step/shape table the walker
indexes at DS:(bx-0x7b2d); (3) find the caller of 0x46cc = the "generate level"
entry and where the seed vars are initialised; (4) transcribe to C as a
deterministic generator, then wire map -> captive_view_window + compositor.

## Generator algorithm recovered: throttled drunkard's walk (2026-08-14 cont.)
Full mechanics of the map-fill are now recovered statically:
- 0x46cc = one generation STEP, throttled: inc word[0x931c]; while <= 0x0d just
  return (acts once per 14 calls, so the maze grows incrementally over frames).
  On the acting call: call 0x4749 (index of current pos word[0x931a]), call
  0x4764 (walker: pick a direction from the step table, advance). If the walk
  went out of range (dx sign) it sets the mode flags word[0x931e]=3 /
  word[0x92f6]=3; otherwise it commits the new packed pos to word[0x931a] and
  stamps a plus-brush around it.
- 0x4736 = the cell writer: bounds x<=0x3f, y<=0x1f; then
  map[i] = (map[i] & 0x80) | 0x26  (force low7 = 0x26, keep marker bit).
- 0x4706..0x472a = the plus-brush: stamps centre, W(x-1), E(x+1), N(y-1),
  S(y+1) via 0x4736 (index deltas -1, +1, -0x40, +0x40).
- 0x26 is a DRAWN cell (0x26 > 0x1a), confirmed against the holomap renderer
  0xBAF4 which scans all 64x32 cells and draws every cell with value > 0x1a
  (triple-row video write, stride 0xA0). So the walk lays down structure.
- On-enter cell dispatch (0x478c, NOT generation) reads the cell under the
  player (0x2aba index from DS:5E80/5E82, mask 0x7f) and branches on codes
  0x30/0x2e/0x32/0x33/0x35/0x1e/0x60/0x22/0x36; code 0x36 looks the player pos
  up in a 16-entry x 6-byte teleporter table at DS:0x7C2F and warps.

Transcribed to C (real data, tested): src/data/captive_dos_generator.c now has
captive_gen_carve_cell (0x4736) and captive_gen_carve_plus (0x4706..472a) plus
the earlier deltas/index/bounds/post-processor. test_captive_dos_generator
covers all of them.

STILL NEEDED for exact reproduction: the direction the walker reads at each
step comes from the step table byte[bx-0x7b2d] (DS:0x84D3, runtime-filled) and
the initial pos word[0x931a] — i.e. the RNG/seed feeding the walk. Next: find
where DS:0x84D3 and word[0x931a] are initialised (the "new level" entry), and
what seeds the RNG (level number vs timer). Then the C generator can reproduce
CAPPO's exact per-level maze.

## Data layout + the parallel direction field (2026-08-14 cont.)
The dungeon state is three parallel per-cell/lookup arrays in DGROUP (para 0x0e3f):
- DS:0x7CB3  the 64x32 map (2048 bytes) — cell values, walk stamps 0x26.
- DS:0x84D3  a 64x32 DIRECTION FIELD (2048 bytes, 0x20 past the map end). The
  walker (0x4764) reads byte[index + 0x84D3] (encoded as [bx-0x7b2d]) to choose
  its next step direction. This is the maze "flow field": whoever fills it IS
  the seed-bearing generator.
- DS:0x7C2F  16-entry x 6-byte teleporter/link table (on-enter code 0x36).
Walk start: 0x7148 sets word[0x931a] (walk pos) = word[0x5e95] (current pos),
word[0x931c]=0x64 (throttle primed to act at once), word[0x931e]=1 — i.e. a
"begin carving from here" trigger gated by dx in [0x5c,0x76] and byte[0x92cf]
== 0x1b.

Nothing in code segment 0 writes DS:0x84D3 by immediate (searched bf/bb/be d3
84 -> none), so the direction-field fill lives in another code segment (image is
>64KB / multi-segment) or uses computed addressing. THAT fill + its RNG seed is
the single remaining unknown for byte-exact per-level reproduction. Two ways to
close it next: (a) disassemble the later code segments (file offset >0x10000)
for the routine that stosb-fills a 2048-byte region at DS:0x84D3; (b) one memory
snapshot of DS:0x84D3 + word[0x931a] right after a level is entered, to validate
a reconstructed fill. Everything else in the map subsystem is recovered and, for
the carve/render/layout parts, transcribed to tested C in captive_dos_generator.c.

## New-level entry + state clear (2026-08-14 cont.)
Found the "new level" driver: 0x4284.
- 0x4284 -> call 0x49c8 (state clear): sets DS=ES=DGROUP(0xe3f), preserves
  words[0x9348/0x934a/0x934c], then `mov cx,0x3af2; xor al,al; mov di,0x5e6a;
  rep stosb` — ZEROS the whole dungeon-state block DS:0x5E6A..0x995C, which
  contains the map (0x7CB3), the direction field (0x84D3), the teleporter table
  (0x7C2F) and the position vars (0x5E80/0x5E95). Then seeds a few bytes
  (DS:0x8DBD..0x8DC0 = 3,2,1,0; word[0x8DC3]=1) and copies ROM tables via 0x4B42
  (di=0x8DC7<-si=0x9AF6, di=0x8ED5<-si=0x9B0C, ...).
- After the clear, 0x4284 runs a long init fan-out: 0xE44, 0xE99, 0x435D
  (overlay/gfx load), 0xED7, 0x5500, 0x59F8, 0x4B83 (var clear), 0x5035, 0x69FD,
  0x643B, 0x63D7, 0xBC26, 0x851E, 0x44C4, 0x53C9, 0xA220, 0x432E, ... The maze
  builder (the routine that populates the direction field 0x84D3 with the
  seeded walk directions, then the map) is one of these calls.
NEXT: walk the 0x4284 fan-out to find the routine that writes DS:0x84D3 (the
seeded maze), OR snapshot DS:0x5E6A..0x995C once after a level is entered to
capture the real map+field for validation. captive_dos_generator.c already
reproduces the carve/render/layout faithfully; only the seeded direction-field
fill remains for byte-exact per-level dungeons.

## CORRECTION: role of 0x46cc is unconfirmed (2026-08-14) — honesty note
Do NOT over-read the earlier "drunkard's-walk GENERATOR" framing. New evidence:
- The map (0x7CB3) and field (0x84D3) are filled by NO stosb and NO absolute
  write in code segment 0 (a full opcode scan for mov [imm16] and rep stos into
  [0x7CB3..0x8CD3] finds only 4 single-cell pokes at 0x83C3/0x83FA/0x88B8).
  So the map is only ever written cell-by-cell by the stamp writer 0x4736.
- 0x46CC and 0x4749 are entries in FUNCTION-POINTER DISPATCH TABLES in DGROUP
  (0x46CC at DS:0xCAFD; 0x4749 in six table slots at DS:~0xAE09.. etc). They are
  dispatched by type/index, not called linearly from the new-level driver.
Therefore 0x46CC is a table-dispatched handler whose ROLE (level generator vs.
per-entity/creature walk that reveals-or-carves as it moves vs. a cell-type
handler) is NOT yet confirmed. What IS confirmed and faithfully transcribed:
the map-mutation PRIMITIVES — stamp writer 0x4736 (map[i]=(map[i]&0x80)|0x26,
bounds 64x32), plus-brush 0x4706..472A, post-processor 0x4661, cardinal deltas
DS:0x5E18/0x5E20, index (y<<6)+x, and the holomap wall test (>0x1A at 0xBAF4).
src/data/captive_dos_generator.c transcribes those primitives correctly; it does
NOT assert they constitute the whole level generator. Confirming 0x46CC's role
needs either resolving the DGROUP dispatch table at DS:0xCAFD (what indexes it)
or one runtime observation. This correction supersedes the "generator algorithm
recovered" heading's implication that 0x46CC is definitively the level builder.
