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
