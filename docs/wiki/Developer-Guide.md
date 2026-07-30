# Developer guide

## Code layout

| Area | Responsibility |
| --- | --- |
| `src/data` | Containers, compression, hashes and asset decoding |
| `src/engine` | Game state, map generation, combat, inventory and UI logic |
| `src/render` | Software framebuffer, viewport, HUD and enhanced renderer |
| `src/audio` | MIDI parsing/synthesis, music ownership, effects and mixer |
| `tests` | Decoder, VFS, hash, ISO, map, game-state, save and Liberation checks |
| `tools` | Narrow inspection tools; never a game-data redistribution mechanism |

## Build and test contract

```sh
cmake -S . -B build -G Ninja
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

Run the same suite with `-DCMAKE_BUILD_TYPE=Release`. Test targets compile with
assertions explicitly enabled, because test expressions use `assert()` for both
execution and verification.

## Native visual comparison

Do not use a generated renderer hash as proof of parity. Capture an original
frame and an OpenCaptive frame at the same native resolution, then compare them
directly:

```sh
./build/visual_compare original.ppm opencaptive.ppm diff.ppm
```

The command reports exact-pixel coverage and mean absolute RGB error; its
optional last path receives a red error heatmap. Keep captures outside the
repository and record their SHA-256 digests in the test report. Compare only
the same game, platform, scene and animation frame.

## State ownership

- `DataVFS` owns archive indexing, not returned match buffers.
- `vfs_find_sha256()` returns caller-owned bytes.
- `MusicSystem` owns the current MIDI bytes until track replacement or stop.
- `LiberationData` owns the raw disc buffer and closes it as one unit.
- `GameState`, `CreatureList` and `PuzzleList` are saved together for Captive
  so a loaded session cannot respawn defeated actors or reset solved panels.

## Save format

The Captive save header has magic, version, campaign identity, party state,
objective counters, gold and dynamic-list counts. The loader constructs a
temporary replacement game, regenerates the deterministic base, overlays saved
cells, validates every dynamic record and assigns it only after complete
success. Corrupt input therefore leaves the active session intact.

Do not extend a save record without incrementing its version and adding tests
for an old-version rejection and a new-version round trip.

## Data rules

Never use original media filenames as identity. Introduce a resource with:

1. a SHA-256 digest in source,
2. a content-hash lookup,
3. checked lengths/offsets before decode,
4. a deterministic test or verifier, and
5. no copied game payload in the repository.

For Amiga OFS media, use `amiga_ofs_inventory <data-dir> <adf-sha256>` during
discovery. It exposes byte count, SHA-256 and container signature only. Do not
turn an inventory result into a resource role merely because its length or
compressed form looks plausible; retain the digest and verify the role against
an original trace or independently decoded output.

## Change checklist

1. Preserve unrelated dirty work in the checkout.
2. Add a focused test that fails before the change.
3. Build Debug and Release.
4. Run the complete CTest suite.
5. Test a direct hash-verified startup when changing loading or runtime code.
6. Document implementation status honestly; do not label a placeholder as
   original-game parity.
