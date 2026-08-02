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

### DOS MapGen disassembly (partial)

The DOS generator entry at 0x1C3C in the unpacked CAPPO.EXE operates on a
bitmask buffer (20 bytes per row, stride 0xA0 in viewport mode). Key stages:

1. **Random rectangle carving** (0x1C4B): Decrements iteration counter
   `[0x8D7F]`, generates random position (AND 0x7F, max 0x6F) and size
   (AND 0x7, INC → 1-8), writes wall data via bitmask OR/AND.
2. **Boundary cleanup** (0x1D1C): Scans 3×3 neighborhoods around each cell
   comparing against threshold 0x18. Fills border cells with 0x20 (floor).
3. **Cell type assignment** (0x1E50): Converts bitmask to cell records (8 bytes
   each). Values: 0x00=wall, 0x20=floor, 0x44=door. Checks grid boundaries
   at CX=0x40 (width 64) and BX=0x20 (height 32).

The PRNG at 0x8E78 is the DOS variant: `state = state * 0x5E5 + 0x29` (no
rotation or XOR), stored at `[0x12DE]`.

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

## Viewport renderer

The viewport renderer (`src/render/viewport.c`) draws the first-person dungeon
view using the 19-cell trapezoid from `captive_view_window_build()` and
hash-verified PL5 panel sheets from the texture atlas:

- **5 depth ranges** with perspective-correct cell sizing
- **Back-to-front compositing** matching the documented draw order
- **Wall faces**: front, left and right walls sampled from PL5 source sheets
- **Floor/ceiling strips**: sampled from PL5 sheets per range
- **Doors**: drawn from the dedicated door PL5 sheet
- **Ornaments**: wall decorations from the icon PL5 sheet
- **Special cells**: visual markers for stairs, generators and shops

The renderer uses real PL5 pixel data exclusively — no synthetic textures. Each
wall/floor/ceiling fragment is sampled from the hash-verified source sheets at
positions determined by the cell's texture indices. Transparency is handled by
skipping pixels where the source ARGB has zero RGB.

This is the first stage of viewport parity. The original DOS renderer uses a
descriptor table with exact source rectangles, destinations and blitter flags.
Until that table is fully recovered, the current renderer approximates the
projection geometry from documented view dimensions rather than using the
original per-descriptor coordinates.

## Sound system

### MIDI music

The DOS release includes 63 standard MIDI files (format 1) organized into 14
categories matching the music_categories table from the executable:

| Category | Variants | Use |
|----------|----------|-----|
| MAIN2 | 1 | Title screen |
| GENBASE | 1 | Generator base exploration |
| BATT | 11 | Combat encounters |
| SHOPKEEP | 1 | Shop interaction |
| HOLOMAP | 1 | Holamap screen |
| ESCAPED | 1 | Escape sequence |
| FINAL2 | 1 | Final mission |
| TRAPPED | 1 | Trapped state |
| FCBASE | 11 | FC base exploration |
| VCBASE | 11 | VC base exploration |
| LONGNT | 11 | Long night ambient |
| W | 11 | Walking/exploration |
| COMPROOM | 1 | Computer room |
| RUNNING | 1 | Running/chase |

Categories with multiple variants are selected randomly using the game PRNG.
All 63 files are identified by SHA-256 content hash. The MIDI player renders
through a software synthesizer at 22050 Hz.

### AdLib sound effects

Sound effects are generated via AdLib OPL2 FM synthesis, not stored as PCM
samples. The `CAP_A.BIN` file (5,426 bytes) is a loadable x86 driver containing:

1. **x86 driver code** (0x000–0x3BF): AdLib I/O port programming and SFX
   interpreter loop
2. **9 channel descriptors** (0x3C0–0x46F): OPL2 channel-to-register mapping
3. **Amplitude decay table** (0x470–0x4FF): 128 descending values for envelope
4. **OPL2 F-number table** (0x530–0x5CF): 128 frequency values for note pitch
5. **49 SFX sequences** (0x0A6A–0x138F): bytecode programs that write OPL2
   registers to produce sound effects, each terminated by `0xFF` + `pend`
6. **26 instrument patches** (0x1390–0x1532): 16 bytes each containing the
   11 OPL2 register values per voice (AM/VIB/EG/KSR/MULT, KSL/TL, AR/DR,
   SL/RR, waveform, feedback/connection)

The SFX sequences, instrument patches, and frequency table have been extracted
to `adlib_data.c` with a verification test suite. Full OPL2 emulation for
rendering these sequences to PCM is a future task.

The CTV files (`SB15.CTV`, `SB20.CTV`, `SBPRO.CTV`) are CT-VOICE driver files
for Sound Blaster DSP, not Creative Voice (VOC) audio samples. They contain
executable code for DMA-based PCM playback but no sound data.

## Item database

Item records are stored in the unpacked CAPPO.EXE at offset 0x1a090. The first
item (HEAD) has a unique 16-byte prefix containing body part armor values for
all six material grades. Subsequent records follow the format:

    00 type_code [grade_byte] NAME 0x20

Type codes recovered from the binary:

| Code | Meaning | Examples |
|------|---------|----------|
| 0x00 | Misc / no class | DEV-SCAPE, KNUCLE-DUSTER, MINE, DIE, BALL, MAP |
| 0x08 | Consumable | BATTERY |
| 0x10 | Ammo | CARTRIDGES, SHELLS, LASER PACK, SONIC PACK |
| 0x20 | Chip | DROID CHIP |
| 0x21 | Equipment | OPTIC, CAMERA, BATTLE-GLOVE, WAR-BLADE, PISTOL |
| 0x27 | Body part (standard) | HEAD, CHEST, LEG, FOOT, HAND, GOLD |
| 0x30 | Ranged weapon | COLT, MAGNUM, RIFLE, all automatics/energy/heavy |
| 0x60 | Explosive | EXPLOSIVES |
| 0x65 | Body part (variant) | ARM |

Body parts carry a grade byte (always 0x05 in the base table). POISON, ACID,
and FLAMBOS use `04 27` as a terminator instead of 0x20, marking them as
special ammo sub-types (grenade-like items that deal body-part damage).

Ammo items (type 0x10) include caliber price entries: CARTRIDGES with prices
20/45/50 (using `%` and `&` delimiters for two price tiers), A51 MISSILES,
SHELLS, LASER PACK, SONIC PACK, and the three grenade types.

The weapon variant section at 0x1a220 contains 23 upgrade tier records in the
format `00 type_code 04 2c PRICE_STRING 2d` where the price string is ASCII
decimal. Tier 0 (0x30): 1.9. Tiers 1-5 (0x21/melee): 2, 3C, 5, 7, 14A.
Tiers 6-19 (0x30/ranged): 14B, 27, 23, 33.1, 56, 78, 99, 111, 141, 165.8,
180, 200, 211, 231. Named variant prefixes: A12, L22, X42.

Item stats (damage, range, ammo capacity, price, weight) are **not** stored as
lookup tables. The original game computes them procedurally from the type code
and material grade. The stat computation formulas remain to be recovered from
the code section of the executable.

## PRNG

Captive uses multiple PRNG variants, all based on the same linear congruential
core (`state = state * 0x5E5 + 0x29`) but with different post-processing:

| Location | Rotation | XOR | Usage |
|----------|----------|-----|-------|
| 0x8A8E | ror 3 | xor ah,0x08 | Main game PRNG |
| 0x9815 | ror 2 | none | Combat randomness |
| 0x9FBA | ror 3 | and 0x0F (mask) | Name/map generation |
| 0xDAB6 | none | xchg al,ah | Map seed chaining (3 iterations) |

The main PRNG state is stored at `[0x9308]` (combat) and `[0x92F8]` (general).
All operate on 16-bit values.

## Combat system

Combat is processed at 0x8D66 in the unpacked CAPPO.EXE. The system iterates
over 4 droid slots (struct size 0x10E bytes, base 0x8DC7) from the droid array
at 0x5E38:

1. **Hit check** (0x97D9): Looks up attacker's target list in a creature table
   indexed by CH. Walks the list comparing each entry to AL (target ID).
   Increments hit counter (DL) on match.

2. **Damage calculation** (0x9BF4): `damage = lo_byte([di+6]) * hi_byte([di+6])`.
   The two bytes at offset 6 in the combat struct encode base damage and
   multiplier.

3. **Damage scaling** (0x9BFC): Shifts damage left up to 3 times (×2, ×4, ×8),
   checking for signed overflow at each step. Returns 0xFFFD (-3) as an
   overflow sentinel.

4. **Damage application** (0x8DAE): `[di+6]` damage value added to accumulator
   at `[0x8D81]` when creature type byte equals 0x72.

### Weapon damage tables

Weapon damage encoding recovered from file offset 0x1A006.

**Melee weapons** (18 entries, 2 bytes each: lo, hi):

| # | lo | hi | Damage (lo×hi) | Notes |
|---|----|----|----------------|-------|
| 0-3 | 0x04 | 0x21-0x3C | 132-240 | Starter weapon, 4 grades |
| 4-7 | 0x4C | 0x21-0x3C | 2508-4560 | Heavy melee, 4 grades |
| 8-12 | varies | 0x21-0x45 | 693-1311 | Medium melee, 5 grades |
| 13-17 | varies | 0x21-0x45 | 2970-6141 | Elite melee, 5 grades |

Grade progression uses hi bytes: 0x21(33), 0x2A(42), 0x33(51), 0x3C(60), 0x45(69)
— increments of 9.

**Ranged weapons** (20 entries, 4 bytes each: base, modifier, flag, 0x00):

5 tiers × 4 levels. Base values: 72, 88, 104, 120 (increment 16).
Modifier values: 45, 35, 25, 15, 5 (decrement 10 per tier).
Flag: 1 for upgradeable, 0 for max-tier or base tier.

The combat PRNG at 0x9812 uses the weaker ror-2 variant without XOR, making
combat sequences more predictable than general game randomness.

## Creature stat tables

Creature stats recovered from CAPPO.EXE DS:0xA1BF (file offset 0x189AF).
Each creature type has a 4-byte entry: `min_hp` (word), `max_hp` (word).
25 creature types total, grouped into 8 categories (3 types each, plus 1 extra).

**HP formula** (0x9B12):

```
base = min + ((max - min) * difficulty) / 8
hp = ((base * 2 * creature_modifier) >> 8) + 6
```

Where `difficulty` = dungeon level capped at 0-7, `creature_modifier` = byte
from creature spawn data at [di+9].

| Type | HP min | HP max | Cat | Speed | Sprite |
|------|--------|--------|-----|-------|--------|
| 1 | 10 | 150 | 0 | 0 | 0x67 |
| 2 | 200 | 400 | 0 | 5 | 0x67 |
| 3 | 400 | 600 | 0 | 7 | 0x67 |
| 4 | 600 | 800 | 1 | 10 | 0x67 |
| 5 | 800 | 1000 | 1 | 16 | 0x67 |
| 6 | 1000 | 1500 | 1 | 20 | 0x67 |
| 7 | 1000 | 1700 | 2 | 30 | 0x6A |
| 8 | 1700 | 2400 | 2 | 20 | 0x6A |
| 9 | 2400 | 3000 | 2 | 22 | 0x6A |
| 10 | 3000 | 3700 | 3 | 24 | 0x6A |
| 11 | 3700 | 4400 | 3 | 24 | 0x6A |
| 12 | 4000 | 4900 | 3 | 40 | 0x6A |
| 13 | 3000 | 3700 | 4 | 32 | 0x6A |
| 14 | 3700 | 4400 | 4 | 36 | 0x6A |
| 15 | 4400 | 5700 | 4 | 46 | 0x6A |
| 16 | 9000 | 10000 | 5 | 56 | 0x6B |
| 17 | 10000 | 12000 | 5 | 20 | 0x6B |
| 18 | 12000 | 15000 | 5 | 24 | 0x6B |
| 19 | 15000 | 17000 | 6 | 26 | 0x6C |
| 20 | 17000 | 19000 | 6 | 25 | 0x6B |
| 21 | 19000 | 22000 | 6 | 30 | 0x6C |
| 22 | 1000 | 3000 | 7 | 40 | 0x6D |
| 23 | 3000 | 5000 | 7 | 25 | 0x6D |
| 24 | 5000 | 7000 | 7 | 30 | 0x6E |
| 25 | 1000 | 1100 | 0 | 30 | 0x60 |

Category table at DS:0x9A42 (file 0x18232): groups creature types into 8
categories (0-7), 3 types per category. Speed values at DS:0xA1A4 (file
0x18994). Sprite assignments at DS:0xA16E (file 0x1895E): graphic_id maps
to ALIEN PL5 sheets, frame_index selects the animation variant.

DS segment base = 0x0E3F (file offset = 0xE7F0 + DS_offset).

## VGA palette

The 32-color VGA palette is set via INT handler at 0x9F2, loading from
DS=0x0E3F:0x7DE (file offset 0xEFCE). Each entry is 3 bytes of 6-bit VGA
values (0-63). The palette has been verified to match the existing
`pl5_default_palette` in the codebase. Notable entries:

- Colors 0-5: black to gray ramp (monochrome shading)
- Colors 6-7: green tones (vegetation, status)
- Colors 8-9: red tones (damage, alerts)
- Colors 10-15: yellow, pink, magenta, orange (effects, UI)
- Color 16: black (duplicate of 0, used as separate layer)
- Color 17: white
- Colors 19-23: brown ramp (wood, earth textures)
- Color 24: bright red
- Colors 27-29: blue ramp (sky, water, energy)
- Colors 30-31: green tones (different from 6-7)

## HUD panel layout

The following blit rectangles were recovered from the VGA copy routines in
CAPPO.EXE. Each is defined by a linear VGA offset (`mov di`), line count
(`mov bp`), and bytes-per-line (`mov cx` or `rep movsw` count):

| File offset | Screen (x,y) | Height | Description |
|-------------|--------------|--------|-------------|
| 0x485F | (32, 55) | 112 | 3D viewport (144×112, confirmed) |
| 0x4839 | (32, 170) | 28 | Message/text area below viewport |
| 0x0615 | (28, 3) | 32 | Top-left panel (droid status) |
| 0x0DCA | (220, 4) | 37 | Top-right panel |
| 0x3AD8 | (190, 15) | 30 | Right status panel |
| 0xD41D | (210, 157) | 56 | Bottom-right panel |
| 0xDC92 | (80, 120) | 49 | Center-bottom panel |
| 0xDD47 | (0, 119) | 99 | Left panel (inventory) |
| 0x1F48 | (293, 76) | 99 | Right edge panel |

The viewport blit at 0x485F copies 72 words (144 bytes) per line with a source
skip of 16 bytes and destination stride of 176 (320−144), confirming the
viewport fills exactly 144×112 pixels.

## Current runtime boundary

Captive accepts movement, rotation, interaction, inventory, terminal, save and
F10 runtime controls. These currently operate on OpenCaptive's provisional map
state and must not be mistaken for an original-state recovery. The runtime
displays verified intro/HUD data and renders the viewport using real PL5 panel
sheets with approximate projection geometry.

The F10 menu provides God Mode, Infinite Energy and Complete Objective for the
active local Captive state. They are runtime conveniences, not original-game
commands or evidence of gameplay parity.
