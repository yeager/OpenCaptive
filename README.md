# OpenCaptive

A modern, open-source reimplementation of **Captive** (1990) by Antony Crowther.

Captive is a sci-fi dungeon crawler where you control four droids through procedurally generated space bases. Often considered one of the best Dungeon Master clones ever made.

## Features

- Faithful reimplementation of the original game engine
- Supports DOS, Atari ST, and Amiga game data
- Original rendering mode (pixel-perfect) and enhanced mode
- SDL3-based cross-platform runtime (macOS, Linux, Windows)

## Building

Requires CMake 3.20+, a C17 compiler, and SDL3.

```bash
cmake -S . -B build -DCMAKE_C_COMPILER=cc -G Ninja
ninja -C build
```

## Running

You need original Captive game data files (not included).

```bash
./build/opencaptive --data /path/to/captive/data --scale 3
```

## Status

Early development — file format decoders and renderer scaffolding in place.

## License

TBD
