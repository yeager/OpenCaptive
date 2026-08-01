# Developer Guide

## Code layout

| Directory | Responsibility |
| --- | --- |
| `src/main.c` | Entry point, CLI parsing, VFS init, game loop |
| `src/engine/` | Game state, map generation, combat, inventory, shop, puzzle, UI, Liberation |
| `src/data/` | Containers (VFS, ADF, ISO9660, RNC), decoders (PL5, ANM), loaders (GFX, texture atlas, MIDI) |
| `src/render/` | Software framebuffer, viewport, HUD, enhanced renderer |
| `src/audio/` | MIDI parser/synthesizer, 8SVX sample loader, 8-channel mixer, music, SFX |
| `include/` | All public headers |
| `tests/` | Decoder, VFS, hash, ISO, map, game-state, save and Liberation checks |
| `tools/` | Inspection tools (pl5_to_bmp, anm_extract, rnc_decode) — never redistributes game data |
| `docs/` | Technical documentation and wiki source |
| `assets/` | Application icons (SVG, ICO, ICNS) |

## Language and build

- **Pure C** (C17 standard). No C++ code.
- **Compiler**: `cc` (system clang on macOS). Do not use gcc.
- **Build system**: CMake 3.20+ with Ninja generator.
- **Dependencies**: SDL3, zlib.

```sh
cmake -S . -B build -DCMAKE_C_COMPILER=cc -G Ninja
ninja -C build
ctest --test-dir build -j4 --output-on-failure
```

Always test both Debug and Release configurations. Test targets compile with assertions enabled in Release builds.

## Architecture

### Virtual File System (VFS)

`DataVFS` (`src/data/data_vfs.c`) provides transparent access to game data from directories and ZIP archives. Resources are identified by SHA-256 content hash, never by filename.

- `vfs_init()` / `vfs_free()` — lifecycle
- `vfs_find_sha256()` — returns caller-owned buffer matching a digest
- `vfs_file_exists()` — check if a file is accessible
- `vfs_read_file()` — read a named file (for non-hash-identified resources)

### State ownership

- `DataVFS` owns archive indexing, not returned match buffers.
- `vfs_find_sha256()` returns caller-owned bytes.
- `MusicSystem` owns the current MIDI bytes until track replacement or stop.
- `LiberationData` owns the raw disc buffer and closes it as one unit.
- `GameState`, `CreatureList` and `PuzzleList` are saved together for Captive.

### Save format

The Captive save header has magic, version, campaign identity, party state, objective counters, gold and dynamic-list counts. The loader regenerates the deterministic base, overlays saved cells, validates every record and assigns only after complete success. Corrupt input leaves the active session intact.

Never extend a save record without incrementing its version and adding tests for old-version rejection and new-version round trip.

## Data rules

Never use original media filenames as identity. Introduce a resource with:

1. A SHA-256 digest in source
2. A content-hash lookup via VFS
3. Checked lengths/offsets before decode
4. A deterministic test or verifier
5. No copied game payload in the repository

For Amiga OFS media, use `amiga_ofs_inventory` during discovery. For Liberation CD32 resources, use `liberation_inventory`. Both emit only digest, byte count and container class.

## Visual comparison workflow

Do not use a generated renderer hash as proof of parity. Capture an original frame and an OpenCaptive frame at the same native resolution, then compare:

```sh
./build/visual_compare original.ppm opencaptive.ppm diff.ppm
```

Reports exact-pixel coverage and mean absolute RGB error. Keep captures outside the repository and record their SHA-256 digests in the test report.

## Change checklist

1. Preserve unrelated dirty work in the checkout
2. Add a focused test that fails before the change
3. Build Debug and Release
4. Run the complete CTest suite
5. Test hash-verified startup when changing loading or runtime code
6. Document implementation status honestly — do not label a placeholder as original-game parity

## Release process

1. Bump version in `CMakeLists.txt` and `include/opencaptive.h`
2. Commit and tag with `vX.Y.Z`
3. Push with `--tags` — GitHub Actions builds all platforms and creates a release
4. CI matrix: Ubuntu 24.04 (deb, rpm, AppImage), macOS 14, Windows 2022 (Inno Setup installer)
