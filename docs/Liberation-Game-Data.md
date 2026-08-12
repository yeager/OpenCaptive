# Liberation Game Data

> Documentation baseline: v1.1.109. Liberation media is verified by content identity before use.

Data extracted from the verified CD32 executable `captiveII` (Ratt V2.02, Wyvern V2.00c). The binary is a 245,628-byte Amiga 68000 executable with four hunks.

## Version information

```
$VER: Liberation : Ratt V2.02 : Wyvern V2.00c
Platform: Amiga CD32
```

"Ratt" and "Wyvern" are internal engine/tool codenames by Tony Crowther.

## City system

### Grid layout

Cities are generated on a 64x64 grid with 3 vertical planes:

| Plane | Content |
| --- | --- |
| 0 | Ground level: streets, buildings, exterior features |
| 1 | Upper level: elevated walkways, upper floors |
| 2 | Underground: tunnels, basements, sewers |

### 9 building types

| # | Type | Interior |
| --- | --- | --- |
| 0 | Residential | Living quarters |
| 1 | Shop | Merchandise display, counter |
| 2 | Bar | Tables, bar counter, drinks |
| 3 | Business | Office layout, desks |
| 4 | Industrial | Machinery, hazards |
| 5 | Government | Official chambers |
| 6 | Police station | Cells, front desk |
| 7 | Bank | Vault, teller windows |
| 8 | Special | Mission-specific layout |

### 8 city visual themes

Each mission selects one of 8 city visual themes. Wall colors vary by mission number:

| Theme | Wall palette | Used in missions |
| --- | --- | --- |
| 0 | Grey stone | 1, 9, 17, ... |
| 1 | Red brick | 2, 10, 18, ... |
| 2 | Blue metal | 3, 11, 19, ... |
| 3 | Green organic | 4, 12, 20, ... |
| 4 | Brown wood | 5, 13, 21, ... |
| 5 | White marble | 6, 14, 22, ... |
| 6 | Dark steel | 7, 15, 23, ... |
| 7 | Gold trim | 8, 16, 24, ... |

Theme is selected as `mission_number % 8`. The 4 city vector variants (0-3) map to themes via `theme % 4`.

### Building exterior colors

Building exterior colors vary by building ID (`building_id % palette_count`), giving each building a visually distinct appearance within the city theme.

### Building interior floor plans

Building interior layouts vary by `building_index`. Each building type has a set of floor plan templates, and the specific plan is selected using the building's index, so different buildings of the same type have different internal layouts.

### Street layout

Streets are generated from grid connections between buildings. The grid connectivity algorithm places roads along cardinal directions, creating a network of streets, avenues, and alleys. Street names are generated from the name tables (see below).

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

## NPCs and shops

### NPC type indicator icons

NPCs display a type indicator icon above their head in the 3D view, showing their role:

| Icon | NPC type |
| --- | --- |
| Star | Governor / high-rank official |
| Coin | Trader / merchant |
| Wrench | Engineer |
| Badge | Police |
| Scroll | Information source |
| Mug | Bar patron |

### Shop interface

When entering a shop, the interface displays:
- **Item count**: number of items currently in stock
- **Gold display**: shop's total gold reserves and player's gold
- Items are listed with name, condition, and price

### Dialogue options

NPCs offer context-dependent dialogue:

| Option | Effect |
| --- | --- |
| Trade | Opens buy/sell interface (merchants only) |
| Ask around | Provides hints about mission objectives, building locations, or gossip |
| Buy drinks | Available in bars; consumed immediately; 25% chance triggers a bar fight |
| Leave | Exit conversation |

### Bar fights

After buying drinks in a bar, there is a 25% chance of a bar fight:
- Combat initiates with 1-3 bar patrons
- Patrons have stats scaled to current mission level
- Reputation decreases by 10 after a bar fight

### Police fines

After a bar fight, police may arrive:
- Fine: 100 gold
- Paying the fine increases reputation by +15
- Refusing the fine triggers combat with police

## Reputation system

| Event | Change |
| --- | --- |
| Starting value | 0 |
| Bar fight | -10 |
| Police fine paid | +15 |
| Mission complete | +20 |
| Attacking civilians | -25 |
| Helping NPCs | +5 |

Range: -100 to +100. Reputation affects NPC willingness to trade and provide information. At -50 or below, shops refuse service. At +50 or above, shops offer discounts.

## Industrial hazards

Buildings with `building_index % 3 == 0` trigger electrical hazards when entered:

```
Damage = 5 + mission * 2
```

Applied to all droids in the party simultaneously. The hazard triggers once on entry; moving within the building does not re-trigger it.

## Day/night cycle

The game runs a tick-based day/night cycle:

| Period | Hours | Sky color | Ground color |
| --- | --- | --- | --- |
| Day | 06:00 - 18:00 | Light blue | Normal palette |
| Dusk | 18:00 - 21:00 | Orange/red gradient | Darkening palette |
| Night | 21:00 - 06:00 | Dark blue/black | Dark palette |

Sky and ground colors interpolate smoothly between periods. Street lights activate at dusk. Creature spawn rates increase at night.

## Taxi system

### Phone box interaction

Taxis are called from phone box objects placed throughout the city:

1. Player interacts with phone box
2. Cost: 50 gold (deducted immediately)
3. Screen displays green fade effect with "TAXI" text overlay
4. Fade animation runs for 15 ticks
5. Player is teleported to the special building (mission objective or key location)

If the player has less than 50 gold, the phone box displays "INSUFFICIENT FUNDS" and no taxi is called.

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

Note the German corporate suffixes (a.g., g.m.b.h) -- consistent with the German city names.

## 3D rendering system

Liberation uses true 3D polygon rendering, a major advancement over Captive's panel-based viewport.

### 3D assets

#### VGM wall textures

71 wall texture sets stored in VGM format, extracted from the CD32 disc image. Loaded via wildcard pattern `WALL*.VGM`. Textures are assigned to city themes and building types.

#### x3g 3D vector objects

All 3D models use the x3g format (IFF FORM O3DG). The format stores vertex lists, face definitions with color/shading attributes, and optional animation keyframes.

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

#### Img sprite files

All Img files use the `ImgA` container format with planar Amiga bitplanes + transparency mask.

| File | Sprites | Dimensions | Planes | Content |
| --- | --- | --- | --- | --- |
| `MainSP.Img` | 158 | 10-128px wide | 1-6 | Main sprite sheet (UI elements, icons, cursors) |
| `3dView.Img` | 23 x 6 frames | 4-16px | 4-6 | 3D viewport border (distance-LOD multi-frame) |
| `Taxi.Img` | 4 | 188-192px | 6 | Taxi/transport graphics |
| `backpack.img` | 176 | 9-32px | 4-6 | Inventory/backpack UI elements |
| `GameMenu.spr` | -- | -- | -- | Game menu sprites (AmSp format) |

Total: 361 decoded sprites (338 simple + 23 multi-frame with 6 LOD levels each).

#### FNT font files

2 font variants found on CD32:

| File | Size | Width |
| --- | --- | --- |
| `0Liberation.FNT` | Large | Up to 9px wide |
| `1Liberation.FNT` | Small | Up to 6px wide |

Both use the `CHAR` magic format:
- 114 proportional-width glyphs (ASCII 32-145)
- 7 rows per glyph
- 2 bitplanes (foreground + drop-shadow outline)

## PlotGen system

PlotGen generates mission content including:

- **Mission briefing**: procedurally generated text describing the mission objective
- **City name**: selected from the German syllable generator
- **Victim objective**: the person to rescue or item to retrieve

### ArcD compressed text files

PlotGen uses the ArcD decompressor (Huffman+LZSS) to decompress three text data files:

| File | Compressed | Decompressed | Content |
| --- | --- | --- | --- |
| PGE.txt | 7,085 bytes | 16,304 bytes | Plot/game event text |
| DTE.txt | 5,323 bytes | 14,136 bytes | Dialogue text |
| CTE.txt | 8,230 bytes | 17,809 bytes | City text (location descriptions) |

The ArcD format uses canonical Huffman codes with three tables per block (literal count, match offset, match length) and LZSS back-references. See [[File Formats]] for full format documentation.

## Data generation files

| File | Size | Content |
| --- | --- | --- |
| `CityGen` | 10,896 bytes | City layout generator (AmigaOS executable, v1.12, built 1994-01-03) |
| `BuildingGen` | 23,252 bytes code + 3,956 BSS | Building/road/naming generator |
| `PlotGen` | 12,388 bytes code + 27,016 BSS | Plot/interior generator with ArcD decompressor |

CityGen is a relocatable AmigaOS `loadseg()` executable. It receives a parameter block, clears a 12,288-byte work area and generates a 64x64 city grid.

BuildingGen generates the building placement graph with 9 types, road connections, city names (German syllables + Greek suffix) and building names from type-specific string tables.

PlotGen contains the ArcD Huffman+LZSS decompressor (offsets 0x302-0x520) used to decompress the text data files listed above.

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
