# OpenCaptive

**Current release: v1.1.79**

A modern C/SDL3 reimplementation of **Captive** (1990) and **Liberation: Captive 2** (1993) by Antony Crowther, published by Mindscape.

OpenCaptive is an actively verified reimplementation. Data formats, creature
stats, combat primitives, audio mappings, and parts of map/city generation are
validated against original binaries; full gameplay parity and the remaining
runtime paths are still under development.

## Downloads

Pre-built packages for all platforms are available on the [Releases](https://github.com/yeager/OpenCaptive/releases) page. Maintainers can also run the **Release** GitHub Actions workflow manually with a version such as `2.7.5`; tag-based releases remain supported.

| Platform | Package |
|----------|---------|
| Linux x86_64 | `.deb`, `.rpm`, `.AppImage`, `.tar.gz` |
| macOS arm64 | `.dmg` (app bundle) |
| Windows x64 | Inno Setup installer (`.exe`) |
| Android | APK (`.apk`) |
| iOS arm64 | Sideloadable IPA (`.ipa`) |

## About the games

### Captive (1990)

A sci-fi first-person dungeon crawler. You play as a prisoner controlling four battle droids remotely through a briefcase computer. Your droids fight through procedurally generated space bases across 10 planets, battling 25 creature types and solving puzzles to secure your freedom. Originally released for Amiga and Atari ST, later ported to DOS.

### Liberation: Captive 2 (1993)

The sequel expands into a cyberpunk city setting with hundreds of interactive buildings, detective-style gameplay, and on CD32 — voice acting and a CD-quality soundtrack. Developed by Byte Engineers. Features procedural city generation, 3D textured rendering, and a reputation system.

## Credits

- **Game design and programming**: Antony Crowther
- **Published by**: Mindscape International
- **Liberation developed by**: Byte Engineers
- **OpenCaptive**: Daniel Nylander

## Features

### Both games
- Hash-verified original data discovery and format decoding
- Original-resolution presentation paths; full gameplay parity remains under verification
- OPL2 FM synthesis (AdLib emulation) for music and sound effects
- Optional HQ MIDI output filter (`--hq-midi`)
- 19 languages (English, Svenska, Deutsch, Francais, Espanol, Italiano, and 13 more)
- Save/load with multiple slots
- Optional display enhancements: scanlines, CRT curvature, bilinear filtering, integer scaling
- Optional xBRZ framebuffer upscaling at 2x, 3x or 4x (`--hd-upscale`)
- Optional widescreen presentation (`--widescreen`), using a 16:9 output or
  the configured `widescreen_width`; native game frames remain unchanged
- Gamepad and keyboard controls

### Captive
- Prototype 10-mission campaign and procedural dungeon generation
- 25 creature types across 8 categories with documented source references
- 38-item weapon catalog with documented damage tables
- 63 MIDI music tracks across 14 categories
- 49 AdLib SFX sequences with bytecode interpreter (4 voices, 70 Hz)
- Intro cutscene playback (ANM format)
- Holamap planet selection between missions
- Droid configuration, body part damage, energy system

### Liberation
- Prototype procedural city generation (64x64 grid, 9 building types, 8 visual themes)
- 3D textured viewport prototype with perspective projection and z-buffer
- Prototype NPC dialogue, shops, bars, police and reputation systems
- Prototype day/night cycle, taxi system and industrial hazards
- Prototype building-interior flow and mission briefings

These generated gameplay systems are development prototypes, not claims of
original gameplay parity. The verified Liberation runtime boundary remains
original presentation playback while city, plot and dialogue state are being
reverse-engineered; Captive gameplay parity is likewise still under review.

### Start menu
- Game cards with SHA-256 data verification status (checkmark/cross)
- Continue game from existing saves
- Settings panel (24 options: display, audio, language, data path and gameplay)
- `WINDOW SIZE` provides presets; changing `SCALE` explicitly selects a custom
  native-size window and is preserved when launching from the menu
- Audio output can be set to 22,050, 44,100 or 48,000 Hz; the choice applies
  when the next game session starts and is shared by SFX and MIDI playback
- About/Credits screen
- Controls reference (F1)
- In-game F10 runtime popup: change display effects and toggle cheats in real time (God Mode and Infinite Energy work in both games)
- Data Scanner (D key) — reports verified game files per game
- Scanner results are cached by file identity and metadata, so unchanged files are not hashed again; the initial scan runs incrementally in the background, while D opens the visible progress view
- If multiple verified versions of a game are available, launch opens a localized version-selection popup
- With `cross_save=1` or `--cross-save-export`, Captive F5 saves also create a
  portable `.ocsv` file beside the normal save (`opencaptive.ocsv` or the
  matching quicksave slot).

## Building

Requires CMake 3.20+, a C17 compiler, SDL3, SDL3_ttf, and zlib.

```bash
cmake -S . -B build -DCMAKE_C_COMPILER=cc -G Ninja
ninja -C build
```

## Testing

```bash
ctest --test-dir build -j4 --output-on-failure
```

58 tests covering format decoders, game logic, combat, save/load, map generation, audio, rendering, UI, and release-version consistency.

## Running

```bash
./build/opencaptive --data ~/.opencaptive
```

### Default data paths

| Platform | Default path |
|----------|-------------|
| Linux / macOS | `~/.opencaptive` |
| Windows | `<install dir>\data` |

Place your game data files (ZIP archives, loose files, ADF disk images) in the data directory. Files are identified by SHA-256 content hash — filenames don't matter. Nested ZIPs, ADF disk images, and ISO tracks are scanned automatically. The scanner reuses cached results for unchanged files and rescans only files whose size, timestamp, or identity changed.

### Command-line options

| Flag | Description |
|------|-------------|
| `--data <path>` | Path to game data directory |
| `--game captive\|liberation` | Start specific game directly |
| `--platform dos` | Captive runtime platform (the current playable renderer) |
| `--scale <N>` | Window scale factor, 1-5 (default: `3`) |
| `--lang <code>` | Language code (en, sv, de, fr, es, it, ja, ko, zh, ...) |
| `--fullscreen` | Start in fullscreen mode |
| `--verify-data captive\|liberation` | Verify game data and exit |
| `--hd-upscale` | Apply xBRZ upscaling to native game frames |
| `--upscale-factor <N>` | xBRZ factor: 2, 3 or 4 (implies `--hd-upscale`) |
| `--widescreen` | Expand the game presentation horizontally to 16:9 |
| `--hq-midi` | Enable the enhanced MIDI output filter |
| `--replay-record` | Record Captive inputs to `opencaptive.ocrp` on exit |
| `--replay-output <file>` | Select the output file for replay recording |
| `--replay-play <file>` | Play back a recorded Captive replay |
| `--version` | Show version and exit |
| `--help` | Show help and exit |

### Controls

#### Movement
| Key | Action |
|-----|--------|
| W / Up | Move forward |
| S / Down | Move backward |
| A / Left | Turn left |
| D / Right | Turn right |

#### Combat and items
| Key | Action |
|-----|--------|
| Space | Fire weapon |
| Enter | Use item / Interact |
| Tab | Cycle droids |
| I | Inventory |

#### System
| Key | Action |
|-----|--------|
| F5 | Save game (also exports `.ocsv` when cross-save export is enabled) |
| F9 | Load game |
| F6 | Cycle save slots |
| M | Minimap |
| Shift+M | City map (Liberation) |
| H | Help screen |
| ESC | Pause menu |
| F1 | Controls reference |
| F10 | Runtime options popup: display effects, overlays, and cheats |

## Supported game data

### Captive

25 required files identified by SHA-256 content hash, including the two boot
resources and the complete first-person atlas. Supported sources:
- DOS version (CAPPO.EXE, PL5 graphics, CAP_A.BIN sound driver)
- Amiga ADF disk images (880KB) — verified by `--verify-data`; native Captive
  runtime loading is not yet exposed as a playable source
- Atari ST disk images — format support is available for analysis; no separate
  Atari runtime source is currently exposed
- ZIP archives (nested scanning supported)

### Liberation: Captive 2

7 required files identified by SHA-256 content hash. Supported sources:
- CD32 ISO 9660 data track (MODE1/2352)
- Amiga OCS/AGA floppy ADF images
- ZIP archives containing the above

## Project structure

```
src/
  engine/     Game engine (start menu, combat, save/load, inventory,
              shop, puzzle, map generation, droid UI, terminal, spawner)
  game/       Liberation runtime (city navigation, dialogue, shops,
              building interaction, combat, NPC dialogue, save)
  render/     SDL3 renderer, 3D viewport, HUD, holamap, compositor
  audio/      OPL2 emulator, AdLib SFX, MIDI player, music system
  data/       Format decoders (PL5, ANM, RNC, ArcD, CTV, AmSp, VGM,
              x3g, Img, FNT), VFS, SHA-256, i18n, ISO9660, ADF
  custom/     Optional features (replay, cross-save)
include/      Public headers
tests/        59 test source files (58 CTest targets)
docs/         Format documentation, disassembly notes
po/           Translation files (19 languages)
data/         Bundled fonts (DejaVu Sans Mono Bold)
assets/       Icons (SVG, ICO, ICNS)
tools/        Standalone utilities (hash_find, hash_extract)
```

## Reverse engineering

The following components have been cross-referenced against the original binaries:

| Component | Source | Method |
|-----------|--------|--------|
| Combat formulas | CAPPO.EXE at 0x5380 | Disassembly |
| Creature stats (25 types) | CAPPO.EXE DS:0xA1BF | Disassembly |
| Weapon damage (38 weapons) | CAPPO.EXE DS:0x9A42, 0x1A006 | Disassembly |
| SFX mappings (10 events) | CAPPO.EXE INT 61h sites | Disassembly |
| Combat PRNG | CAPPO.EXE at 0x9815 | Disassembly |
| MapGen cellular automaton | CAPPO.EXE at 0x39CC-0x3C21 | Disassembly |
| Generator placement | CAPPO.EXE at 0x1C3C | Disassembly |
| Item pricing | CAPPO.EXE at 0x1A220+ | Disassembly |
| CityGen grid (64x64) | Amiga CityGen 1.12 (10,824 bytes) | Disassembly |
| BuildingGen | Amiga BuildingGen (23,252 bytes) | Disassembly |
| PlotGen / ArcD | Amiga PlotGen at 0x302-0x520 | Disassembly |
| Game data assets | All platforms | SHA-256 content hash |

## Wiki

Full documentation is available on the [wiki](https://github.com/yeager/OpenCaptive/wiki):

- [User Guide](https://github.com/yeager/OpenCaptive/wiki/User-Guide) — Building, running, controls, settings
- [Developer Guide](https://github.com/yeager/OpenCaptive/wiki/Developer-Guide) — Code layout, architecture, contribution
- [Captive Technical](https://github.com/yeager/OpenCaptive/wiki/Captive-Technical) — DOS engine analysis, viewport, combat, MapGen
- [Liberation Technical](https://github.com/yeager/OpenCaptive/wiki/Liberation-Technical) — CD32 engine, CityGen, 3D rendering
- [File Formats](https://github.com/yeager/OpenCaptive/wiki/File-Formats) — PL5, ANM, RNC, ArcD, VGM, x3g, and more
- [Data Identity](https://github.com/yeager/OpenCaptive/wiki/Data-Identity-and-Verification) — SHA-256 verification system
- [Start Menu](https://github.com/yeager/OpenCaptive/wiki/Start-Menu) — localized version selection and launcher behavior
- [Internationalization](https://github.com/yeager/OpenCaptive/wiki/Internationalization) — 19-language translation workflow

## References

- [The Ultimate Captive Guide](https://captive.atari.org/) — comprehensive fan site
- [Captive on Wikipedia](https://en.wikipedia.org/wiki/Captive_(video_game))
- [Liberation: Captive 2 on Wikipedia](https://en.wikipedia.org/wiki/Liberation:_Captive_II)

## License

MIT License — see [LICENSE](LICENSE).
