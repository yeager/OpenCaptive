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

## CityGen observation log

The city-generator payload selected by
`e54540c3bf8dfaf569380a135ac039f1438e9efb85cf6d5e3e487e25d4c7c13e`
is 10,896 bytes and is recognised as an AmigaOS `loadseg()` executable. Its
embedded release string identifies it as **CityGen 1.12**, built 1994-01-03.

It uses the Amiga HUNK container layout. Initial disassembly shows an exported
entry path that receives a caller-owned parameter block, clears a 12,288-byte
work area and records 64×64 dimensions in its output state before invoking its
generation routines. These are observations from the verified bytes, not a
claim that OpenCaptive already reproduces CityGen output.

## Text-resource observation log

The hash-identified city-text payload
`99f7bd75794a7b4f3e94eeef9c61b756da938d862bb83339b140c18d02eb79c5`
is 17,809 bytes. The hash-identified dialogue payload
`e154d250c1acdbed66835bb356a699efdb6f9f8b5e6d586ca07080414610a94c`
is 14,136 bytes. Both contain readable English prose interleaved with compact
control bytes and expression-like tokens, so neither is a flat NUL-terminated
string table.

The city-text payload has a readable location-description section whose
records begin with a numeric selector followed by prose and branch comments.
The surrounding control stream contains conditional expressions and variable
references. The dialogue payload similarly contains response alternatives,
quoted text and scripted branches. This is enough to establish that text is
data-driven, but not enough to assign opcode semantics or wire original plot
decisions into the runtime. A future parser must retain byte offsets and
control boundaries, rather than stripping printable strings and treating them
as independent records.

`liberation_extract` is the supported inspection entry point:

```sh
./build/liberation_extract /path/to/media \
  e54540c3bf8dfaf569380a135ac039f1438e9efb85cf6d5e3e487e25d4c7c13e \
  /tmp/citygen.bin
```

The command first verifies the enclosing CD32 track, then performs an ISO
lookup by the resource digest. Its output must stay outside version control.

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
