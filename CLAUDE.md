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
  engine/     - Game logic, map generator, encounters, droids
  data/       - File format decoders (PL5, ANM, MID, RNC, Atari ST disk, Amiga ADF)
  render/     - SDL3 renderer, viewport, UI
  audio/      - Sound and music playback
include/      - Public headers
tests/        - Test sources
tools/        - Standalone tools (pl5_to_bmp, pl5_analyze, rnc_decode)
docs/         - Format documentation, reverse engineering notes
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
- **PL5**: Graphics (CAPICS/*.PL5) — 40000 bytes, 320x200, 32 colors (5-bit custom packing: 5 bytes → 8 pixels). The bit arrangement is non-standard — not a simple MSB/LSB bitstream.
- **ANM**: Animations (ANIMS/*.ANM) — 768-byte VGA palette header (256 colors, 6-bit RGB) followed by compressed frame data. First 32 palette entries match the PL5 game palette.
- **MID**: Music (SOUND/*.MID) — standard MIDI sequences
- **CTV**: Sound driver configs (SOUND/*.CTV)

### Captive (Atari ST)
- FAT12 disk image, all FED* files are RNC Method 1 compressed
- Decompressed graphics are 32000 bytes (4 bitplanes, word-interleaved, 320x200)

### Captive (Amiga)
- ADF disk images, RNC Method 1 compressed data

### Liberation: Captive 2 (Amiga / CD32)
- Format analysis pending

## Reference

- Game info & technical docs: https://captive.atari.org/
- Technical pages: ViewRendering, InternalGfx, MapGen, Sounds, DrawingEncounters, FlyingItems, Holamap, FireHydrants
- CaptiveTools.jar by oFF_rus (old-games.ru) — reference PL5/ANM decoder
- Wikipedia: https://en.wikipedia.org/wiki/Captive_(video_game)
