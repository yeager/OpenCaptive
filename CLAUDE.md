# OpenCaptive - Project Guide

## What is OpenCaptive?

A modern reimplementation of two classic sci-fi dungeon crawlers by Antony Crowther:

- **Captive** (1990, Mindscape) — supports DOS, Atari ST, and Amiga game data
- **Liberation: Captive 2** (1994, Mindscape) — supports Amiga and Amiga CD32 game data

Both games use original-faithful rendering with an optional enhanced graphics mode. A start menu lets the user choose which game to play.

## Build

```bash
cmake -S . -B build -DCMAKE_C_COMPILER=cc -G Ninja
ninja -C build
```

## Test

```bash
ctest --test-dir build -j4
```

## Run

```bash
./build/opencaptive --data <path-to-game-data> [--enhanced] [--platform dos|atari|amiga] [--scale N]
```

## Project structure

```
src/
  engine/     - Game logic: map_gen, combat, start_menu, inventory, shop, save_load, puzzle, droid_ui, terminal
  data/       - Format decoders: pl5, anm, rnc, gfx_loader, texture_atlas, adf_reader, iso9660, st_disk, mid_loader
  render/     - SDL3: renderer, viewport (3D with creatures), hud
  audio/      - Sound: 8SVX loader, 8-channel mixer, MIDI player, music system
include/      - Public headers (one per module)
tests/        - Test sources (pl5_decoder, anm_decoder, map_gen)
tools/        - Standalone: pl5_to_bmp, anm_extract, rnc_decode
docs/         - Format docs: PL5, ANM, map generation, weapons/items, viewport, Liberation
gamedata/     - Original game data (gitignored)
```

## Conventions

- Pure C, no C++. Use `cc` (clang), not gcc.
- Ninja generator for CMake.
- SDL3 for rendering and audio.
- Original game data never committed to git.
- Headers in `include/`, sources in `src/`, tests in `tests/`.

## Game data formats

### Captive (DOS)
- **PL5**: Graphics — 40000 bytes, 320x200, 32 colors (5-bit custom packing: 5 bytes → 8 pixels, non-standard bit arrangement). See `docs/FORMAT_PL5.md`.
- **ANM**: Animations — 768-byte VGA palette, LE cmd_end word, frames backwards from EOF, XOR-delta encoded. See `docs/FORMAT_ANM.md`.
- **MID**: Music — standard MIDI sequences
- **CTV**: Sound driver configs

### Captive (Atari ST)
- FAT12 disk image, FED* files are RNC Method 1 compressed
- Decompressed graphics: 32000 bytes (4 bitplanes, word-interleaved, 320x200)

### Captive (Amiga)
- ADF disk images (OFS/FFS), RNC Method 1 compressed data

### Liberation: Captive 2
- **Amiga**: 5 ADF disks, OFS, IFF FORM/ANIM chunks, RNC compressed
- **CD32**: ISO9660+CDTV, Volume "Liberation_1", 10 CD audio tracks (soundtrack)
- See `docs/LIBERATION_FORMATS.md`

## Implemented systems

| System | Module | Description |
|--------|--------|-------------|
| Start menu | start_menu.c | Game selection, keyboard navigation |
| Map generator | map_gen.c | Procedural rooms, corridors, doors, generators, shops |
| Viewport | viewport.c | Back-to-front 3D, 4-depth perspective |
| HUD | hud.c | 4 droid panels, HP/energy bars, minimap, compass |
| Combat | combat.c | Creature AI, damage/defense, leveling |
| Inventory | inventory.c | 24 weapons, 12 armor, ammo types |
| Shop | shop.c | Level-scaled stock, buy/sell |
| Sound | sound.c | 8SVX loader, 8-channel mixer |
| Save/load | save_load.c | Binary format, seed-regenerated maps |
| Puzzles | puzzle.c | Buttons, levers, triple-levers, power sockets |
| MIDI music | midi_player.c | Software square-wave synth, 32-voice polyphony |
| Music system | music.c | Track management, state transitions |
| Texture atlas | texture_atlas.c | PL5 wall/floor/door/icon sheets |
| Droid UI | droid_ui.c | Equipment, inventory, equip/unequip |
| Terminal | terminal.c | CRT-style map, status, mission briefing |
| Creature render | viewport.c | Colored sprites, HP bars in viewport |
| Game states | main.c | Game over, victory, mission progression |
| PL5 loader | gfx_loader.c | Loads PL5 textures from game data |
| ADF reader | adf_reader.c | Amiga OFS/FFS floppy images |
| ISO reader | iso9660_reader.c | CD32 BIN/CUE (MODE1/2352) |
| ST reader | st_disk_reader.c | Atari ST FAT12 disk images |

## Reference

- Technical docs: https://captive.atari.org/ (MapGen, ViewRendering, InternalGfx, Sounds)
- CaptiveTools.jar by oFF_rus (old-games.ru) — reference PL5/ANM decoder
- DMWeb: https://dmweb.free.fr/ (RNC compression reference)
