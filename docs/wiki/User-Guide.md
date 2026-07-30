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

## Captive status and controls

The Captive runtime currently shows hash-verified original presentation data.
Movement, combat, interaction, save/load and mission input are deliberately
unavailable: the earlier generated state was not derived from original game
logic and is therefore not shipped as a substitute.

| Input | Action |
| --- | --- |
| F10 | Open runtime display options when the host makes F10 available |
| Escape | Return to the start menu |

## Liberation status and controls

Verified CD32 presentation data is decoded for Liberation's opening and city
frame. The old procedural city/interior controls are not presented as a
playable Liberation implementation: their generated logic is not the original
game's logic and no longer produces substitute graphics. `Escape` returns to
the start menu; F10 can open runtime display options when available.

## Runtime options and cheats

Press `F10` during either presentation to open the runtime display popup when
the operating system and host application pass the key through. It can adjust
display-only settings such as scaling and filtering. It does not pause or alter
a gameplay simulation: cheats, save/load commands and synthetic sound effects
remain unavailable until they can be recovered from original behaviour.

## Saves

Save and load are unavailable in the shipped presentations. Experimental state
and save code remains outside the runtime until it can be validated against
original game behaviour.

## Useful diagnostics

For a noninteractive smoke start on a Unix-like system:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  timeout 5 ./build/opencaptive --data /path/to/your/media --game captive
```

The expected result is a timeout after the game loop has started, with log lines
for texture loading and verified game startup. It is not a gameplay-completion
test.
