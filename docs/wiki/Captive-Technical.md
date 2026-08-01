# Captive technical notes

## Runtime model

Captive's recovered presentation currently consists of the original 320×200
HUD shell and the documented visibility rules. Legacy `DungeonLevel` and
gameplay structures remain in the source tree as reverse-engineering notes,
but are not driven by the runtime: they were generated substitutes, not decoded
original map or save state.

## PL5 graphics

A PL5 image is 40,000 bytes: 320×200 pixels at five bits per pixel. Every five
input bytes encode eight output pixels using a bespoke bit layout, not a normal
linear bitstream. `pl5_decode()` expands it to indexed pixels; palette values
are converted to ARGB for renderer textures.

The decoder has a dedicated regression test. Bounds checks reject a truncated
payload instead of reading beyond it.

## DOS renderer observation log

The verified DOS executable has SHA-256
`71bcf404103f1ac2920800a8bc166939bb49a1204cf51bebce8aca7dd5faafde` before
LZEXE unpacking. Static analysis of its unpacked 16-bit code shows a 320×200
VGA presentation path and two distinct PL5 blitters. One copies decoded pixels
unconditionally; the other preserves destination pixels for transparent source
values. Both write four VGA planes when planar output is active. The executable
also contains the original game-screen, roof, wall and door asset references.

These observations validate the resource-to-framebuffer path and are useful
for checking the PL5 decoder. They do **not** yet identify the first-person
projection tables, draw ordering, wall/door state encoding or creature
placement. OpenCaptive therefore keeps the default dungeon viewport unpainted
until those parts can be reproduced from original behaviour rather than an
invented perspective approximation.

### Recovered DOS dispatch boundary

The LZEXE-expanded program has SHA-256
`fa7d5ca76d26f614476ed41f27cf737084942e9216b20b4605734df9ede9aee4`.
Offsets in this subsection are relative to that expanded load module, not a
DOS segment address. The view path dispatches a sampled cell at `0x1fd1`.
Its handlers select a graphic ID, then enter the range-aware helpers at
`0x1ee4` and `0x1ef3`; those apply the original cell-depth adjustment before
calling the common draw entry at `0x2bd7`.

`0x2bd7` computes `0x00c0 + graphic_id × 8` in the original runtime's
descriptor table. The descriptor supplies a source pointer, destination
position, dimensions and blitter flags. The common path selects one of the
planar copy routines at `0x37dd`, `0x393f`, `0x3c5f`, `0x3c62` or `0x3c65`.
This is direct evidence that the projection is a table-driven sequence of
masked panel copies, rather than texture mapping. It also gives concrete
acceptance criteria for the native port: recover descriptor records and their
flags exactly, preserve the original range adjustment and call order, then
compare against DOS-VGA captures. The offsets alone do not license a guessed
table or a synthetic scene.

### Original runtime descriptor fixture

The original one-megabyte DOS memory fixture has SHA-256
`9003c4a8818cb97f8299ac90cfe51e90e535ab9a725545526fe75f14ddb8dd7e`.
It captures a running DOS renderer, not an archive extraction. In that state
the executable code is loaded at segment `0x0824`; MZ relocation changes the
unexpanded table selector in the code to segment `0x2942`. The descriptor
array begins at `0x2942:0x00c0` and has eight bytes per graphic ID. The common
draw entry reads, in order, a little-endian source offset, a little-endian
destination offset, width, height, flags and source-bank index.

For example, graphic IDs `0x004`–`0x009` occupy source offsets near `0x6660`,
have heights of 49 bytes and route through bank zero. The original code then
resolves that bank through its relocated segment selector table before choosing
the planar copy routine. This fixture makes the record layout, runtime
relocation and source-bank indirection reproducible. It is an analysis oracle
only: no dump bytes are embedded in OpenCaptive or used as a game-data
substitute.

The captured selector table resolves the first seven source-bank slots to
these DOS PL5 content hashes:

| Bank | Hash |
| ---: | --- |
| 0 | `7edb8ee856a91e835ea86dda00af49fda3dae730d694bd7234b7fa96d711e296` |
| 1 | `dec7143f063c98459ab2f267ed135204cdee1b521eda9810b219e8c10e05c7e8` |
| 2 | `70e0b9bfbaa5dfd12643b50cbe10d0b664de2fb1106d8ff0f2fde1ce6f443bbe` |
| 3 | `ce00ba2bc78f160b934486fe101a90264163356e02e7acbea2a41cf5d125b017` |
| 4 | `fed16e510697e17123d474c08687de548076b26a55f08f1d00fd17e3fcdf9410` |
| 5 | `21db7daf64cff3b0cae19c3e7eb2057762df9110055e7253175024ecb146fb6b` |
| 6 | `63ffa6901b59d463b050088065503d386ca2f3813ed91d8e0833320f9df2fe11` |

This relation was proven by comparing each relocated source segment to the
corresponding full 40,000-byte hash-identified payload. It is a fixture for
this verified renderer state, not a substitute for the original loader's
asset-set selection.

### Destination-buffer correction

The descriptor's destination word is **not** an offset into a 200-byte-wide
PL5 source sheet. In the native `0x3bc1` copy path, one five-byte source group
is expanded to eight indexed output bytes and the destination row advance is
`0x140` (320 bytes). The alternative path at `0x3d23` applies the original
transparent-write rule. The low flag bit selects the mirrored variant.

Consequently, a descriptor cannot be reproduced by decoding a source crop and
placing it at `destination / 200`. That tempting shortcut yields plausible
wall fragments but at incorrect positions and is not used by OpenCaptive.
The remaining conversion from the intermediate buffer to the displayed VGA
frame, plus the per-cell descriptor order, must be recovered before native
view rendering can claim visual parity.

The renderer samples a 19-cell trapezoid rather than a full 5×5 view: five
cells at ranges four and three, three cells at ranges two and one, then the
left/current/right cells at range zero. It rotates that sample from the party
orientation into a fixed forward-facing order, removes cells hidden behind
walls, and draws back to front. `captive_view_window_build()` now reproduces
the verified 19-cell sampling order and the ordered original wall-occlusion
pass while retaining the earlier 5×5 work area for analysis. A cleared cell
does not take part in later wall tests, matching the original copied-map
behaviour. Panel projection and resource selection are still separate recovery
work, not inferred rendering.

The sampling footprint, ordered cleanup conditions and back-to-front boundary
are documented independently in [The Ultimate Captive Guide: View Rendering](https://captive.atari.org/Technical/ViewRendering/ViewRendering.php).
The implementation intentionally limits itself to those published rules until
the DOS panel tables are also recovered from verified executable/media bytes.

### Verified panel sheets

The five relevant DOS PL5 resources are selected only by SHA-256:

```text
47ad15b4a593c37880d0306b6a0f51b7a9f20615cf6a188f23716d5b48315524
43833e4a8df622f84d53698a76c6d18f910c1cca79c6b89cbfacc563f695356c
8b7301fc6c302fd673a81d23e7a99d715aa02d5b404c1e1edea19ceccccc9681
519d3ef4494f0e868479a90c8a47249b840598e382c7ba3272f417ce3daf5936
7edb8ee856a91e835ea86dda00af49fda3dae730d694bd7234b7fa96d711e296
```

The first digest decodes to the published `fed7-A` interior reference image.
This establishes that the source data is authentic, but also rules out the old
64×64 tile interpretation: each 320×200 sheet contains irregular, overlapping
preprojected panels. A parity renderer must recover the original panel source
rectangles, destinations, transparency convention and per-cell state table;
sampling fixed-size tiles cannot reproduce the reference viewport.

## ANM animation

ANM starts with a 768-byte VGA palette of 6-bit RGB triples. A little-endian
word at offset 768 marks the command end. Frame records are read backwards from
end of file; each ends in a little-endian total-size word. Frame deltas use:

```text
non-zero byte  => XOR that byte into the current output position
zero byte      => next byte is a skip count
zero + zero    => end of frame
```

The target is a 64,000-byte 320×200 chunky frame buffer. Reconstructing each
frame from XOR deltas preserves the original incremental animation behaviour.

## RNC Method 1

Atari ST and Amiga resources may begin with `RNC\x01`. The header contains
big-endian unpacked and packed lengths plus CRC16 fields. Method 1 uses three
Huffman tables per sub-block. The decoder validates lengths before allocating
or copying and is kept separate from container readers.

## Media containers

- **Atari ST:** FAT12 disk image; the BIOS parameter block begins at offset 11.
- **Amiga:** 80 tracks × 2 sides × 11 sectors × 512 bytes (880 KiB), with OFS
  or FFS filesystem structures.
- **DOS-style asset sets:** may be extracted or ZIP-packed; the VFS identifies
  required content by SHA-256.

## Architect map generation

The original map model is a single 2,048-byte, 64×32 allocation divided into
sixteen 16×8 physical sections. Logical floors are assigned across those
sections; changing level changes the logical floor offset, not the allocation
shape. The modern engine exposes those regions as `levels[]`, which is an API
adaptation rather than proof of byte-for-byte MapGen parity.

The documented mission/base seed is:

```text
seed = ((mission - 1) × 11) + base
```

The first base is seed zero. Architect uses the sparse physical sections 2, 6
and 10 (one-based numbering) and starts the player at `(30,0)`. OpenCaptive
implements and tests that special case. For maps 1–4, the documented usable
section sets progressively expand: row one plus 6–7; rows one/two plus 10–11;
the first three rows; then the first three rows plus 14–15.

The original generator has 30 ordered stages. The documented order includes
floor layout, offsets, root position, unconditional walls, elevators,
no-touch zones, digging/rooms, root directions, dead-end smoothing, fire,
generators, doors, puzzles, traps, encounters, decorations and exterior
generation. Current `map_generate_base()` preserves the allocation dimensions,
early masks, root special case and deterministic API, but it is **not yet a
byte-identical implementation** of those 30 stages. In particular, the
current PRNG arithmetic and feature placement must not be treated as recovered
original code until they are validated against original MapGen output.

The technical reference used for this boundary is the documented
[MapGen introduction](https://captive.atari.org/Technical/MapGen/Introduction.php)
and its linked stage pages. New MapGen work must add an original-output fixture
or an independently reproducible reference before claiming parity.

## DOS executable analysis

The verified unpacked executable (144,556 bytes) contains the complete game data tables. See [[Captive Game Data]] for the full extraction:

- **10 droid material grades** (SHIT through TITANIUX)
- **10 combat skills** (Brawling through Energy Weapon)
- **~40 items** with hex classification bytes
- **48 name-generation syllables** (8 consonants x 6 vowels)
- **14 music categories x 11 variations** (154 track slots)
- **4 sound drivers** (AdLib, Roland, PC Speaker, Sound Blaster)
- **16 ANM animation files** and **multiple PL5 graphics sheets**

The executable structure, dispatch tables and descriptor format are documented above. The disassembly data provides ground truth for reimplementing item tables, shop pricing, combat formulas, and name generation without guessing.

## Current runtime boundary

Captive accepts movement, rotation, interaction, inventory, terminal, save and
F10 runtime controls. These currently operate on OpenCaptive's provisional map
state and must not be mistaken for an original-state recovery. The runtime
displays verified intro/HUD data and keeps the viewport untouched until the
original panel compositor and state format are recovered.

The F10 menu provides God Mode, Infinite Energy and Complete Objective for the
active local Captive state. They are runtime conveniences, not original-game
commands or evidence of gameplay parity.
