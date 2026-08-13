# File Formats

> Documentation baseline: v1.1.117. Format readers are covered by the release test suite; original game data is never distributed.

This page documents all file formats encountered in Captive and Liberation media.

## PL5 — Captive DOS graphics

A PL5 image is exactly **40,000 bytes**: 320x200 pixels at 5 bits per pixel.

Every 5 input bytes encode 8 output pixels using a bespoke bit layout (not a linear bitstream). The palette is not stored in the PL5 file — it comes from the corresponding ANM file header or a hardcoded VGA palette.

Decoding algorithm (C pseudocode):

```c
for each group of 5 bytes (b0..b4):
    for bit 7 down to 0:
        pixel = ((b0 >> bit) & 1)
              | (((b1 >> bit) & 1) << 1)
              | (((b2 >> bit) & 1) << 2)
              | (((b3 >> bit) & 1) << 3)
              | (((b4 >> bit) & 1) << 4);
```

Output is 32-colour indexed. The renderer converts to ARGB via the loaded palette.

### OBJECTS.PL5

`OBJECTS.PL5` stores sprites as 16x16 pixel frames arranged in a 20-column grid.
Each sprite occupies one cell in the grid. The same PL5 5bpp encoding applies;
frames are extracted by grid coordinates rather than sequential offsets.

## ANM — Captive DOS animation

Structure:

| Offset | Size | Content |
| --- | --- | --- |
| 0 | 768 | VGA palette (256 entries x 3 bytes, 6-bit RGB) |
| 768 | 2 | Command end offset (little-endian) |
| 770.. | variable | Frame records (stored backwards from EOF) |

Each frame record ends with a little-endian 16-bit total-size word. Frame deltas use XOR encoding against a 64,000-byte (320x200) chunky frame buffer:

- Non-zero byte: XOR into current output position
- Zero byte followed by N: skip N pixels
- Zero byte followed by zero: end of frame

Captive DOS uses 16 ANM files for various animations (intro, game screens, cutscenes).

## RNC — Rob Northen compression

Atari ST and Amiga resources may begin with `RNC\x01` (Method 1) or `RNC\x02` (Method 2).

### RNC Method 1

- 18-byte header: magic (4), unpacked size BE (4), packed size BE (4), unpacked CRC16 (2), packed CRC16 (2), unknown (2)
- Three Huffman tables per sub-block
- Forward decompression

### RNC Method 2

Used in Liberation CD32 data. Old Amiga backwards stream:

- 12-byte header
- Reads compressed bytes from packed end, writes decoded bytes backwards
- OpenCaptive verifies output byte-for-byte against separately decoded reference

## ADF — Amiga disk image

Standard Amiga floppy disk format:

- 80 tracks x 2 sides x 11 sectors x 512 bytes = **880 KiB**
- OFS (Old File System) or FFS (Fast File System)
- Captive uses 1 ADF disk
- Liberation uses 5 ADF disks (CaptiveII_Disk1 through CaptiveII_Disk4 + boot)

The `adf_reader` module reads the OFS filesystem structure. The `amiga_ofs_inventory` tool inventories content by SHA-256.

## ISO9660 — Liberation CD32 disc image

Liberation CD32 comes as a raw BIN/CUE image:

- Sector size: **2352 bytes** (raw MODE1)
- User data: 2048 bytes per sector, starting at offset 16
- CDTV extensions present
- Audio tracks: 10 tracks by Mark Knight (Track 02-11)

The `iso9660_reader` module validates directory records, extents and file sizes before returning buffers.

## Atari ST disk image

FAT12 disk image. The BIOS parameter block begins at offset 11. Used for the Atari ST release of Captive.

## ArcD — Liberation Huffman+LZSS compression

Used by the PlotGen executable on Liberation Disk 3 to compress text data files (PGE.txt, DTE.txt, CTE.txt). The decompressor lives at offsets 0x302-0x520 in the PlotGen 68k binary.

### Header (12 bytes)

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 4 | Magic: `ArcD` (0x41726344) |
| 4 | 4 | Decompressed size (big-endian) |
| 8 | 4 | Compressed size (big-endian) |

### Bit buffer

- 32-bit register (d6), 8-bit available counter (d7)
- Bits consumed LSB-first from 16-bit big-endian words loaded from the source
- Initial state: load first 2 bytes into d6, d7 = 0
- Refill: when d7 goes negative (unsigned > 127), shift out remaining old bits, swap halves, load 2 new bytes into low word, swap back

### Block structure

Each block begins with a 16-bit block_count read from the bit stream. If block_count == 0, decompression is done. Otherwise:

1. Read 5-bit ot value. If ot == 31, the next block_count bytes are raw literals (not Huffman-coded).
2. Build three Huffman tables from the bit stream:
   - **lit_count** table (at a3+256): ot symbols
   - **offset** table (at a3+0): own 5-bit symbol count
   - **length** table (at a3+128): own 5-bit symbol count
3. Decode initial literal run from lit_count table
4. Repeat block_count - 1 times:
   a. Decode offset from offset table. Negate to get back-reference distance.
   b. If offset >= 512, copy 1 extra byte from back-reference.
   c. Decode match length from length table.
   d. Copy 1 mandatory byte + (match_length + 1) bytes from back-reference.
   e. Decode literal count from lit_count table. Copy (literal_count - 1) raw bytes.

### Huffman table format (128 bytes per table)

Each table is stored as a flat array:
- Bytes 0-63: up to 16 entries of (mask:16, match:16)
- Bytes 64-127: up to 16 entries of (shift:8, symbol:8, extra_mask:16)

Canonical Huffman codes with bit-reversed match values. The code counter starts at 0 for each bit length and doubles between lengths (like canonical Huffman). Match values are the bit-reversal of the code counter to the code length.

### Symbol decoding

The decoded symbol index determines the output value:
- Symbol 0 or 1: value = symbol (literal 0 or 1)
- Symbol k >= 2: read (k-1) extra bits from stream, set bit (k-1), giving values in range [2^(k-1), 2^k - 1]

### Table reuse

When a table's symbol count is 0, the previous block's table is reused (the 68k code does not zero or rebuild the table).

### Known compressed files

| File | Compressed | Decompressed | Blocks |
| --- | --- | --- | --- |
| PGE.txt | 7,085 | 16,304 | 2 |
| DTE.txt | 5,323 | 14,136 | 2 |
| CTE.txt | 8,230 | 17,809 | 2 |

## x3g — Liberation 3D vector graphics

Liberation uses true 3D polygon rendering (not raycasting). Vector data is stored in `.x3g` files:

| File pattern | Content |
| --- | --- |
| `{N}CityVectors.x3g` | City 3D geometry (4 variants, N=0-3) |
| `{N}RoomVectors.x3g` | Room/interior 3D geometry |
| `{N}BankVectors.x3g` | Bank interior vectors |
| `{N}BarVectors.x3g` | Bar interior vectors |
| `{N}ShopVectors.x3g` | Shop interior vectors |
| `{N}Droids.x3g` | Droid 3D models |
| `{N}CityMonsters.x3g` | City monster models |
| `PoliceVectors.x3g` | Police/guard models |
| `Monsters{2,3,4}.x3g` | Additional monster sets |
| `People.x3g` | NPC models |
| `Objects.x3g` | Item/object models |

### x3g binary format

IFF container: `FORM O3DG`

```
FORM O3DG
  OFFS          — object count (uint16) + offset table (uint32 per object)
  FORM VCDO     — one per object
    EXVL        — vertex list: uint16 count, then 16 bytes per vertex:
                    int16 x, y, z, group, reserved[4]
    PLST        — polygon list: variable-size records, terminated by 4 zero bytes
```

#### PLST polygon record format

Each record is self-sized (uint16 at offset 2). Fixed 36-byte header + variable vertex refs:

| Offset | Type | Field |
| --- | --- | --- |
| 0 | uint16 | Type (always 0 observed) |
| 2 | uint16 | Record size in bytes |
| 4–11 | int16×4 | w2–w5 (flags/parameters) |
| 12–15 | int16×2 | w6, w7 (shading parameters) |
| 16–19 | int16×2 | Normal X, Normal Y |
| 20–21 | uint16 | Render flags (0x1700 = filled, 0x1304/0x1101 = other modes) |
| 22–23 | uint16 | Color/texture index |
| 24–31 | uint16×4 | UV rect (left, top, right, bottom) |
| 32–33 | uint16 | w16 |
| 34–35 | uint16 | Vertex count (N) |
| 36+ | uint16×(N+1) | Vertex refs as EXVL byte offsets (÷16 = index), last repeats first (closing) |

Record size = 38 + 2×N. Common sizes: 40 (1 vertex), 44 (triangle), 46 (quad).

Verified against `Objects.x3g` (3 objects), `people.x3g` (4 objects), `0CityVectors.x3g` (33 objects).

## VGM — Liberation wall textures

Wall textures in Liberation use the `.VGM` format. 71 files (`Wall01.VGM`–`Wall71.VGM`), each exactly 167,766 bytes.

Each VGM file contains 4 concatenated **AmSp** (AMOS Sprite Bank) banks:

| Bank | Sprites | Purpose |
| --- | --- | --- |
| 0 | 42 | Primary wall faces |
| 1 | 45 | Alternate angles / details |
| 2 | 24 | Overlay elements |
| 3 | 41 | Additional variants |

Total: 152 sprites per file. All sprites are 4bpp (16 colors) with mask. Each bank has its own 64-byte palette at the end.

The AmSp format is the standard AMOS Professional Sprite Bank — the same decoder used for Captive sprite data works here.

## Img — Liberation graphics

Pre-rendered graphics in Liberation's Amiga image format. Magic `ImgA` + uint16 sprite count + uint32 offset table.

| File | Content | Sprites |
| --- | --- | --- |
| `MainSP.Img` | Main sprite sheet | 158 simple |
| `3dView.Img` | 3D viewport frame/border | 23 multi-frame (6 LOD frames each) |
| `Taxi.Img` | Taxi graphics | 4 simple |
| `backpack.img` | Inventory/backpack UI | 176 simple |

### Img sprite record format (simple, flags=0)

| Offset | Size | Field |
| --- | --- | --- |
| 0 | uint16 | Flags (0 = simple sprite) |
| 2 | uint16 | Width in pixels |
| 4 | uint16 | Height in pixels |
| 6 | uint8 | Format tag (always 0x06) |
| 7 | uint8 | Reserved (0) |
| 8 | uint16 | Data size (bytes from here to end of color planes) |
| 10 | uint16×6 | Plane offset table (from byte 8, zero = sequential/unused) |
| 22 | var | Color bitplane data (1–6 planes, planar Amiga layout) |
| 22+color | plane_size | Mask bitplane (1-bit transparency) |

`plane_size = ceil(width/16) × 2 × height`. Total sprite = 8 + data_size + plane_size.

Color depth varies per sprite: 1–6 bitplanes (2–64 colors). The number of color planes = `(data_size - 14) / plane_size`.

### Multi-frame sprites (flags > 0)

| Offset | Size | Field |
| --- | --- | --- |
| 0 | uint16 | Frame count |
| 2 | uint16×N | Frame offsets (from byte 0 of sprite record) |

Each frame is a sub-sprite identical to the simple format but without the 2-byte flags field (header starts at width). Used by 3dView.Img for distance-LOD viewport frames.

## AmSp — AMOS sprite bank

Used in Liberation CD32. `AmSp` signature followed by entry count.

Record structure: 10-byte header, then `words x height x (depth + 1) x 2` bytes. The extra plane is a 1-bit transparency mask. High bit of width word indicates odd bytes-per-row (the loader doubles low 15 bits and subtracts one).

Records are word-aligned: odd total payload is followed by one padding byte.

## FNT — Liberation fonts

Font data stored as `{N}Liberation.FNT` (2 variants: N=0 large, N=1 small). `CHAR` magic, 1836 bytes, 114 proportional-width glyphs starting at ASCII 32.

### FNT file header (12 bytes)

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 4 | Magic `CHAR` |
| 4 | uint16 | Glyph count (114) |
| 6 | uint16 | Version (1) |
| 8 | uint16 | Max glyph width (8) |
| 10 | uint16 | Plane count (2) |

### FNT glyph record (16 bytes)

| Offset | Size | Field |
| --- | --- | --- |
| 0 | uint8 | Glyph width (proportional spacing) |
| 1–7 | 7×uint8 | Plane 0 bitmap (7 rows, 8px wide, MSB=leftmost) |
| 8 | uint8 | Width duplicate (plane 1) |
| 9–15 | 7×uint8 | Plane 1 bitmap (drop shadow/outline) |

2-plane rendering: plane 0 = foreground stroke, plane 1 = shadow/outline. 4 possible color indices per pixel (0=transparent, 1=fg, 2=shadow, 3=both).

## spr — Liberation sprites

`GameMenu.spr` is a standard AmSp (AMOS Sprite Bank) — same format as VGM banks. See AmSp section.

## IFF ANIM — Amiga animation

Liberation Amiga floppy version uses IFF ANIM (opt5 interleave). Contains FORM/ANIM chunks with:

- `PACK` — compressed frame data (planar images)
- `PALL` — 32-colour palette
- `SCPT` — animation script bytecode

The verified player reads big-endian 16-bit record sizes from SCPT, advancing by full size, stopping at zero size.

## MIDI — Captive music

Captive DOS uses 63 standard MIDI files (format 1) organized into 14 categories with up to 11 variations each (154 total track slots). All files are identified by SHA-256. The original supports AdLib, Roland MT-32, PC speaker and Sound Blaster output. OpenCaptive plays through OPL2 FM synthesis using the 26 instrument patches from CAP_A.BIN.

## 8SVX — Amiga audio samples

IFF 8SVX format for audio samples. Contains VHDR (voice header), BODY (sample data). Used for Amiga sound effects. OpenCaptive's 8-channel mixer with channel stealing handles playback.

## CTV — CT-VOICE driver files

The `.CTV` files in Captive DOS are CT-VOICE driver files for Sound Blaster DSP,
**not** Creative Voice (VOC) audio samples. They contain executable code for
DMA-based PCM playback but no sound data.

| File | Target hardware |
| --- | --- |
| SB15.CTV | Sound Blaster 1.5 |
| SB20.CTV | Sound Blaster 2.0 |
| SBPRO.CTV | Sound Blaster Pro |

These drivers are loaded by the game to interface with the Sound Blaster DSP
for PCM playback. The actual sound effects are generated via AdLib OPL2 FM
synthesis from CAP_A.BIN sequences.

## OCSV — OpenCaptive save format

OpenCaptive's native save format for Captive game state:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 4 | Magic: `OCSV` |
| 4 | 2 | Version |
| 6 | 4 | Map seed |
| 10 | 2 | Mission number |
| 12 | 2 | Base number |
| 14 | 4 | Player position (x, y) |
| 18 | 1 | Facing direction |
| 19 | 1 | Current level |
| 20 | var | 4 droid state records |
| var | var | Inventory state |
| var | var | Map exploration bitmap |

## LSAV — Liberation save format

Liberation's binary save format:

| Offset | Size | Field |
| --- | --- | --- |
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

The save stores the procedural city seed rather than the full 64x64 grid,
allowing deterministic regeneration on load.
