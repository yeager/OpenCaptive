# Developer Guide

> Updated for v1.1.115. Release tags build and verify Linux, macOS, Windows, Android, and iOS artifacts in GitHub Actions.

## Build

```bash
cmake -S . -B build -DCMAKE_C_COMPILER=cc -G Ninja
ninja -C build
```

- **Pure C** (C17 standard). No C++ code.
- **Compiler**: `cc` (system clang on macOS). Do not use gcc.
- **Build system**: CMake 3.20+ with Ninja generator.
- **Dependencies**: SDL3, SDL3_ttf, zlib.
- **Platforms**: macOS (arm64), Linux (x86_64), Windows (x64).

Always test both Debug and Release configurations. Test targets compile with assertions enabled in Release builds.

## Test

```bash
ctest --test-dir build -j4 --output-on-failure
```

- 60 tests covering decoders, game logic, rendering, combat, save/load, localization catalogs, and release metadata.
- Some viewport tests require original game data files and will fail or timeout without them.
- Run a subset with `-R <pattern>`:

```bash
ctest --test-dir build -R "liberation" -j4 --output-on-failure
```

## Project Structure

```
src/
  engine/     - Game engine: start_menu, combat, save_load, inventory, shop,
                puzzle, droid_ui, terminal, map_gen, spawn, arcd_decoder,
                liberation_plotgen
  game/       - Liberation runtime: city_nav, dialogue, shop, building_interact,
                combat, npc_dialogue, save
  render/     - Rendering: hud, holamap, captive_compositor, renderer, viewport,
                liberation_viewport_3d
  audio/      - Audio: music, midi_player, sfx, adlib_sfx, opl2_emu
  data/       - Data loading: data_vfs, sha256, gfx_loader, texture_atlas,
                pl5/anm/rnc/ctv decoders, amiga_ofs/hunk/planar,
                liberation_data/anim/vgm/x3g/img/fnt, i18n, iso9660
  custom/     - Custom features: replay, cross_save, debug_hud, automap, lighting,
                minimap, audio_reverb, upscale_xbrz
  platform/   - Platform-specific: macos_menu
include/      - All public headers
tests/        - 62 test source files (62 CTest targets)
docs/         - Documentation and wiki
po/           - Translation files (19 languages)
data/         - Bundled fonts
assets/       - Icons (SVG, ICO, ICNS)
tools/        - hash_find, hash_extract, and other utilities
```

## Start Menu Architecture

The start menu presents an 8-item navigation grid (2 columns x 4 rows):

| Column 0 | Column 1 |
| --- | --- |
| Captive (new game) | Liberation (new game) |
| Continue Captive | Continue Liberation |
| Settings | About |
| Controls | Quit |

- **Continue** items are only shown when saves exist for that game.
- **Data status indicators**: each game card shows a SHA-256 verification indicator (checkmark when all required data is present, cross when missing).
- **Data Scanner** (press D): scans the VFS for all required content hashes and reports results per game.
- **Settings panel**: 24 configurable options with scrolling.
- **About screen**: credits, version, technology.
- **Controls screen**: full keyboard reference.

## Key Architecture

### GameState

`GameState` struct (~1.1 MB). Must be allocated as static or heap storage, never on the stack (Windows enforces a 1 MB stack limit).

### DataVFS

Content-addressed asset loading via SHA-256. Supports directories, ZIP archives, nested ZIPs, ADF disk images, and ISO9660 tracks. Resources are identified by content hash, never by filename.

- `vfs_init()` / `vfs_free()` — lifecycle
- `vfs_find_sha256()` — returns caller-owned buffer matching a digest
- `vfs_file_exists()` — check if a file is accessible
- `vfs_read_file()` — read a named file (for non-hash-identified resources)

### StartMenu

8-item grid with settings, about, controls, and scanner overlays. Renders game card backgrounds, data status indicators, and navigation highlighting.

### Audio

- **OPL2 emulator** for AdLib SFX (YM3812 FM synthesis).
- **MIDI playback** with OPL2 instrument bank.
- **8SVX sample loader** and 8-channel mixer.

### State Ownership

- `DataVFS` owns archive indexing, not returned match buffers.
- `vfs_find_sha256()` returns caller-owned bytes.
- `MusicSystem` owns the current MIDI bytes until track replacement or stop.
- `LiberationData` owns the raw disc buffer and closes it as one unit.
- `GameState`, `CreatureList` and `PuzzleList` are saved together for Captive.

### Save Format

The Captive save header has magic, version, campaign identity, party state, objective counters, gold and dynamic-list counts. The loader regenerates the deterministic base, overlays saved cells, validates every record and assigns only after complete success. Corrupt input leaves the active session intact.

Never extend a save record without incrementing its version and adding tests for old-version rejection and new-version round trip.

## Data Rules

Never use original media filenames as identity. Introduce a resource with:

1. A SHA-256 digest in source
2. A content-hash lookup via VFS
3. Checked lengths/offsets before decode
4. A deterministic test or verifier
5. No copied game payload in the repository

For Amiga OFS media, use `amiga_ofs_inventory` during discovery. For Liberation CD32 resources, use `liberation_inventory`. Both emit only digest, byte count and container class.

## Visual Comparison Workflow

Do not use a generated renderer hash as proof of parity. Capture an original frame and an OpenCaptive frame at the same native resolution, then compare:

```bash
./build/visual_compare original.ppm opencaptive.ppm diff.ppm
```

Reports exact-pixel coverage and mean absolute RGB error. Keep captures outside the repository and record their SHA-256 digests in the test report.

## Naming Conventions

- Source files: `src/{subsystem}/{feature}.c`
- Headers: `include/{feature}.h`
- Tests: `tests/test_{feature}.c`

## Release Workflow

1. Bump version in `CMakeLists.txt` and `include/opencaptive.h`.
2. Commit and tag with `vX.Y.Z`.
3. Push with `--tags` — GitHub Actions builds all platforms and creates a release.
4. CI matrix: Ubuntu 24.04 (deb, rpm, AppImage, tar.gz), macOS 14 (DMG), Windows 2022 (Inno Setup installer).

## Change Checklist

1. Preserve unrelated dirty work in the checkout.
2. Add a focused test that fails before the change.
3. Build Debug and Release.
4. Run the complete CTest suite.
5. Test hash-verified startup when changing loading or runtime code.
6. Document implementation status honestly — do not label a placeholder as original-game parity.
