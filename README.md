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

Movement, combat, saves and mission commands are intentionally disabled. The
previous implementation generated its own Captive state and was not derived
from verified original data.

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

| Feature | Status |
|---------|--------|
| SDL3 renderer + framebuffer | Done |
| PL5 graphics decoder (32-color, 5-bit packing) | Done |
| ANM animation decoder (XOR delta, LE frame sizes) | Done |
| RNC Method 1 decompressor | Done |
| Start menu (game selection) | Done |
| Original Captive viewport visibility analysis | Done; original panel compositor remains unrecovered |
| Verified original Captive HUD shell | Done |
| Generated map, combat, inventory and shop prototypes | Kept out of runtime; not parity evidence |
| Sound engine (8SVX, 8-channel mixer) | Done |
| Save/load game state | Done |
| Atari ST disk reader (FAT12) | Done |
| Amiga ADF reader (OFS/FFS) | Done |
| ISO9660 reader (CD32 BIN/CUE) | Done |
| PL5 texture loading from game data | Done |
| Intro cutscene playback (ANM) | Done |
| CI/CD (Linux, macOS, Windows) | Done |
| GitHub Actions release workflow | Done |
| Liberation: Captive 2 hash verification and format analysis | Done |
| MIDI music (software synth, 32-voice) | Done |
| Original Captive map, state and input command decoding | In reverse engineering |
| Original Captive viewport compositor | In reverse engineering |
| Original Captive encounters in viewport | In reverse engineering |
| Captive generated map/combat/shop/save implementation | Disabled pending original state recovery |
| Settings menu (graphics, audio, scale) | Done |
| Liberation: Captive 2 original presentation playback | Verified first frames |
| Liberation: original city, plot and dialogue logic | In reverse engineering |
| Liberation: city/building interaction | Not implemented from inferred state |
| Generated enhanced dungeon rendering | Removed pending original compositor |
| Procedural sound effects | Disabled; synthetic effects are not parity evidence |

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
