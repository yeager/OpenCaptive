# User Guide

## System requirements

- **OS**: Linux (x86_64), macOS 14+ (Apple Silicon), Windows 10+
- **Dependencies**: SDL3, zlib (bundled in release builds)
- **Game data**: original Captive and/or Liberation media files (not included)

## Installation

### From release packages

Download from [GitHub Releases](https://github.com/yeager/OpenCaptive/releases):

| Platform | Package |
| --- | --- |
| Linux (Debian/Ubuntu) | `opencaptive_X.Y.Z_amd64.deb` |
| Linux (Fedora/RHEL) | `opencaptive-X.Y.Z.x86_64.rpm` |
| Linux (universal) | `opencaptive-x86_64.AppImage` |
| macOS (Apple Silicon) | `OpenCaptive-macos-arm64.dmg` |
| Windows | `OpenCaptive-X.Y.Z-setup-x64.exe` |

### Building from source

Requires CMake 3.20+, a C17 compiler (clang or MSVC), SDL3, zlib and Ninja.

```sh
cmake -S . -B build -DCMAKE_C_COMPILER=cc -G Ninja
ninja -C build
```

Release build:

```sh
cmake -S . -B build -DCMAKE_C_COMPILER=cc -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

Run tests:

```sh
ctest --test-dir build -j4 --output-on-failure
```

## Game data setup

Place your original game files in a directory. OpenCaptive identifies files by SHA-256 content hash, so filenames and directory structure do not matter. ZIP archives are scanned transparently.

Supported source media:

| Game | Accepted formats |
| --- | --- |
| Captive | DOS files (PL5, ANM, MID), Amiga ADF disk images, Atari ST disk images, ZIP archives |
| Liberation | CD32 raw BIN/CUE image (MODE1/2352), Amiga ADF disk images (5 disks) |

## Launching

```sh
opencaptive --data /path/to/your/media
opencaptive --data /path/to/your/media --game captive
opencaptive --data /path/to/your/media --game liberation
```

### Data verification

```sh
opencaptive --data /path/to/media --verify-data all
opencaptive --data /path/to/media --verify-data captive
opencaptive --data /path/to/media --verify-data liberation
```

Performs read-only SHA-256 verification of all known resources.

## Command-line flags

### Game selection

| Flag | Description |
| --- | --- |
| `--data <path>` | Path to game data directory |
| `--game <name>` | Select game: `captive` or `liberation` |
| `--verify-data <scope>` | Verify data integrity: `captive`, `liberation`, or `all` |

### Display settings

| Flag | Description |
| --- | --- |
| `--fullscreen` | Start in fullscreen mode |
| `--scale <N>` | Window scale factor (1-8, default: 3) |
| `--scanlines` | Enable scanline effect |
| `--crt` | Enable CRT curvature effect |
| `--bilinear` | Enable bilinear texture filtering |
| `--integer-scaling` | Force integer scaling (default: on) |
| `--no-integer-scaling` | Allow non-integer scaling |
| `--vsync` | Enable vertical sync (default: on) |
| `--no-vsync` | Disable vertical sync |
| `--fps <N>` | FPS limit: 0 (unlimited), 30, 60, 120 (default: 60) |
| `--brightness <N>` | Brightness level 0-100 (default: 50) |
| `--contrast <N>` | Contrast level 0-100 (default: 50) |
| `--renderer <mode>` | Render mode: `original` or `enhanced` |
| `--platform <name>` | Platform variant: `dos`, `amiga`, or `atarist` |

### Information

| Flag | Description |
| --- | --- |
| `--help`, `-h` | Show all available options |
| `--version`, `-v` | Show version and author |

## Start menu

The start menu provides access to all settings without CLI flags:

1. **New Game** — start a new game session
2. **Continue** — resume from last save (when available)
3. **Settings** — 14-item scrollable settings panel:
   - Renderer (Original / Enhanced)
   - Scanlines, CRT Curvature, Bilinear Filtering
   - Integer Scaling, Scale Factor
   - Fullscreen, VSync, FPS Limit
   - Brightness, Contrast
   - Music Volume
   - Data Path
4. **Quit** — exit the application

Navigate with arrow keys, Enter to select, Escape to go back.

## Controls

| Input | Action |
| --- | --- |
| Arrow keys | Navigate menus |
| Enter | Select / confirm |
| Escape | Return to start menu |
| F10 | Runtime display options (when available) |

Movement, combat, interaction, inventory and save/load are under active development. The runtime currently displays verified original presentation data (intro sequences, HUD, viewport frames).

## Diagnostics

Headless smoke test (Unix):

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  timeout 5 ./build/opencaptive --data /path/to/media --game captive
```

The expected result is a timeout exit after the game loop starts, with log lines for texture loading and verified startup.
