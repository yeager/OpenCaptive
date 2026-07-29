# OpenCaptive

A modern, open-source reimplementation of **Captive** (1990) and **Liberation: Captive 2** (1993) by Antony Crowther.

## About the games

### Captive (1990)

Captive is a sci-fi first-person dungeon crawler set in space. You play as a prisoner trapped in a cell, controlling four battle droids remotely through a briefcase computer (the ABCC 500XL). Your droids must fight through procedurally generated space bases across different planets, battling alien encounters and solving puzzles to ultimately secure your freedom.

Originally released for the Amiga and Atari ST, later ported to DOS. Published by Mindscape. Widely considered one of the best Dungeon Master clones ever made. Its procedural map generator made the game essentially endless.

### Liberation: Captive 2 (1993)

The sequel expands the concept into a cyberpunk city setting. Developed by Byte Engineers and published by Mindscape for Amiga and Amiga CD32. Features a vast explorable city with hundreds of interactive buildings, detective-style gameplay, and on CD32 — voice acting and a CD-quality soundtrack by Mark Knight.

## Credits

- **Game design and programming**: Antony Crowther
- **Published by**: Mindscape International
- **Liberation developed by**: Byte Engineers

## What is OpenCaptive?

OpenCaptive is a faithful reimplementation of both game engines in modern C with SDL3. It aims to:

- **Preserve the original experience** — pixel-perfect rendering, original game logic, and authentic gameplay using the original data files
- **Enhance where it makes sense** — optional higher-resolution rendering, modern display scaling, and quality-of-life improvements while keeping the original mode as the default
- **Run on modern systems** — native builds for macOS, Linux, Windows, and Steam Deck

A start menu lets you choose which game to play and configure graphics settings.

This project does **not** include original game data. You need your own copies to play.

## Building

Requires CMake 3.20+, a C17 compiler, and [SDL3](https://github.com/libsdl-org/SDL).

```bash
cmake -S . -B build -DCMAKE_C_COMPILER=cc -G Ninja
ninja -C build
```

## Running

Point OpenCaptive at your game data directory:

```bash
./build/opencaptive --data /path/to/captive/data
```

### Options

| Flag | Description |
|------|-------------|
| `--data <path>` | Path to game data (required) |
| `--game captive\|liberation` | Select which game to play (default: start menu) |
| `--platform dos\|atari\|amiga\|cd32` | Select platform data to use (default: auto-detect) |
| `--scale <N>` | Window scale factor (default: `3`) |
| `--enhanced` | Enable enhanced rendering mode |

## Supported game data

### Captive

| Platform | Files | Notes |
|----------|-------|-------|
| DOS | `CAPICS/*.PL5`, `ANIMS/*.ANM`, `SOUND/*.MID` | Extracted game directory |
| Atari ST | `.st` disk image | RNC compressed, 4-bitplane graphics |
| Amiga | `.adf` disk images | RNC compressed |

### Liberation: Captive 2

| Platform | Files | Notes |
|----------|-------|-------|
| Amiga | `.adf` disk images (5 disks) | Floppy version |
| Amiga CD32 | `.bin/.cue` disc image | CD audio tracks 2-11 are the soundtrack |

## Project structure

```
src/
  engine/     Game logic, procedural map generator, encounters, droids
  data/       File format decoders (PL5 graphics, ANM animation, RNC, MIDI, disk images)
  render/     SDL3 renderer, 3D viewport, UI overlay
  audio/      Sound effects and music playback
include/      Public headers
tests/        Test suite
tools/        Standalone utilities (pl5_to_bmp, rnc_decode)
docs/         Reverse engineering notes and format documentation
```

## Status

**Early development.** Current state:

- [x] Project scaffolding and build system
- [x] SDL3 renderer with original-resolution framebuffer
- [x] PL5 graphics decoder (DOS 32-color custom 5-bit packing)
- [x] PL5 game palette (32 colors, verified against original)
- [x] RNC Method 1 decompressor (Atari ST / Amiga data)
- [ ] ANM animation decoder (palette header parsed, frame decompression pending)
- [ ] MIDI music playback
- [ ] Start menu (game selection + settings)
- [ ] 3D viewport rendering
- [ ] Procedural map generator
- [ ] Droid control and inventory system
- [ ] Encounter/combat system
- [ ] Shop system
- [ ] Atari ST disk image reader
- [ ] Amiga ADF reader
- [ ] Amiga CD32 ISO reader
- [ ] Liberation: Captive 2 engine
- [ ] Enhanced rendering mode
- [ ] Save/load game state

## References

- [The Ultimate Captive Guide](https://captive.atari.org/) — comprehensive fan site with technical documentation
- [Captive on Wikipedia](https://en.wikipedia.org/wiki/Captive_(video_game))
- [Liberation: Captive 2 on Wikipedia](https://en.wikipedia.org/wiki/Liberation:_Captive_II)
- CaptiveTools.jar by oFF_rus — reference PL5/ANM decoder

## License

MIT License — see [LICENSE](LICENSE).
