# User Guide

> Updated for v1.1.116. The launcher now caches unchanged data-scan results and asks which verified version to launch when several versions are present.

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

## Running

### Basic usage

```sh
opencaptive --data /path/to/your/media
opencaptive --data /path/to/your/media --game captive
opencaptive --data /path/to/your/media --game liberation
opencaptive --data /path/to/your/media --lang sv
```

### Default data paths

- **Linux / macOS**: `~/.opencaptive`
- **Windows**: `installdir\data` (relative to the installation directory)

### Command-line flags

#### Game selection

| Flag | Description |
| --- | --- |
| `--data <path>` | Path to game data directory or one ZIP archive |
| `--game <name>` | Select game: `captive` or `liberation` |
| `--lang <code>` | Language code (see Internationalization below) |
| `--verify-data <scope>` | Verify data integrity: `captive`, `liberation`, or `all` |

#### Display settings

| Flag | Description |
| --- | --- |
| `--fullscreen` | Start in fullscreen mode |
| `--scale <N>` | Window scale factor (1-5, default: 3) |
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
| `--hd-upscale` | Apply xBRZ upscaling to native game frames |
| `--upscale-factor <N>` | xBRZ factor: 2, 3 or 4 (implies `--hd-upscale`) |
| `--widescreen` | Expand the game presentation horizontally to 16:9 |
| `--hq-midi` | Enable the enhanced MIDI output filter |
| `--renderer <mode>` | Render mode: `original` or `enhanced` |
| `--platform <name>` | Platform variant: `dos`, `amiga`, or `atarist` |

#### Information

| Flag | Description |
| --- | --- |
| `--help`, `-h` | Show all available options |
| `--version`, `-v` | Show version and author |

## Start Menu

The start menu uses an 8-item card layout:

### Top row: game cards

- **Captive card** — click or press Enter to start a new Captive game
- **Liberation card** — click or press Enter to start a new Liberation game
- Each card displays a checkmark (green) if game data passes SHA-256 verification, or a cross (red) if data is missing or fails verification

### Continue row

Appears only when save files exist (`opencaptive.sav` or `liberation.sav`). Resumes from the most recent save.

### Lower rows

- **Settings** — opens the 16-item settings panel
- **About** — version, credits, and license information
- **Controls** — input reference
- **Quit** — exit the application

### Keyboard shortcuts in the start menu

- **D** — opens the Data Scanner (scans the data path, reports ZIP count, per-game SHA-256 verification status)
- **F1** — opens the Controls reference overlay

## Settings

The settings panel contains 24 configurable items (some shown only when relevant):

| Setting | Values |
| --- | --- |
| Renderer | Original / Enhanced |
| Scanlines | On / Off |
| CRT Curve | On / Off |
| Bilinear | On / Off |
| Integer Scale | On / Off |
| Scale | 1x through 5x |
| Fullscreen | On / Off |
| VSync | On / Off |
| FPS Limit | Unlimited / 30 / 60 / 120 |
| Brightness | 0-100% |
| Contrast | 0-100% |
| Music | On / Off |
| SFX | On / Off |
| Data Path | Editable text field |
| Language | 19 languages (see below) |
| Back | Return to start menu |

## Controls

### Movement

| Key | Action |
| --- | --- |
| W / Up | Move forward |
| S / Down | Move backward |
| A / Left | Turn left |
| D / Right | Turn right |

### Combat

| Key | Action |
| --- | --- |
| Space | Fire weapon |
| Enter | Use item / interact |

### User interface

| Key | Action |
| --- | --- |
| Tab | Cycle droids |
| I | Inventory |
| M | Minimap |
| Shift+M | City map (Liberation only) |

### System

| Key | Action |
| --- | --- |
| F5 | Save game |
| F9 | Load game |
| F6 | Cycle save slots |
| F7 | Debug HUD |
| F8 | Minimap toggle |
| F10 | Runtime options popup: display effects, audio toggles, overlays and cheats |

### Menu and help

| Key | Action |
| --- | --- |
| H | Help screen |
| ESC | Pause menu |
| F1 | Controls reference |

The F10 popup applies display changes immediately while playing. God Mode and
Infinite Energy affect both Captive and Liberation; Escape or F10 closes the
popup.

`--widescreen` changes only the presentation canvas; it does not invent extra
world geometry outside the verified native frame. Set `widescreen_width` in a
features configuration file to request a specific output width.

### Speed

| Key | Action |
| --- | --- |
| Numpad + | Increase game speed (if enabled) |
| Numpad - | Decrease game speed (if enabled) |

## Internationalization

OpenCaptive supports 19 languages, selectable via `--lang <code>` or in the Settings menu:

| Code | Language |
| --- | --- |
| en | English |
| sv | Swedish |
| cs | Czech |
| da | Danish |
| de | German |
| es | Spanish |
| fi | Finnish |
| fr | French |
| hu | Hungarian |
| it | Italian |
| ja | Japanese |
| ko | Korean |
| nl | Dutch |
| no | Norwegian |
| pl | Polish |
| pt | Portuguese |
| ro | Romanian |
| ru | Russian |
| zh | Chinese |

## Game Data

### Content identity

OpenCaptive identifies all game files by SHA-256 content hash, never by filename. This means files can have any name and be placed in any directory structure.

### Supplying data

Place ZIP archives containing your original game media in the data directory, or pass one archive directly with `--data`. OpenCaptive transparently scans:

- ZIP archives (including nested ZIPs)
- ADF disk images
- ISO tracks (BIN/CUE)

### Verification requirements

| Game | Verified files needed |
| --- | --- |
| Captive | 25 |
| Liberation | 7 |

### Data Scanner

Press **D** in the start menu to open the Data Scanner. It scans the configured data path and reports:

- Number of ZIP archives found
- Per-game SHA-256 verification status for each required file

### CLI verification

```sh
opencaptive --data /path/to/media --verify-data all
opencaptive --data /path/to/media --verify-data captive
opencaptive --data /path/to/media --verify-data liberation
```

## Diagnostics

Headless smoke test (Unix):

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  timeout 5 ./build/opencaptive --data /path/to/media --game captive
```

The expected result is a timeout exit after the game loop starts, with log lines for texture loading and verified startup.
