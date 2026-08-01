# Game Preservation

This page documents the original Captive and Liberation releases, their platform variants, and preservation considerations.

## Captive (1990)

### Release history

| Year | Platform | Publisher | Notes |
| --- | --- | --- | --- |
| 1990 | Amiga | Mindscape | Original release by Tony Crowther |
| 1990 | Atari ST | Mindscape | Simultaneous with Amiga |
| 1992 | DOS | Mindscape | v1.06 (Oct 7 1992), LZEXE compressed |

### Platform differences

| Feature | DOS | Amiga | Atari ST |
| --- | --- | --- | --- |
| Resolution | 320x200 | 320x200 (256 overscan) | 320x200 |
| Colours | 32 (VGA) | 32 (OCS/ECS) | 16 (ST low) |
| Graphics format | PL5 (5bpp packed) | IFF ILBM / custom | Degas Elite / custom |
| Animation | ANM (XOR delta) | IFF ANIM | ANM variant |
| Compression | LZEXE (executable) | RNC Method 1 | RNC Method 1 |
| Sound | AdLib/Roland/SB/Speaker | Paula 4-channel | YM2149 |
| Music format | MIDI | Amiga module | Amiga module variant |
| Disk format | DOS files | ADF (880K OFS) | FAT12 disk image |
| Map generation | Identical PRNG | Identical PRNG | Identical PRNG |

The map generation algorithm and PRNG are identical across all platforms — the same mission seed produces the same dungeon layout. Visual assets differ in format but represent the same content.

Cross-platform verification: Amiga ADF panel sheet and DOS PL5 panel sheet produce identical 64,000-pixel output (verified by OpenCaptive's `captive_panel_match` tool).

### DOS executable details

```
Original (packed):    73,056 bytes, LZEXE v0.91
Unpacked:            144,556 bytes
Packed SHA-256:      71bcf404103f1ac2920800a8bc166939bb49a1204cf51bebce8aca7dd5faafde
Unpacked SHA-256:    fa7d5ca76d26f614476ed41f27cf737084942e9216b20b4605734df9ede9aee4
```

### Amiga media

Standard 3.5" DD floppy disk (880 KiB), OFS filesystem. The `amiga_ofs_inventory` tool inventories content by SHA-256 without using filenames.

### Preservation status

- DOS version: widely preserved, LZEXE decompression well understood
- Amiga version: ADF images commonly available in preservation archives
- Atari ST version: disk images available, FAT12 format straightforward
- All versions: map generation verified to produce identical output from same seed

## Liberation: Captive 2 (1993)

### Release history

| Year | Platform | Publisher | Notes |
| --- | --- | --- | --- |
| 1993 | Amiga | Mindscape | Original release, 5 floppy disks |
| 1994 | Amiga CD32 | Mindscape | CD version with Red Book audio, CityGen 1.12 |

No DOS or Atari ST version was released.

### Platform differences

| Feature | Amiga floppy | Amiga CD32 |
| --- | --- | --- |
| Media | 5x ADF (880K each) | 1x CD-ROM (ISO9660 + audio) |
| Animation | IFF ANIM opt5 | IFF ANIM (PACK/PALL/SCPT) |
| Compression | RNC Method 1 | RNC Method 2 |
| Music | Amiga module | 10 Red Book CD audio tracks |
| CityGen | Built-in | External executable (v1.12, 1994-01-03) |
| Sprites | AMOS sprite bank | AMOS sprite bank (AmSp) |

### CD32 disc structure

```
Track 01:  Data (ISO9660, MODE1/2352, CDTV extensions)
Track 02:  Audio — Mark Knight
Track 03:  Audio — Mark Knight
...
Track 11:  Audio — Mark Knight
```

Data track SHA-256: `f807b1385c0996d54ed10afab271a7dd31d2c6dc6a18f13196ad2a79a0af8a80`

### CD32 filesystem contents

The ISO9660 filesystem contains:

| Category | Files | Description |
| --- | --- | --- |
| Executable | `captiveII` | Main game binary (245,628 bytes, 68000) |
| City generators | `CityGen`, `BuildingGen` | AmigaOS relocatable executables |
| 3D vectors | `*.x3g` | City, room, monster, NPC, object geometry |
| Wall textures | `WALL*.VGM` | VGM format wall textures |
| Sprites | `MainSP.Img`, `MainSP16.Img` | Sprite sheets |
| UI graphics | `3dView.Img`, `backpack.img`, `Taxi.Img` | Pre-rendered UI elements |
| Menu | `GameMenu.spr` | Menu sprites |
| Fonts | `{0-3}Liberation.FNT` | 4 font variants |
| Data | `CityGen`, `BuildingGen`, `Wall.Log` | Generation data and mappings |
| Saves | `Lib-Saves/` | Save game directory |

### Amiga floppy media

5 standard 3.5" DD floppy disks (4.4 MiB total):
- CaptiveII_Disk1 through CaptiveII_Disk4
- Boot disk
- OFS filesystem

### Preservation status

- CD32 version: raw BIN/CUE images preserved; ISO9660 is standard and readable
- Amiga floppy version: ADF images available; 5-disk set is complete
- Audio tracks: Red Book CD audio, preserved as part of BIN/CUE image
- CityGen/PlotGen: relocatable AmigaOS executables requiring 68000 analysis

## OpenCaptive's approach to preservation

### Content identity

OpenCaptive uses SHA-256 content hashes as the sole identity for game resources. This means:

- Renamed files are still recognized
- Repacked archives work transparently
- Different release dumps of the same content match
- Corrupted or modified files are rejected

### No redistribution

Original game data is never committed to the repository, bundled in releases, or emitted by tools. Discovery tools (`amiga_ofs_inventory`, `liberation_inventory`) output only digests and byte counts.

### Verification tools

```sh
# Verify all known resources
opencaptive --data /path/to/media --verify-data all

# Inventory Amiga media
./build/amiga_ofs_inventory /path/to/media <adf-sha256>

# Inventory Liberation CD32
./build/liberation_inventory /path/to/media

# Extract resource for offline analysis (output stays outside repo)
./build/liberation_extract /path/to/media <resource-sha256> /tmp/output.bin
```

### Visual parity verification

OpenCaptive includes frame-comparison tools for pixel-exact verification:

- `--capture-frame` — capture a rendered frame
- `--compare-frames` — compare two frames
- `--compare-frames-rect` — compare a specific region
- `--extract-dos-vga` — extract VGA state from a DOS memory dump

Current verification status:
- Liberation intro: 320x162 region — **0 deviating pixels** (pixel-perfect)
- Liberation city: 320x167 region — **0 deviating pixels** (pixel-perfect)
- Captive viewport: under active work (panel projection recovery in progress)

## External references

- [The Ultimate Captive Guide](https://captive.atari.org/Technical/MapGen/Introduction.php) — map generation documentation
- [Hall of Light — Captive](https://hol.abime.net/1197) — Amiga release database entry
- [Hall of Light — Liberation](https://hol.abime.net/2634) — Amiga/CD32 release database entry
