# OpenCaptive

A modern, open-source reimplementation of **Captive** (1990) by Antony Crowther, published by Mindscape.

## About the game

Captive is a sci-fi first-person dungeon crawler set in space. You play as a prisoner trapped in a cell, controlling four battle droids remotely through a briefcase computer (the ABCC 500XL). Your droids must fight through procedurally generated space bases across different planets, battling alien encounters and solving puzzles to ultimately secure your freedom.

The game was released for the Atari ST, Amiga, and DOS, and is widely considered one of the best Dungeon Master clones ever made. Despite using only 16 colors on the Atari ST, it delivered rich 3D environments with a huge variety of weapons, gadgets, droid upgrades, and interactive elements. Its procedural map generator made the game essentially endless.

## What is OpenCaptive?

OpenCaptive is a faithful reimplementation of the Captive game engine in modern C with SDL3. It aims to:

- **Preserve the original experience** — pixel-perfect rendering, original game logic, and authentic gameplay using the original data files from DOS, Atari ST, and Amiga versions
- **Enhance where it makes sense** — optional higher-resolution rendering, modern display scaling, and quality-of-life improvements while keeping the original mode as the default
- **Run on modern systems** — native builds for macOS, Linux, and Windows via SDL3

This project does **not** include original game data. You need your own copy of Captive to play.

## Building

Requires CMake 3.20+, a C17 compiler, and [SDL3](https://github.com/libsdl-org/SDL).

```bash
cmake -S . -B build -DCMAKE_C_COMPILER=cc -G Ninja
ninja -C build
```

## Running

Point OpenCaptive at your Captive game data directory:

```bash
./build/opencaptive --data /path/to/captive/data
```

### Options

| Flag | Description |
|------|-------------|
| `--data <path>` | Path to Captive game data (required) |
| `--platform dos\|atari\|amiga` | Select which platform's data to use (default: `dos`) |
| `--scale <N>` | Window scale factor (default: `3`) |
| `--enhanced` | Enable enhanced rendering mode |

## Supported game data

| Platform | Files | Format |
|----------|-------|--------|
| DOS | `CAPICS/*.PL5`, `ANIMS/*.ANM`, `SOUND/*.MID` | Extracted game directory |
| Atari ST | `.st` disk image | ST floppy image |
| Amiga | `.adf` / `.zip` archives | ADF floppy images |

## Project structure

```
src/
  engine/     Game logic, procedural map generator, encounters, droids
  data/       File format decoders (PL5 graphics, ANM animation, MIDI, disk images)
  render/     SDL3 renderer, 3D viewport, UI overlay
  audio/      Sound effects and music playback
include/      Public headers
tests/        Test suite
docs/         Reverse engineering notes and format documentation
```

## Status

**Early development.** Current state:

- [x] Project scaffolding and build system
- [x] SDL3 renderer with original-resolution framebuffer
- [x] PL5 and ANM decoder stubs
- [ ] PL5 graphics decoder (DOS 16-color planar format)
- [ ] ANM animation decoder
- [ ] MIDI music playback
- [ ] 3D viewport rendering
- [ ] Procedural map generator
- [ ] Droid control and inventory system
- [ ] Encounter/combat system
- [ ] Shop system
- [ ] Atari ST disk image reader
- [ ] Amiga ADF reader
- [ ] Enhanced rendering mode
- [ ] Save/load game state

## References

- [The Ultimate Captive Guide](https://captive.atari.org/) — comprehensive fan site with technical documentation on the map generator, view rendering, internal graphics formats, and encounters
- Original game manual included gameplay mechanics and the ABCC 500XL briefcase computer interface

## License

TBD
