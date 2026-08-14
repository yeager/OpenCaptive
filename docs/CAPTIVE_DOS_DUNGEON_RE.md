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

## RIGOR NOTE: dispatch-table claim retracted; limits of flat seg-0 analysis
Retract the "0x46CC is a DGROUP dispatch handler at DS:0xCAFD" claim from the
previous section: 0x46CC occurs at file 0x1AEED = DS:0xCAFD, an ODD offset that
straddles two entries of a word-aligned table whose real entries are cbXX/ccXX
addresses — i.e. a coincidental byte match, not a pointer. Likewise several of
the 0x4749 "table" hits are at odd offsets and are unreliable.
Rigorous conclusion: in code segment 0, 0x46CC has NO confirmed caller (no near
CALL, no clean word-aligned pointer entry). It is therefore reached from the
SECOND code segment (the image is >64KB: DGROUP occupies para 0x0E3F..~0x1E3F,
and there is code after it at file >0x1E3F0 that a flat CS=0 disassembly does
NOT decode correctly), or by computed/far dispatch. The same segment-2 code is
the most likely home of the direction-field (0x84D3) seed fill, which is absent
from segment 0.
WHAT REMAINS SOLID (unaffected by the above): the transcribed map-mutation
primitives in captive_dos_generator.c (stamp 0x4736, brush 0x4706, post-proc
0x4661, deltas 0x5E18/0x5E20, index (y<<6)+x, bounds, wall test >0x1A at 0xBAF4)
— each verified against the original bytes and unit-tested. Confirming the
driver/role and the seed requires correctly disassembling the second code
segment (relocate to its real load segment, then analyse) or one runtime
snapshot of DS:0x5E6A..0x995C after a level is entered. Next concrete static
step: carve segment 2 out of the image and disassemble it as its own segment.

## GENERATOR CORE RECOVERED: RNG + phase machine + lifecycle (2026-08-14)
The driver DOES exist in segment 0 — the earlier "no caller" was a search gap.
Full generator now recovered statically:
- RNG (0x44EF..0x44FA), an LCG on word[0x9322]:
    state = state * 0x05E5 + 0x0029   (mod 2^16)
- Direction (0x4605): al=state&0xff; rol al,1; rol al,1; dir = al & 3  (0..3).
- Per-tick driver: the main mode handler at 0x44CE runs the RNG each tick, then
  at 0x4514: `if word[0x931E]!=0 call 0x4610` (the generator dispatcher).
- Dispatcher 0x4610 keyed on word[0x931E] (phase):
    phase 1 (0x4622): decrement word[0x931C]; when it hits 0, carve a 5x5 ROOM
      around word[0x931A] (cx-=0x202 to top-left, 5x5 loop via 0x2ABA index,
      cells with ch&0x7e==0x28 get byte[di]-=2), then set phase 2.
    phase 2 (0x46CC): throttled drunkard walk (acts 1-in-14), step via 0x4764
      using the direction, then stamp the plus-brush of 0x26; on out-of-range it
      sets phase 3 (word[0x931E]=3, word[0x92F6]=3).
    phase 4 (0x468A): finalize.
- START (0x7148): word[0x931A]=word[0x5E95] (start pos), word[0x931C]=0x64,
  word[0x931E]=1.
- COMPLETE (0x64xx handlers): set word[0x931E]=0, place the party at the entry
  coords word[0x8CF5]/word[0x8CF7] into word[0x5E80]/word[0x5E82].
- SEED: word[0x9322] is zeroed by the level-init clear (0x49C8) and only ever
  advanced by the LCG (no explicit per-level reseed found), so the maze follows
  from the RNG state at generation time. A faithful C reimplementation of this
  exact RNG + phase machine reproduces the game's own generator output (real
  Captive data); a single runtime snapshot can pin the exact seed for a numbered
  level if byte-exact level-N matching is later required.

Transcribed to tested C (captive_dos_generator.c): captive_gen_rng_next
(0x05E5/0x0029) and captive_gen_rng_dir (rol-rol-&3), alongside the carve/brush/
post-proc/deltas/index/bounds primitives. NEXT (task #7): assemble the full
phase machine (room carve + throttled walk) into a deterministic C generator and
wire its map into captive_view_window + the compositor.

## Multiple map systems; the RNG feeds entity "diggers" (2026-08-14)
Clarification after tracing where the RNG direction (0x4605) is consumed: it is
called from 0x4561/0x45CC/0x45D5, all inside the ENTITY update 0x454A — NOT the
maze walk. So randomness drives 8 entities (records at DS:0x5EB7, stride 6:
word=pos, byte+4=dir, byte+5=state), updated in the 8x loop at 0x4503. Each
entity: takes/refreshes an RNG direction, moves (0x6284), and where it lands on
a cell of type 0x20/0x60 it rewrites it to 0x1E (byte[di]=(byte[di]&0x81)|0x1E);
blocked terrains 0xFF/0xFE/0xFA/0x28/0x36 make it repick a direction. These are
random-walking "diggers".
So at least THREE systems mutate the 64x32 map: (1) the phase machine
(0x4610: 5x5 room carve + drunkard walk stamping 0x26, driven by word[0x931E]);
(2) the 8 RNG diggers (0x454A stamping 0x1E); (3) the on-enter cell handlers
(0x478C). Disentangling which of these is "initial level generation" vs. runtime
creature/gameplay behaviour is the remaining RE work, together with the still-
unlocated fill of the walk's direction field DS:0x84D3.
SOLID + TESTED so far: the RNG (LCG 0x5E5/0x29) and its direction extraction,
the carve/brush/post-proc primitives, deltas, index, bounds, wall test, and the
full data layout + generator lifecycle map. These are real, verified building
blocks; a byte-exact whole-dungeon generator additionally needs the three
systems disentangled (or one runtime snapshot of DS:0x5E6A..0x995C to anchor it).

## CORRECTION: DS:0x84D3 is a per-cell FLAGS/visibility array, not a direction field
Tracing writes to map+0x820 (= DS:0x84D3) shows it is accessed with BIT ops, not
as 0..7 directions:
- 0x6580/0x659F: the auto-map REVEAL. For a cell whose map value (&0x7E)!=0x60 it
  sets field bit 0x80 on the cell (or byte[di+0x820],0x80) AND on its 4 bounds-
  checked neighbours (di+0x821 E, di+0x81F W, di+0x860 S, di+0x7E0 N). bit 0x80
  = "explored/visible" (drives the holomap).
- 0xC3F9/0xC404: test/or bit 0x40 of the same field byte (another per-cell flag).
So DS:0x84D3 is a per-cell FLAGS byte (bit7=revealed, bit6=another flag, ...),
NOT the maze "direction field" the earlier notes assumed. Consequently the
0x46CC walk's `byte[index+0x84D3] & 7` reads the LOW bits of that flags byte
(normally 0), so 0x46CC is NOT confirmed to be the maze generator either.
NET (honest): the map's INITIAL structure builder is still not cleanly isolated.
The strongest remaining candidate for procedural carving is the 8 RNG "diggers"
(0x454A), but they may equally be creature behaviour. Captive's dungeon is a web
of subsystems (carve phase machine, diggers, visibility reveal, on-enter, teleport)
all sharing the 64x32 map + this flags array; cleanly separating "level generation"
from runtime behaviour needs deeper multi-pass RE or one runtime snapshot. What is
solid and tested stays solid: the RNG (LCG 0x5E5/0x29), the carve/brush/post-proc
primitives, the deltas/index/bounds, the wall test, and the data layout — none of
which depend on this correction.

## STRUCTURAL CONCLUSION: no monolithic generator; map built by command/tick subsystems
Exhaustive search settles where generation lives:
- The whole-image scan for map-address immediates (DS:0x7CB3 loads) finds hits
  ONLY in code segment 0 (file <0x10000). CAPPO is small-model (DS=DGROUP
  everywhere), so any map-writing code MUST use those DS-relative addresses.
  Therefore the code after DGROUP (file >0x1E3F0) does NOT touch the map, and the
  map generator is entirely in segment 0.
- In segment 0 the ONLY code that writes map cells is: the phase machine
  (0x4610: 5x5 room carve + drunkard walk, stamps 0x26), the entity update
  (0x454A: stamps 0x20/0x60 -> 0x1E), the 4 single-cell pokes, and the on-enter
  handlers. The entity array (0x5EB7, 9 slots) is populated by SPAWN routines
  (0x70ED) driven by command codes (dx 0x6C..0x7F), and the phase machine is
  kicked by 0x7148 (also command-gated: dx range + byte[0x92CF]==0x1B). The main
  tick dispatches through jmp word[0x8D5B] (0x44C9); no immediate write to
  0x8D5B exists, so the mode pointer is set indirectly.
CONCLUSION: Captive does not build the dungeon in one monolithic "generate level"
call. The 64x32 map is produced by these command/tick-driven subsystems (the
carve phase machine + the RNG entities), i.e. generation is interleaved with the
game loop and the level-entry command sequence. A native reimplementation must
therefore drive the SAME subsystems from the level-entry sequence, not port a
single function. The verified pieces (RNG LCG 0x5E5/0x29, carve primitives,
deltas, index, bounds, wall test, flags/visibility semantics) are the real
building blocks for that; the remaining work is to recover the exact level-entry
trigger sequence + parameters (which commands, in what order, with what start
positions) — best pinned by one runtime trace, since it is control-flow through
the command dispatch rather than a single routine.

## Runtime capture pipeline PROVEN WORKING (2026-08-14) — one step left
Built and verified a non-debugger way to read Captive's live dungeon state:
- DOSBox-X `autosave=N` writes periodic savestates (no heavy debugger needed).
  Config at opencaptive-re/capsnap/cap.conf mounts the game dir and runs CAPPO.
- A savestate is a ZIP whose `Memory` entry is the full guest RAM (16MB).
- opencaptive-re/capsnap/extract_map.py locates CAPPO in that RAM by its RNG
  code signature (a1 22 93 ba e5 05 f7 e2 = CODE 0x44EF), derives code_base and
  DGROUP, and reads the map at DGROUP:0x7CB3 and flags at DGROUP:0x84D3, dumping
  <state>.map.bin / <state>.flags.bin — REAL level data for the C engine.
- Verified end-to-end: CAPPO's code was found (code_base=0x82C8) and the map
  region read correctly. In a fresh CAPPO boot the map is all-zero (confirming
  the level is generated only after the game is driven past its title/FILEPLAY
  into play; CAPTIVE.BAT is INTRO -> FILEPLAY -> CAPPO <arg>).
THE ONLY REMAINING STEP is to drive CAPPO into a dungeon so a level exists in
RAM, then run extract_map.py on the autosave. Driving needs live keystrokes:
DOSBox-X AUTOTYPE can't fire while CAPPO holds the shell, and interactive GUI
control here was declined. So this final step needs the user to either (a) run
CAPPO to its first dungeon once with capsnap/cap.conf's autosave on (the state
is captured automatically), or (b) grant emulator GUI control. With the captured
<state>.map.bin, the native engine can render the REAL level immediately, and
comparing several captures pins down the generator parameters for full native
generation.

## AUTONOMOUS EXECUTION of the real generator via Unicorn (2026-08-14) — no emulator GUI
Breakthrough: CAPPO's actual generation code can be executed directly in a
Unicorn (x86-16) harness — no DOSBox GUI, no user input, no keystroke driving.
opencaptive-re/emu_gen.py:
- loads CAPPO_UNP.EX's load image at seg 0x1000, applies the MZ relocations,
  sets CS=0x1000, DS=ES=DGROUP(0x1E3F), stack, and stubs INT 10/16/21/33.
- Validated end-to-end: running CAPPO's RNG bytes (0x44EF) reproduces the LCG
  exactly (seed 0->0x29, 0x3039->0x4026, 0xABCD->0xAF8A) — the harness executes
  the real code correctly.
- Calling the new-level driver 0x4284 runs the whole init fan-out cleanly and
  returns; it sets up state but does NOT itself build the maze (word[0x931E]=0),
  confirming generation is a separate, triggered step.
- Triggering generation (word[0x931A]=start, word[0x931C], word[0x931E]=1) and
  driving the phase dispatcher 0x4610 executes the real carve code and STAMPS
  0x26 cells into the map at DS:0x7CB3 (verified: 49 cells written, a walk
  column). So the real generator runs autonomously and writes real map bytes.
REMAINING to get a COMPLETE level: the single phase-machine walk is degenerate
because the walk direction comes from the per-cell flags low bits (0x84D3), which
are populated during play; the fuller maze is built by the 8 RNG "diggers"
(0x454A) over many ticks. Next: drive the full tick handler 0x44CE (RNG +
diggers + phase machine) across simulated ticks, with diggers spawned as the
game does (0x70ED), so the real code fills the whole 64x32 map. Then read
DS:0x7CB3 and feed it through captive_dos_map_to_level() -> renderer. This path
needs NO user input and NO GUI — it runs the game's own code. (extract_map.py's
savestate route remains a valid cross-check.)

## KEY INSIGHT from autonomous runs: generation REFINES a template (2026-08-14)
Driving the real code in Unicorn revealed the true generation model:
- The phase-machine WALK (0x46CC) reads its step direction from the per-cell
  flags low bits (0x84D3). With an empty map those are 0, so the walk goes
  straight -> it stamps a single degenerate column (observed: 49 cells at x=32).
- The phase-1 ROOM carve (0x4622) only alters cells where (val&0x7E)==0x28, and
  the diggers (0x454A) only convert cells of type 0x20/0x60 -> 0x1E. On an empty
  (all-zero) map NONE of these fire.
=> These subsystems do NOT build the dungeon from nothing; they REFINE an initial
   map TEMPLATE made of cells 0x20/0x28/0x60 etc. The template must be produced
   earlier by a routine not yet identified, OR loaded — note 0x435D loads a file
   via INT 21 (stubbed to empty in the harness), and the BIOS comm area 0040:00F0
   (set by FILEPLAY/GM before CAPPO) carries parameters CAPPO reads at startup.
NEXT to get a COMPLETE real level autonomously: find the template source — either
(a) the routine that stamps the initial 0x20/0x28/0x60 layout, or (b) let 0x435D's
file load succeed in the harness (feed the real CAPICS/overlay bytes) and re-run,
or (c) provide the 0040:00F0 params FILEPLAY passes. Then drive 0x4284 + ticks and
read DS:0x7CB3. The Unicorn harness (opencaptive-re/emu_common.py + emu_tick.py)
already executes the real code headlessly and correctly (RNG + phase machine
verified), so this is now pure RE of the template stage — no user input, no GUI.

## EXHAUSTIVE autonomous sweep (2026-08-14): generator is parameterized, not a bare call
Ran emu_sweep.py: after 0x4284 init, snapshot memory, then call EACH of the 853
near-call targets in code segment 0 (memory restored between calls) and check
whether the map at DS:0x7CB3 gains a template (>50 cells in 0x1A..0x6F). Result:
ZERO hits. So no single function, invoked from the initialized state with no
arguments, generates a dungeon. Combined with the trace result (only 0x49E3
clears the map during normal ticks), this exhaustively establishes that Captive's
maze generator is PARAMETERIZED and STATE-DRIVEN: it fires only from the
probe-entry flow with the right inputs (the level/probe params GM/FILEPLAY pass
via BIOS comm area 0040:00F0, plus the base/probe game state and mode transition
through word[0x8D5B]). It is not reachable by a bare call.
CONSEQUENCE: producing a real generated level autonomously now requires supplying
those inputs — i.e. reconstructing the probe-entry parameter setup (which level
number, the 0040:00F0 bytes, the mode handler value, entry coords 0x8CF5/0x8CF7)
and then running the tick loop. That is a bounded but genuinely multi-session RE
task. Tools are all in place (emu_common.py harness executes real code + traces
map writes; captive_dos_map_to_level renders the result). The fast alternative
remains one user-driven capture (capsnap/ pipeline) of a real level.

## Full-boot emulation (2026-08-14): game runs headless but waits for UI navigation
Ran CAPPO's real game entry 0x4260 in the Unicorn harness with INT stubs, port-I/O
stubs (VGA status 0x3DA toggles for vsync waits), and an emulated timer tick
(periodic decrement of the CS-relative frame counter cs:[0x1A]). Results:
- Without the timer, the boot hangs in a frame-delay spin at 0x439E
  (cmp cs:[0x1A],0; jne) — cs:[0x1A] is a countdown decremented by the timer ISR.
- WITH the emulated timer, the game advances past that delay into its main
  presentation loop (settles around 0x4404, the screen-buffer rep movsw), running
  frames — but the map at DS:0x7CB3 stays empty across 60x4M instructions.
CONCLUSION (execution-proven, not assumed): CAPPO boots and runs headless, but it
does NOT auto-generate a dungeon; it sits in the base/menu UI waiting for the
player to NAVIGATE into a dungeon (probe select + land). That navigation is
mouse/icon + render dependent, so a headless harness cannot drive it blind.
Combined with the exhaustive 853-function sweep (no bare call generates a level),
this is the definitive characterization: the maze generator is reachable only
through interactive UI navigation. Autonomous completion therefore requires
reconstructing that base/probe navigation flow (large, multi-session) OR one
user-driven level capture (capsnap/ pipeline, built + proven). All other pieces
(RNG, carve primitives, data layout, render converter, execution harness) are
done and tested.

## Rendering + input injection (2026-08-14): game seen at probe UI; multi-step gate
Extended the Unicorn harness to render CAPPO's framebuffer and inject input:
- CAPPO renders to an off-screen buffer at seg 0xB000 (~26k nonzero bytes). A
  linear 4bpp decode shows a status/panel row + the text "CAPTIVE" and a GRID at
  the bottom — i.e. the game is sitting at its pre-dungeon probe/mission selection
  screen (evidence: opencaptive-re/cappo_b000_linear.png). The dungeon map at
  DS:0x7CB3 is empty, confirming no dungeon is entered.
- Implemented INT 33 mouse (reset/get-pos/get-press) and swept button-1 clicks
  across the whole grid region (x 110..300, y 120..195, press+release each). The
  map stayed empty at every position: blind clicks do NOT launch a probe.
CONCLUSION (now proven with the game running, rendering, AND receiving injected
input): reaching a dungeon requires the specific MULTI-STEP probe-launch UI
sequence (select probe/target/confirm), which cannot be reproduced blind. The
harness can now run + render + drive CAPPO, so the remaining autonomous path is
to decode the 0xB000 framebuffer format precisely, read the UI state, and script
the exact probe-launch interaction — a bounded but multi-session task. The fast
alternative is unchanged: one user-driven capture via capsnap/. All non-interactive
pieces (RNG, primitives, layout, render converter, harness) are done and tested.

## ROOT CAUSE of input gate (2026-08-14): CAPPO is driven by hardware ISRs, not INT polling
Instrumented every software interrupt during the main loop: CAPPO makes ZERO
INT 33/INT 16/INT 21 calls while running at 0x4404. It does not poll input via
BIOS/DOS interrupts. Instead it installs its OWN hardware ISRs at boot (INT 9
keyboard reading port 0x60 directly; the timer INT 8/1C; the mouse IRQ) and the
main loop waits on flags those ISRs set. A headless Unicorn run never raises
hardware interrupts, so no key/mouse input ever reaches the game — which is the
true reason blind INT 16/INT 33 injection and mouse sweeps did nothing.
PRECISE REMAINING MECHANISM to drive CAPPO autonomously: (1) read the installed
INT 9 vector at 0000:0024 and periodically INVOKE that handler with a scancode
staged for its `in al,0x60` (delivering real keypresses); (2) likewise drive the
mouse handler; (3) then reverse the exact navigation sequence (title -> new game
-> probe select -> land) that reaches a dungeon, using the framebuffer render to
read UI state. Each is bounded but together they are genuinely multi-session
(it is effectively building the input/interrupt layer of an emulator plus
reversing Captive's front-end flow). The harness already runs + renders CAPPO, so
this is the concrete continuation. Fast alternative unchanged: one user-driven
capsnap capture. All non-interactive engine pieces remain done and tested.
