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

### Controls

| Key | Action |
|-----|--------|
| W / Up | Move forward |
| S / Down | Move backward |
| A / Left | Turn left |
| D / Right | Turn right |
| Q | Strafe left |
| E | Strafe right |
| Space | Attack (selected droid) |
| F | Interact (puzzles, doors, generators, shops) |
| I | Droid inventory/equipment screen |
| T | Terminal (map, status, mission info) |
| M | Toggle minimap overlay |
| 1-4 | Select droid |
| , / . | Go up/down stairs |
| F5 | Quick save |
| F9 | Quick load |
| F10 | Runtime graphics and cheat options |
| Escape | Return to menu |

### Command-line options

| Flag | Description |
|------|-------------|
| `--data <path>` | Path to game data (required for original assets) |
| `--game captive\|liberation` | Select which game to play (default: start menu) |
| `--platform dos\|atari\|amiga` | Select Captive platform mode (default: `dos`) |
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
| Amiga | `.adf` disk images (5 disks) | OFS filesystem, IFF FORM/ANIM, RNC |
| Amiga CD32 | `.bin/.cue` disc image | ISO9660+CDTV, CD audio tracks 2-11 |

## Project structure

```
src/
  engine/     Game logic, map generator, combat, shop, inventory, save/load
  data/       Format decoders: PL5, ANM, RNC, ADF, ISO9660, FAT12, MIDI
  render/     SDL3 renderer, 3D viewport, HUD
  audio/      8SVX/8-channel sound mixer, MIDI
include/      Public headers
tests/        Test suite
tools/        Standalone utilities (pl5_to_bmp, anm_extract, rnc_decode)
docs/         Format documentation
```

## Status

| Feature | Status |
|---------|--------|
| SDL3 renderer + framebuffer | Done |
| PL5 graphics decoder (32-color, 5-bit packing) | Done |
| ANM animation decoder (XOR delta, LE frame sizes) | Done |
| RNC Method 1 decompressor | Done |
| Start menu (game selection) | Done |
| 3D viewport (back-to-front, 4-depth) | Done |
| HUD (droid status, minimap, compass) | Done |
| Procedural map generator (rooms, corridors, doors) | Done |
| Combat system (creatures, AI, attacks, leveling) | Done |
| Item/weapon database (24 weapons, 12 armor, ammo) | Done |
| Shop system | Done |
| Sound engine (8SVX, 8-channel mixer) | Done |
| Save/load game state | Done |
| Atari ST disk reader (FAT12) | Done |
| Amiga ADF reader (OFS/FFS) | Done |
| ISO9660 reader (CD32 BIN/CUE) | Done |
| PL5 texture loading from game data | Done |
| Intro cutscene playback (ANM) | Done |
| CI/CD (Linux, macOS, Windows) | Done |
| GitHub Actions release workflow | Done |
| Liberation: Captive 2 format analysis | Done |
| MIDI music (software synth, 32-voice) | Done |
| Puzzle systems (buttons, levers, power sockets) | Done |
| Textured viewport (walls, floors, doors) | Done |
| Creature rendering in viewport | Done |
| Droid inventory/equipment UI | Done |
| Terminal system (map, status, mission) | Done |
| Game over / victory states | Done |
| Mission progression (10 missions) | Done |
| Gold economy (drops, shop, HUD display) | Done |
| Settings menu (graphics, audio, scale) | Done |
| Liberation: Captive 2 city engine | Done |
| Liberation: building interiors | Done |
| Liberation: city/building navigation | Done |
| Enhanced rendering mode (2x SSAA) | Done |
| Procedural sound effects (10 types) | Done |

## Game data formats

### PL5 (Captive DOS graphics)

40000 bytes = 320x200, 32 colors. Custom 5-bit packing: every 5 bytes encode 8 pixels with a non-standard bit arrangement (not a simple bitstream). Palette: 32 ARGB8888 colors extracted from ANM file headers.

### ANM (Captive DOS animations)

768-byte VGA palette (256 colors, 6-bit RGB), then LE word at offset 768 = `cmd_end` offset, then commands section, then frames stored backwards from EOF. Each frame ends with an LE word = total size (including the 2-byte size field). Frame data uses XOR-delta encoding: byte != 0 → XOR at position, byte == 0 → next byte is skip count (0 = end of frame). Frame buffer is 64000 bytes (chunky 8-bit).

### RNC Method 1 (Rob Northen Compression)

Used by Atari ST and Amiga versions. Header: "RNC\x01", then BE32 uncompressed size, BE32 compressed size, CRC16 fields. Huffman-coded with three tables per sub-block.

### ADF (Amiga Disk File)

880KB floppy image. OFS or FFS filesystem. 80 tracks × 2 sides × 11 sectors × 512 bytes.

### Atari ST disk image

FAT12 filesystem. 720KB-880KB. Standard BPB at offset 11.

### ISO9660 (CD32)

Standard ISO9660 with CDTV extension. MODE1/2352 raw sectors. Volume ID "Liberation_1", mastered 1994-04-15 with ISOCD by Pantaray.

## References

- [The Ultimate Captive Guide](https://captive.atari.org/) — comprehensive fan site with technical documentation
- [Captive on Wikipedia](https://en.wikipedia.org/wiki/Captive_(video_game))
- [Liberation: Captive 2 on Wikipedia](https://en.wikipedia.org/wiki/Liberation:_Captive_II)
- CaptiveTools.jar by oFF_rus — reference PL5/ANM decoder

## License

MIT License — see [LICENSE](LICENSE).
