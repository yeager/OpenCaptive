# User guide

## Build

OpenCaptive requires CMake 3.20+, a C17 compiler, SDL3 and zlib.

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Use a separate `build-release` directory with
`-DCMAKE_BUILD_TYPE=Release` for a shipping-style test run. Tests explicitly
keep assertions enabled in Release builds.

## Launch and data checks

```sh
./build/opencaptive --data /path/to/your/media --verify-data all
./build/opencaptive --data /path/to/your/media --game captive
./build/opencaptive --data /path/to/your/media --game liberation
```

`--verify-data captive`, `--verify-data liberation` and `--verify-data all`
perform read-only SHA-256 verification. A successful check means that the
known content required by the current loader was found; it does not claim that
every original gameplay system is implemented.

Archives may be stored directly in the chosen directory. The loader scans
archive entries and loose files by their bytes, so repacking or renaming media
does not change its identity.

## Captive controls

| Input | Action |
| --- | --- |
| W/S or arrows | Walk forward/back |
| A/D or left/right | Turn |
| Q/E | Strafe |
| Space | Attack with selected droid |
| F | Open/interact with the facing object |
| 1–4 | Choose droid |
| I / T / M | Inventory / terminal / map overlay |
| `,` / `.` | Use up/down stairs |
| F5 / F9 | Save / load Captive state |
| F10 | Open runtime graphics and cheat options |
| Escape | Return to menu |

Closed doors block movement and line of sight. Face a normal door and use `F`.
Generators are mission objectives; destroying all generated generators advances
the campaign. A direct attack or enemy shot cannot cross a wall or closed door.

## Runtime options and cheats

Press `F10` during either game to pause the live simulation and open the
runtime popup. It can switch the enhanced viewport, scanlines, CRT curvature,
bilinear output filtering, brightness, music and SFX immediately. The same popup has explicit cheat
switches for god mode, infinite energy and completing the current objective.
`Escape` or `F10` closes it. Cheats are deliberately visible and opt-in; they
are not activated by command-line defaults.

## Saves

Captive saves contain the map-cell state, droids, money, generator progress,
creatures and puzzle state. Loading is transactional: malformed or truncated
saves are rejected without overwriting the running game.

The save format is intentionally versioned. Newer engine versions can reject
old experimental saves rather than interpreting them incorrectly. Liberation
saves are not exposed by the current save system.

## Useful diagnostics

For a noninteractive smoke start on a Unix-like system:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  timeout 5 ./build/opencaptive --data /path/to/your/media --game captive
```

The expected result is a timeout after the game loop has started, with log lines
for texture loading and verified game startup. It is not a gameplay-completion
test.
