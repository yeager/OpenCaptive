# Liberation: Captive II technical notes

## Current boundary

OpenCaptive currently verifies and opens the known CD32 data track, reads its
ISO9660 filesystem by content hash, and provides a separate city/interior
runtime loop. It does **not** yet reproduce the original CityGen/PlotGen game
logic. The verified payloads are preserved as a reverse-engineering boundary,
not silently substituted for original behaviour.

## CD32 data track

The supported track is a raw MODE1/2352 image containing ISO9660 with CDTV
extensions. ISO sectors are 2352 bytes in raw storage; user data starts after
the MODE1 framing. The ISO reader validates directory-record extents and file
sizes before returning a buffer.

The root-track and all current runtime-analysis resources are selected by their
SHA-256 bytes. See [Data identity and verification](Data-Identity-and-Verification)
for the manifest.

## Resource roles

The verified resource set includes an executable payload, city generator,
plot generator, plot text, city text and dialogue text. At present:

1. `liberation_data_open()` opens the hash-identified raw track.
2. Each required resource is looked up by a hash scan of the ISO directory.
3. Verification fails closed if any required resource is absent or mismatched.
4. `liberation_data_read()` exposes byte buffers to future parsers without
   filename coupling.

No original payload is bundled or emitted by tooling.

## Current city runtime

The current `LibState` is deliberately separate from Captive's dungeon and
mission loop. It contains a 32×32 city grid, building footprints, up to four
interior floors, a city/building mode and player coordinates. Its deterministic
generator uses a seed, produces streets and building blocks, and supports
entering a building, walking interiors and using elevators.

This separation matters: Captive combat ticks and Captive generator-completion
rules must not advance or end a Liberation session. The main loop explicitly
branches before applying either Captive-only rule.

## Reverse-engineering plan

The next parity work is data-driven rather than visual:

1. Decode executable container/relocation records without distributing code.
2. Identify CityGen inputs, PRNG, output layout and persistent city state.
3. Identify PlotGen’s building/interior format and plot progression state.
4. Decode text table offsets, encoding and dialogue references.
5. Add small independently testable parsers, golden hashes and structural
   invariants before wiring them into the live city loop.

Until those steps are complete, do not describe the procedural city as a
faithful reproduction of Liberation’s original story, population or plot logic.
