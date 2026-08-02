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

OpenCaptive is a modern C/SDL3 reimplementation project for both game engines.
It currently presents hash-verified original Captive and Liberation imagery;
the original gameplay state, input commands and compositors are still being
reverse-engineered. It aims to:

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

### Current controls

| Key | Action |
|-----|--------|
| F10 | Runtime display options; cheat commands are pending original state recovery |
| Escape | Return to menu |

Movement, combat, inventory, terminal, save/load and F10 runtime controls are
available for the current Captive state. The viewport uses real PL5 panel sheets
with approximate projection geometry; the original panel compositor is still
being recovered from the DOS executable.

### Command-line options

| Flag | Description |
|------|-------------|
| `--data <path>` | Path to game data (required for original assets) |
| `--game captive\|liberation` | Select which game to play (default: start menu) |
| `--platform dos\|atari\|amiga` | Select Captive platform mode (default: `dos`) |
| `--scale <N>` | Window scale factor (default: `3`) |
| `--enhanced` | Accepted for configuration compatibility; Captive's reconstructed viewport is withheld until it uses the original compositor |

## Supported game data

### Captive

OpenCaptive discovers required assets recursively in loose data and supported
archives, then accepts each asset only when its SHA-256 content digest matches
the Captive manifest. The path and original filename have no bearing on the
match. Use `--verify-data captive` before starting the game.

### Liberation: Captive 2

The currently supported Liberation source is the known CD32 MODE1/2352 data
track whose outer digest and required ISO resources match the Liberation
manifest. Both the outer track and every required resource are selected by
SHA-256, not by filesystem path or original filename. Use
`--verify-data liberation` before starting it. Other Liberation editions are
not accepted as equivalent data yet.

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

### Core engine

| Feature | Status |
|---------|--------|
| SDL3 renderer + framebuffer | Done |
| Start menu (game selection, settings) | Done |
| CI/CD (Linux, macOS, Windows) | Done |
| GitHub Actions release workflow | Done |

### Captive — format decoders

| Feature | Status |
|---------|--------|
| PL5 graphics decoder (32-color, 5-bit packing) | Done |
| ANM animation decoder (XOR delta, LE frame sizes) | Done |
| RNC Method 1 decompressor | Done |
| CTV/VOC decoder (Creative Voice File) | Done |
| Atari ST disk reader (FAT12) | Done |
| Amiga ADF reader (OFS/FFS) | Done |
| PL5 texture loading from game data | Done |
| Intro cutscene playback (ANM) | Done |

### Captive — reverse engineering

| Feature | Status |
|---------|--------|
| PRNG recovery (4 variants from CAPPO.EXE) | Done |
| Combat system (hit check, damage formula, scaling) | Done |
| Creature stat tables (25 types, HP/speed/sprite) | Done |
| Weapon damage tables (18 melee, 20 ranged entries) | Done |
| Spawn placement algorithm (8 categories, subcell positioning) | Done |
| XP/level-up formulas (per-skill thresholds, caps) | Done |
| Item database (type codes, variant tiers, pricing) | Done |
| MapGen cellular automaton (4 rule types from CAPPO.EXE) | Done |
| Generator placement algorithm | Done |
| Viewport 19-cell trapezoid + HUD panel layout (9 blit rects) | Done |
| VGA palette verification | Done |
| Creature sprite rendering (ALIEN1-6.PL5) | Done |
| Object sprite rendering (OBJECTS.PL5) | Done |
| Holamap display (planet surface, base markers) | Done |
| Planet name generation | Done |

### Captive — sound

| Feature | Status |
|---------|--------|
| MIDI music (63 tracks, 14 categories, OPL2 FM synthesis) | Done |
| AdLib instrument patches (26 patches from CAP_A.BIN) | Done |
| SFX bytecode interpreter (13 opcodes, 4 voices) | Done |
| SFX event mapping (INT 61h call sites) | Done |
| Sound engine (8SVX, 8-channel mixer) | Done |
| Save/load game state | Done |

### Captive — remaining work

| Feature | Status |
|---------|--------|
| Original viewport panel compositor | In reverse engineering |
| Feature placement pipeline (doors, puzzles, traps) | In reverse engineering |
| MapGen byte-identical parity | In reverse engineering |
| Save format verification | In reverse engineering |

### Liberation: Captive 2

| Feature | Status |
|---------|--------|
| ISO9660 reader (CD32 BIN/CUE) | Done |
| Hash verification and format analysis | Done |
| Original presentation playback (intro + city ANIM) | Done — pixel-perfect |
| AMOS sprite bank decoder (AmSp) | Done |
| RNC Method 2 decompressor | Done |
| Amiga HUNK executable parser | Done |
| CD32 executable string tables (cities, NPCs, buildings) | Done |
| BuildingGen disassembly (23,252 bytes, PRNG, grid, 9 building types) | Done |
| CityGen grid disassembly (64×64 grid, 8×8 meta-grid, 13 tile templates) | Done |
| ArcD Huffman+LZSS decoder (PlotGen text compression) | Done |
| PlotGen text decompression (PGE.txt, DTE.txt, CTE.txt) | Done |
| PlotGen main algorithm (building interiors, plot state) | In reverse engineering |
| x3g 3D vector format | In reverse engineering |
| VGM wall texture format | In reverse engineering |
| City navigation and interaction | Not implemented |

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
