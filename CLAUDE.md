# OpenCaptive - Project Guide

## What is OpenCaptive?

A modern reimplementation of "Captive" (1990, Antony Crowther / Mindscape) — a sci-fi dungeon crawler often considered the best Dungeon Master clone. Supports DOS, Atari ST, and Amiga original game data with both original-faithful and enhanced rendering modes.

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
./build/opencaptive --data <path-to-captive-data> [--enhanced] [--platform dos|atari|amiga] [--scale N]
```

## Project structure

```
src/
  engine/     - Game logic, map generator, encounters, droids
  data/       - File format decoders (PL5, ANM, MID, Atari ST disk, Amiga ADF)
  render/     - SDL3 renderer, viewport, UI
  audio/      - Sound and music playback
include/      - Public headers
tests/        - Test sources
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

- **PL5**: DOS graphics (CAPICS/*.PL5) — planar image format, 16-color
- **ANM**: Animations (ANIMS/*.ANM) — frame-based animation
- **MID**: Music (SOUND/*.MID) — MIDI sequences
- **ST disk image**: Atari ST version
- **ADF**: Amiga disk images

## Reference

- Game info: https://captive.atari.org/
- Technical docs on the site cover: map generator, view rendering, internal graphics, sounds, encounters
