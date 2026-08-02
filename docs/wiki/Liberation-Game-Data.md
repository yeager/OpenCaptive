# Liberation Game Data

Data extracted from the verified CD32 executable `captiveII` (Ratt V2.02, Wyvern V2.00c). The binary is a 245,628-byte Amiga 68000 executable with four hunks.

## Version information

```
$VER: Liberation : Ratt V2.02 : Wyvern V2.00c
Platform: Amiga CD32
```

"Ratt" and "Wyvern" are internal engine/tool codenames by Tony Crowther.

## Procedural city generation

Liberation generates cities procedurally. There are 4 city visual variants (0-3), each with its own set of 3D vectors, monsters, droids and fonts.

### German-style place name generation

City names are composed from German syllable parts:

```
gold  grun  braun  wein  eisen  ein  bieder  jaeger
tell  fern  hahn   stein mann   berg er      mensch
tod   fisch rhein  kurz  laben  weg  wehr    kropf
esel  erz   fach   fuchs schild ort  paar    platz
```

Names are formed by concatenating 2-3 parts (e.g., "Goldstein", "Eisenberg", "Fuchsplatz").

### Street types

Streets are named with standard English suffixes:

```
Street       Avenue       Road        Boulevard    Lane
Park         Gardens      Gate        Crescent     Mews
Terrace      Drive        Alley       Approach     Arcade
Buildings    Corner       Quadrant    Passage      Promenade
```

## NPC names

### First names (35)

```
Max      Joe      Jack     Jim      Ed       Ann      June     Fred
Kiku     Art      John     Barbara  Steve    Rick     Mark     Barry
Deb      Malc     Tone     Pete     Alison   Maggi    Claire   Kim
Cass     Lynne    Mel      Gill     Cath     Ivor     Huw      Gwynn
Geraint  Narish   Rajiv
```

A mix of English, Welsh (Huw, Gwynn, Geraint), Japanese (Kiku), and South Asian (Narish, Rajiv) names reflecting the multicultural sci-fi setting.

### Last names (32)

```
featherstonehaugh  patel     smythe     singh      meridew    sagan
murray             cherryh   chambers   gaiman     cleese     simon
burkinshaw         forward   rector     alexy      jacobs     crowther
macleod            leary     crowley    gibson     floyd      whittle
pitt               heath     goodley    chapman    palin      gilliam
hake               turbot
```

References to sci-fi authors (Sagan, Cherryh, Gibson), Monty Python members (Cleese, Chapman, Palin, Gilliam), and the developer himself (Crowther).

### NPC titles

```
Governor
Councillor
Investment Counsellor
Engineer
Entrepreneur
Pilot
Trader
Merchant
```

## Building types

### Shops (9 types)

| Shop | Description |
| --- | --- |
| Emporium | General goods |
| Hardware | Tools and parts |
| Pawnshop | Used items |
| Gunsmith | Weapons |
| Jewellers | Luxury items |
| Store | General goods |
| Market | General goods |
| Newsagent | Information |
| TriDee Rental | 3D media rental |

### Bars (12 types)

```
Bar             Boozerama       Diner           PopShop
Pub             Vinyard         Beer Hall       Pool Rooms
Speakeasy       Gin Palace      cha no mise     biru no mise
```

The last two are Japanese (tea house, beer shop), matching the multicultural theme.

### Businesses (11 types)

```
Software                Insurance               Assurance
Bank                    Accountants              Actuarial Services
Credit Control          Mercantile               Import/Export
Productions             Expediters
```

### Industrial (12 types)

```
Engineering             Ceramics                 Steel
Aluminium Corp.         Laser Systems            TeleCommunications
Plastics                Mouldings                Metals a.g.
Optik g.m.b.h           ParaGravitics            Noble Gasses
```

Note the German corporate suffixes (a.g., g.m.b.h) — consistent with the German city names.

## 3D rendering system

Liberation uses true 3D polygon rendering, a major advancement over Captive's panel-based viewport.

### Vector data files

All 3D models are stored in `.x3g` format with 4 city variants:

| Category | Files per variant | Content |
| --- | --- | --- |
| Cities | `{0-3}CityVectors.x3g` | Building exteriors, streets |
| Rooms | `{0-3}RoomVectors.x3g` | Interior geometry |
| Banks | `{0-3}BankVectors.x3g` | Bank interiors |
| Bars | `{0-3}BarVectors.x3g` | Bar/pub interiors |
| Shops | `{0-3}ShopVectors.x3g` | Shop interiors |
| Droids | `{0-3}Droids.x3g` | Player droid models |
| City monsters | `{0-3}CityMonsters.x3g` | Street enemies |
| Police | `PoliceVectors.x3g` | Police/guard models (shared) |
| Extra monsters | `Monsters{2,3,4}.x3g` | Additional enemy sets |
| NPCs | `People.x3g` | Civilian models |
| Items | `Objects.x3g` | Pickup/world items |

### Textures

- `WALL*.VGM` — wall textures in VGM format (wildcard loaded)
- `3dView.Img` — 3D viewport frame/border overlay

## Data generation files

| File | Size | Content |
| --- | --- | --- |
| `CityGen` | 10,896 bytes | City layout generator (AmigaOS executable, v1.12, built 1994-01-03) |
| `BuildingGen` | 23,252 bytes code + 3,956 BSS | Building/road/naming generator |
| `PlotGen` | 12,388 bytes code + 27,016 BSS | Plot/interior generator with ArcD decompressor |

CityGen is a relocatable AmigaOS `loadseg()` executable. It receives a parameter block, clears a 12,288-byte work area and generates a 64×64 city grid.

BuildingGen generates the building placement graph with 9 types, road connections, city names (German syllables + Greek suffix) and building names from type-specific string tables.

PlotGen contains the ArcD Huffman+LZSS decompressor (offsets 0x302–0x520) used to decompress three text data files:

| File | Compressed | Decompressed | Content |
| --- | --- | --- | --- |
| PGE.txt | 7,085 bytes | 16,304 bytes | Plot/game event text |
| DTE.txt | 5,323 bytes | 14,136 bytes | Dialogue text |
| CTE.txt | 8,230 bytes | 17,809 bytes | City text (location descriptions) |

The ArcD format uses canonical Huffman codes with three tables per block (literal count, match offset, match length) and LZSS back-references. See [[File Formats]] for full format documentation.

## Graphics assets

All Img files use the `ImgA` container format with planar Amiga bitplanes + transparency mask.

| File | Sprites | Dimensions | Planes | Content |
| --- | --- | --- | --- | --- |
| `MainSP.Img` | 158 | 10–128px wide | 1–6 | Main sprite sheet (UI elements, icons, cursors) |
| `3dView.Img` | 23 × 6 frames | 4–16px | 4–6 | 3D viewport border (distance-LOD multi-frame) |
| `Taxi.Img` | 4 | 188–192px | 6 | Taxi/transport graphics |
| `backpack.img` | 176 | 9–32px | 4–6 | Inventory/backpack UI elements |
| `GameMenu.spr` | — | — | — | Game menu sprites (AmSp format) |

Total: 361 decoded sprites (338 simple + 23 multi-frame with 6 LOD levels each).

## Font system

2 font variants found on CD32: `0Liberation.FNT` (large, up to 9px wide) and `1Liberation.FNT` (small, up to 6px wide). Both use `CHAR` format: 114 proportional-width glyphs (ASCII 32–145), 7 rows each, 2 bitplanes (foreground + drop-shadow outline).

## Audio

CD32 version has 10 audio CD tracks (Tracks 02-11) composed by Mark Knight. Track 01 is the data track.

## Disc structure

### CD32 version

- Track 01: ISO9660 data track (MODE1/2352)
- Tracks 02-11: Red Book audio
- Save directory: `Lib-Saves`

### Amiga floppy version

- 5 disks: `CaptiveII_Disk1` through `CaptiveII_Disk4` (+ boot disk)
- OFS filesystem
- IFF ANIM opt5 for animations
- RNC compression for assets

## Executable structure

```
Format:       Amiga 68000 LoadSeg executable
Binary size:  245,628 bytes
Hunks:        4 (1 code, 1 data, 2 BSS)
Code hunk:    225,396 bytes at offset 44
RELOC32:      1,356 entries
SHA-256:      db61f7e39fd31ac19b82216ea963711728d25518454fae42fd89c5bab52f2215
```
