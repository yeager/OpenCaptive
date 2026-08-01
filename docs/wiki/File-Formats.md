# File Formats

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

The internal binary format of x3g files is under analysis.

## VGM — Liberation wall textures

Wall textures in Liberation use the `.VGM` format (referenced as `WALL*.VGM`). Internal structure is under analysis. Loaded via the wildcard pattern in the executable.

## Img — Liberation graphics

Pre-rendered graphics in Liberation's Amiga image format:

| File | Content |
| --- | --- |
| `MainSP.Img` | Main sprite sheet |
| `MainSP16.Img` | 16-colour sprite sheet |
| `3dView.Img` | 3D viewport frame/border |
| `Taxi.Img` | Taxi graphics |
| `backpack.img` | Inventory/backpack UI |

## AmSp — AMOS sprite bank

Used in Liberation CD32. `AmSp` signature followed by entry count.

Record structure: 10-byte header, then `words x height x (depth + 1) x 2` bytes. The extra plane is a 1-bit transparency mask. High bit of width word indicates odd bytes-per-row (the loader doubles low 15 bits and subtracts one).

Records are word-aligned: odd total payload is followed by one padding byte.

## FNT — Liberation fonts

Font data stored as `{N}Liberation.FNT` with 4 variants (N=0-3). Format under analysis.

## spr — Liberation sprites

Game menu sprites stored as `GameMenu.spr`. Format under analysis.

## IFF ANIM — Amiga animation

Liberation Amiga floppy version uses IFF ANIM (opt5 interleave). Contains FORM/ANIM chunks with:

- `PACK` — compressed frame data (planar images)
- `PALL` — 32-colour palette
- `SCPT` — animation script bytecode

The verified player reads big-endian 16-bit record sizes from SCPT, advancing by full size, stopping at zero size.

## MIDI — Captive music

Captive DOS uses standard MIDI files for music. Eight tracks identified by SHA-256. Supports AdLib, Roland MT-32, PC speaker and Sound Blaster output in the original (OpenCaptive uses a built-in 32-voice square-wave synthesizer with ADSR envelope).

## 8SVX — Amiga audio samples

IFF 8SVX format for audio samples. Contains VHDR (voice header), BODY (sample data). Used for Amiga sound effects. OpenCaptive's 8-channel mixer with channel stealing handles playback.
