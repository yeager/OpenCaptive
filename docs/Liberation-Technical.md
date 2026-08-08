# Liberation: Captive II technical notes

> Documentation baseline: v1.1.90. The verified runtime boundary and remaining parity work are kept explicit in this document.

## Current status

OpenCaptive currently verifies and opens the known CD32 data track, reads its
ISO9660 filesystem by content hash, and presents verified original ANIM frames
with the original Amiga/CD32 indexed color palette. CityGen (64×64 grid) and
BuildingGen (building placement, road graph, naming) have been disassembled
and implemented as independently tested system slices. The full Liberation
campaign, original state formats, and end-to-end gameplay remain unfinished.
The ArcD Huffman+LZSS decompressor from PlotGen has been recovered with
bit-exact parity, enabling decompression of all three text data files
(PGE.txt, DTE.txt, CTE.txt).

All CD32 asset formats have been decoded: x3g 3D vectors (33 files, IFF FORM
O3DG with EXVL vertices and PLST polygon records), VGM wall textures (71 files,
4 AmSp banks per file, 152 sprites each), Img sprites (361 sprites across 4
files with multi-frame LOD), FNT proportional fonts (2 variants, 114 glyphs
with drop-shadow), and gamemenu.spr (AmSp bank).

CD audio playback uses 10 Red Book tracks from the CD32 disc image via a
dedicated SDL3 stereo audio stream. Speech/voice samples are loaded via the
8SVX decoder. The city rendering pipeline uses the original Amiga palette
loaded from the FORM/ANIM container for indexed-to-ARGB color lookup in
walls, ground, and 3D building entrance objects.

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

The hash-identified main executable
`db61f7e39fd31ac19b82216ea963711728d25518454fae42fd89c5bab52f2215`
is a valid four-hunk Amiga LoadSeg stream. Structural inspection reports one
225,396-byte code hunk at byte offset 44, one data hunk, two BSS hunks and
1,356 `RELOC32` entries. Hunk tags may carry Amiga memory-attribute bits, so
the parser masks those tag bits before validating the container. This makes
the original presentation/game executable available for static analysis; it
does not execute it or claim that its gameplay logic is already reproduced.

`liberation_inventory` is the discovery entry point for every remaining CD32
resource. It recursively walks the verified ISO and emits only SHA-256 digest,
byte length and container class (`IFF/ILBM`, `IFF/ANIM`, `RNC1`,
`Amiga-HUNK`, AMOS sprite/icon bank or raw). It intentionally neither displays
nor accepts filenames:

```sh
./build/liberation_inventory /path/to/media
```

Any future graphics decoder must record the selected digest in code and tests
before it is wired into the renderer.

## Sprite-bank observation log

The hash-identified resource
`07cca53c7efaac9e2880d50524039b0f9cb2a403e0cfbfb0b5f6ce408594d2d1`
has the `AmSp` signature and a 42-entry declaration. Its initial eight entries
are conventional four-colour-bitplane images followed by a fifth, one-bit
transparency mask plane. The observed record size is therefore
`10 + words × height × (depth + 1) × 2`; this makes every boundary through the
eighth entry exact. `amos_sprite_dump` decodes this verified, unflagged prefix
to a PPM inspection image using a resource hash and an entry index.

The high bit of the width word is not compression. The original 68000 loader
doubles the low 15 bits and subtracts one when that bit is set, so it represents
an odd number of bytes per row. The same planar-plus-mask decoder handles both
even and odd row widths. Records are word aligned: an odd total payload is
followed by one padding byte before the next header. The full declared 42-entry
stream, including the final entry, is now structurally decodable. The decoder
remains an analysis tool until the runtime maps particular sprite hashes to
particular game entities.

The loader also tests the X-hotspot sign bit. A negative X hotspot adds one
transparency-mask plane after the colour planes; a non-negative hotspot leaves
the image unmasked. Both variants are now accepted by the decoder.

The separately hash-identified resource
`d6bb0dd9c578beb8e84ddf9f458f0be43ec158b2b261491d023e972d2812c2d2`
contains one unmasked 320×109 AMOS scene. It remains available to the analysis
tools only. The runtime does not place it into an inferred city or building,
because that would falsely assign original pixels to an unverified game state.

RNC method 2 in the verified CD32 data is the old Amiga backwards stream: it
has a 12-byte header, reads compressed bytes from the packed end and writes the
decoded bytes backwards. OpenCaptive verifies its output byte-for-byte against
a separately decoded local reference before using it for container analysis.

## Verified FORM/ANIM city raster

The presentation bundle is selected from the verified ISO by SHA-256
`1d3a335d254c0eae919a712dd73bd41b24ed897bf145ed118ccf2277baa7a35f`.
OpenCaptive scans its RNC2 payloads and accepts the city form only when its
*decompressed* SHA-256 is
`b94a450c12428af9a22b8bb8c31fca74cdc2b2bd3be3dc9c7a1eadd7e6576101`.
It therefore does not use a byte offset, ISO member name or archive filename
as a content identity. The first `PACK` record declares a 320×167,
six-bitplane image (`40 × 167 × 6 = 40,080` bytes) and is paired with the
container's 32-colour `PALL` palette.

`liberation_anim_decode_first_frame()` handles this verified record and
`liberation_anim_blit()` renders planar pixels without interpolation. The
native capture's 320×167 city region has been directly compared against an
independent decoding of the same source bytes: both RGB buffers have SHA-256
`b7c326d1cdd36bb3574b33add3d68cff9739e7a5e339d800f44af3c79f510bb1`.
This proves that particular original raster is rendered exactly. It does not
yet prove parity for animated overlays, the in-game HUD, city state, or plot
logic.

The PACK stream is not a sequence of independently decodable full frames.
The verified player first expands one bounded work area: in the city resource
that area is 40,094 bytes (14-byte descriptor plus 40,080 planar bytes). Its
SHA-256 is
`76daac4c89c209416d7a865d0d4e8ea4da116e4336dacfcffd63e1c48a4b6a70`.
`liberation_presentation_capture` can write this workspace from a requested
FORM hash, alongside its PPM, for reproducible decoder work. The following
bytes in the PACK chunk are not yet assigned a standalone record meaning.
`liberation_pack_decode_workspace()` exposes only the bounded area; the
stricter full-stream diagnostic intentionally reports the trailing format data
as undecoded. This keeps the exact first raster available without
misrepresenting unknown animation layers as image data.

The accompanying `SCPT` IFF chunk is now copied verbatim into
`LiberationAnimScript` after the same FORM boundary checks. Both the intro and
city presentation report `decoded/SCPT` through `--verify-data liberation`.
This makes the original scene bytecode available to the runtime without
pretending that its instruction semantics have already been recovered.

The verified CD32 player traverses a script by reading a big-endian 16-bit
record size at each record boundary, advancing by that full size, and stopping
at a zero size. OpenCaptive validates this first sequence and exposes its
records through `liberation_anim_script_record_at()`. All 21 decoded
presentation resources terminate cleanly under that rule. This proves the
container-level record boundaries only: the payload opcodes, timing units and
draw operations are still deliberately opaque.

One payload field is also directly evidenced by the player: byte 2 of a record
is multiplied by the current timing base, shifted right by four and added to
the next update countdown. It is exposed as `timing_multiplier`; the absolute
clock unit and all remaining payload fields are not yet assigned semantics.

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

`amiga_hunk_parse()` now validates the HUNK header, allocation table, code,
data, BSS, `RELOC32`, symbol and end records without loading or executing the
payload. It is deliberately a structural parser: relocations are counted and
bounds-checked, but no original instruction stream is interpreted as gameplay
logic.

Against the verified CityGen digest, the parser reports two hunks, one code
block of 10,824 bytes at byte offset 36, one BSS block and one `RELOC32`
entry. The structural inspection command is hash-driven:

```sh
./build/liberation_hunk_info /path/to/media \
  e54540c3bf8dfaf569380a135ac039f1438e9efb85cf6d5e3e487e25d4c7c13e
```

The verified PlotGen digest
`bc9c922801661eb66024d0bcf822c03e38ffea7f3576693e0512692ccf6d6705`
has the same two-hunk shape, with one 12,388-byte code block at offset 36,
one BSS block and three `RELOC32` entries. This confirms that CityGen and
PlotGen are separate relocatable executable payloads, not data tables that can
be substituted with the current procedural interior generator.

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

## Current runtime boundary

The former inferred `LibState` city and interior implementation has been
removed. It generated streets, buildings and objectives that were not derived
from the CD32 game state and therefore could not be visually or logically
verified. The runtime now has a narrow, explicit boundary: it displays the
hash-verified intro and city ANIM first frames, can advance from intro to city,
and does not invent movement, buildings, objectives or saves.

## CD32 executable analysis

The verified main executable (245,628 bytes) contains embedded game data. See [[Liberation Game Data]] for the full extraction:

- **32 German city name syllables** for procedural place names
- **20 street types** (Street, Avenue, Boulevard, Promenade, etc.)
- **35 first names and 32 last names** for NPC generation
- **8 NPC titles** (Governor, Pilot, Trader, etc.)
- **9 shop types, 12 bar types, 11 business types, 12 industrial types**
- **4 city visual variants** with matching x3g vectors, fonts and monsters
- **File references** for all x3g, VGM, Img, FNT and spr assets

The executable identifies itself as "Liberation : Ratt V2.02 : Wyvern V2.00c", where Ratt and Wyvern are internal engine/tool codenames.

## 3D rendering

Liberation uses true 3D polygon rendering, not raycasting or tile-based projection.

### VGM wall textures

71 VGM files (`Wall01.VGM`–`Wall71.VGM`), each containing 4 concatenated AmSp
banks with 152 sprites per file. Sprites are 4bpp (16 colors) with transparency
mask. Each bank has its own 64-byte palette.

### x3g vector format

IFF container: `FORM O3DG` with `OFFS` object table and `VCDO` sub-forms.

- `EXVL`: vertex list — uint16 count, then 16 bytes per vertex (int16 x, y, z,
  group, reserved[4])
- `PLST`: polygon list — 36-byte header per polygon with normal vector, render
  flags, color/texture index, UV rect, and variable vertex references

33 x3g files across cities, rooms, monsters, NPCs and objects. 4 city visual
variants (N=0-3) with matching geometry sets.

### Projection

Perspective projection with z-buffer. Flat-shaded polygon rasterizer with
textured support for wall surfaces. The renderer transforms EXVL vertices
through a standard 3D pipeline (model -> world -> view -> screen) and rasterizes
PLST polygons with depth sorting.

## Save format

### LSAV binary format

Liberation saves use the `LSAV` binary format:

| Offset | Size | Field |
|--------|------|-------|
| 0 | 4 | Magic: `LSAV` |
| 4 | 2 | Version (1) |
| 6 | 4 | City seed |
| 10 | 2 | Difficulty level |
| 12 | 2 | Current mission |
| 14 | 4 | Gold amount |
| 18 | 4 | Game tick counter |
| 22 | 2 | Player X position |
| 24 | 2 | Player Y position |
| 26 | 1 | Player facing direction |
| 27 | var | 4 droid state records |
| var | var | Mission completion bitmap |

The save captures the procedural city seed rather than the full grid state,
allowing the city to be regenerated deterministically on load.

## Reverse-engineering plan

Progress on the data-driven parity work:

1. ~~Decode executable container/relocation records without distributing code.~~ **Done** — Amiga HUNK parser validates headers, code, BSS, RELOC32.
2. ~~Identify CityGen inputs, PRNG, output layout and persistent city state.~~ **Done** — 64×64 grid, 8×8 meta-grid, 13 tile templates, 3 planes, difficulty-gated phases.
3. ~~Identify BuildingGen inputs, building types, road graph, naming.~~ **Done** — 9 building types, road connections, city/building name generation.
4. ~~Recover ArcD compression format from PlotGen.~~ **Done** — Huffman+LZSS with bit-exact parity against all three text files.
5. Identify PlotGen’s building/interior format and plot progression state. **In progress**
6. Decode text table opcodes and dialogue branching. **In progress**
7. ~~Decode x3g 3D vector format for city/room/monster geometry.~~ **Done** — IFF FORM O3DG, EXVL vertices (16 bytes each), PLST polygons (36-byte header + variable vertex refs). Verified across all 33 x3g files.
8. ~~Decode VGM wall texture format.~~ **Done** — 4 concatenated AmSp banks per file, 152 sprites/file, 71 wall sets. All parse correctly.
9. ~~Decode Img sprite format.~~ **Done** — ImgA container, 1–6 color bitplanes + mask, multi-frame LOD variants. 361 sprites across 4 files.
10. ~~Decode FNT font format.~~ **Done** — CHAR container, 114 proportional glyphs, 2-plane (foreground + shadow). 2 variants decoded.
11. ~~Identify gamemenu.spr format.~~ **Done** — Standard AmSp bank.
12. Build 3D viewport renderer (x3g geometry + VGM textures + Img frame). **Next**
13. Add small independently testable parsers, golden hashes and structural
    invariants before wiring them into the live city loop.

Until steps 5–6 and 12–13 are complete, do not describe the procedural city as a
faithful reproduction of Liberation’s original story, population or plot logic.
