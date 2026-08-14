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
