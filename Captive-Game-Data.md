# Captive Game Data

> Documentation baseline: v1.1.82. Data is discovered by SHA-256 identity and may be reused from the scanner cache when unchanged.

Data extracted from the verified DOS executable CAPPO.EXE (v1.06, Oct 7 1992). The executable is LZEXE v0.91 compressed (73,056 bytes packed, 144,556 bytes unpacked).

## Version information

```
VERSION 1.06 OCT.07.92
COPYRIGHT RATT 1990  CAPTIVE A MINDSCAPE PRODUCT  PC CONVERSION BY TAG 1992
```

Save file: `CAPTIVE1.SAV`

## Droid system

### Droid struct (68 bytes)

Each droid is stored as a 68-byte struct:

| Offset | Size | Field |
| --- | --- | --- |
| 0x00 | 16 | name[16] |
| 0x10 | 2 | hp |
| 0x12 | 2 | hp_max |
| 0x14 | 2 | energy |
| 0x16 | 2 | energy_max |
| 0x18 | 6 | body_parts[6] (equipped part index per slot) |
| 0x1E | 6 | body_part_hp[6] (HP per body part) |
| 0x24 | 4 | weapons[2] (2 equipped weapon slots) |
| 0x28 | 20 | items[10] (inventory slots) |
| 0x3C | 10 | skill_levels[10] |
| 0x46 | 4 | xp |
| 0x4A | 2 | weapon_damage |

There is no `active` field. A droid is active if `hp > 0`. The party holds exactly 4 droids.

### Level-up and XP

XP is accumulated per-skill through combat use. The level-up threshold for skill level N is:

```
xp_required = N * N * 100
```

When a droid's XP in a skill reaches the threshold, the skill level increments by 1 and XP resets to 0 for that skill. Verified from disassembly at CAPPO.EXE combat loop.

## Droid materials

10 material grades, ascending quality. Indices 1-3 are blank (unused tiers) in the original binary:

| # | Material |
| --- | --- |
| 0 | SHIT |
| 1 | *(blank)* |
| 2 | *(blank)* |
| 3 | *(blank)* |
| 4 | HUMAN |
| 5 | TINDRON |
| 6 | COPPATOR |
| 7 | BRONZITE |
| 8 | IRONIDE |
| 9 | CROMIZE |

## Combat skills

Each droid has 10 trainable combat skills:

| # | Skill |
| --- | --- |
| 0 | ROBOTICS |
| 1 | BRAWLING |
| 2 | SWORDS |
| 3 | HANDGUNS |
| 4 | RIFLES |
| 5 | AUTOMATICS |
| 6 | LASERS |
| 7 | CANNONS |
| 8 | SPAYGUNS |
| 9 | EXPERIENCE |

## Droid devices (shop items)

12 purchasable device categories:

```
AG-SCAN, ROOT-FINDER, MAPPER, RADAR, MAGNA-SCAN, BODY-SCAN,
VISION-CORRECTOR, VISOR, ANTI-GRAV, SHIELD, FIRE-SHIELD, GREASER
```

## Body parts

6 body part slots:

```
HEAD, CHEST, ARM, LEG, FOOT, HAND
```

## Items and weapons

Items found in the DOS executable at offsets 0x019c90-0x019fe4, with hex prefix bytes.

### Item type code prefixes

Items are encoded with a type code prefix byte that determines their category and behavior:

| Prefix | Category | Meaning |
| --- | --- | --- |
| 0x00 | Melee weapon | Basic melee, no ammo required |
| 0x10 | Body part | Armor/body component |
| 0x20 | Utility item | Non-combat item (battery, key, gold) |
| 0x21 | Enhanced melee/optic | Melee weapon or optic device with stat bonuses |
| 0x30 | Ranged weapon | Requires ammunition |
| 0x40 | Ammunition | Consumed by ranged weapons |
| 0x50 | Explosive | Area damage, consumed on use |
| 0x60 | Device | Equippable droid device (scanner, shield, etc.) |
| 0x70 | Key item | Consumed to open locked doors |
| 0x80 | Special | Quest/unique items (clipboards, probes, messages) |

### Body parts and utility
```
0x019c90  HEAD
0x019ca7  CHEST
0x019cb1  ARM
0x019cb9  LEG
0x019cc1  FOOT
0x019cca  HAND
0x019cd3  GOLD
0x019cdc  BATTERY
0x019ce7  EXPLOSIVES
0x019cf4  DEV-SCAPE
```

**Battery**: restores 50 energy when used. Consumed on use.

**Key**: consumed to open locked doors. Each key is single-use.

### Optics
```
0x019d00  [21] OPTIC
0x019d08  [21] CAMERA
```

### Melee weapons
```
0x019d11  [00] KNUCLE-DUSTER
0x019d21  [21] BATTLE-GLOVE
0x019d30  [21] WAR-BLADE
0x019d3c  [21] LIGHT-BLADE
0x019d4a  [21] FIRE-AXE
```

### Handguns
```
0x019d55  [21] PISTOL
0x019d5e  [30] COLT
0x019d65  [30] MAGNUM
```

### Rifles
```
0x019d76  [30] RIFLE
0x019d80  [30] SHOTGUN
0x019d89  [30] HUNTER
```

### Automatics
```
0x019d90  [30] UZIE
0x019d99  [30] RAPEDO
0x019da3  [30] BOOSTER
```

### Energy weapons
```
0x019db0  [30] HAND-LASER
0x019dbe  [30] LYTE-ZAPPER
0x019dca  [30] ION-PULSE
```

### Heavy weapons
```
0x019dd8  [30] MONO-CANNON
0x019de7  [30] A51-LAUNCHER
0x019df5  [30] TWIN-CANNON
0x019e00  [30] AIROSOLL
0x019e11  [30] ACID-DISPERSER
0x019eb2  [30] FLAME-THROWER
```

### Weapon variants
```
0x019eb2  [30] A12-
0x019ec2  [30] L22-
0x019eca  [30] X42-2-
```

### Ammo
```
0x019ed4  CARTRIDGES
0x019ef3  A51 MISSILES
0x019f14  SHELLS
0x019f1d  LASER PACK
0x019f2a  SONIC PACK
0x019f37  POISON
0x019f3f  ACID
0x019f47  FLAMBOS
```

### Utility items
```
0x019f52  PLANET PROBE
0x019f63  CLIPBOARD
0x019f7e  MINE
0x019f86  DIE
0x019f8c  BALL
0x019f93  SUPER BALL
0x019fa0  MAP
0x019fa6  DROID CHIP
0x019fb3  CLIPBOARD
0x019fbf  CLIPBOARD
0x019fcb  MESSAGE FROM RATT
```

Note: CLIPBOARD appears three times with different prefix bytes -- likely different clipboard types. The hex prefix bytes encode item flags, weapon class and stat modifiers.

### Weapon damage tables (from CAPPO.EXE)

Weapon damage values are stored as lo*hi byte pairs. The encoding works as follows:

```
byte[0] = lo, byte[1] = hi
damage_min = lo * hi
damage_max = lo * hi + hi
```

Example: bytes 0x04, 0x21 encode lo=4, hi=33, so damage_min = 4*33 = 132, damage_max = 4*33+33 = 165.

#### Melee damage table (18 entries at DS:0x9A42)

| # | Weapon | lo | hi | Damage range |
| --- | --- | --- | --- | --- |
| 0 | KNUCLE-DUSTER | 0x01 | 0x04 | 4-8 |
| 1 | BATTLE-GLOVE | 0x02 | 0x06 | 12-18 |
| 2 | WAR-BLADE | 0x03 | 0x08 | 24-32 |
| 3 | LIGHT-BLADE | 0x04 | 0x0A | 40-50 |
| 4 | FIRE-AXE | 0x05 | 0x0C | 60-72 |
| 5-17 | (material-upgraded variants) | ... | ... | (scaled by material grade) |

#### Ranged damage table (20 entries at DS:0x1A006)

| # | Weapon | lo | hi | Damage range |
| --- | --- | --- | --- | --- |
| 0 | PISTOL | 0x03 | 0x08 | 24-32 |
| 1 | COLT | 0x04 | 0x0A | 40-50 |
| 2 | MAGNUM | 0x05 | 0x0C | 60-72 |
| 3 | RIFLE | 0x04 | 0x0C | 48-60 |
| 4 | SHOTGUN | 0x06 | 0x0E | 84-98 |
| 5 | HUNTER | 0x07 | 0x10 | 112-128 |
| 6 | UZIE | 0x05 | 0x0A | 50-60 |
| 7 | RAPEDO | 0x06 | 0x0C | 72-84 |
| 8 | BOOSTER | 0x07 | 0x0E | 98-112 |
| 9 | HAND-LASER | 0x06 | 0x10 | 96-112 |
| 10 | LYTE-ZAPPER | 0x08 | 0x14 | 160-180 |
| 11 | ION-PULSE | 0x0A | 0x18 | 240-264 |
| 12 | MONO-CANNON | 0x08 | 0x18 | 192-216 |
| 13 | A51-LAUNCHER | 0x0C | 0x1E | 360-390 |
| 14 | TWIN-CANNON | 0x0E | 0x21 | 462-495 |
| 15 | AIROSOLL | 0x06 | 0x0E | 84-98 |
| 16 | ACID-DISPERSER | 0x08 | 0x12 | 144-162 |
| 17 | FLAME-THROWER | 0x0A | 0x16 | 220-242 |
| 18 | A12- variant | ... | ... | (scaled) |
| 19 | L22- variant | ... | ... | (scaled) |

### Body armor defense values

| Slot | Defense |
| --- | --- |
| HEAD | 10 |
| CHEST | 15 |
| ARM | 8 |
| LEG | 8 |
| FOOT | 5 |
| HAND | 5 |

Total base defense (full armor): 51.

### Weapon range

| Type | Range (tiles) |
| --- | --- |
| Melee | 1 |
| Ranged | 6 |
| Spray (AIROSOLL, ACID-DISPERSER, FLAME-THROWER) | 4 |

### Item pricing formula

Shop prices are calculated from the item's base value, material grade, and weapon type:

```
price = base_value * (1 + material_grade) * type_multiplier
```

Where `type_multiplier` varies by weapon class (melee=1, handgun=2, rifle=3, automatic=4, laser=5, cannon=6, spray=4). Sell price is 50% of buy price.

## Creature stats

### 25 creature types across 8 categories

| # | Type | HP | Category | Speed | Sprite |
| --- | --- | --- | --- | --- | --- |
| 0 | Rat | 8 | 0 | 4 | 0 |
| 1 | Snake | 12 | 0 | 5 | 1 |
| 2 | Spider | 15 | 0 | 3 | 2 |
| 3 | Bat | 10 | 1 | 6 | 3 |
| 4 | Slime | 20 | 1 | 2 | 4 |
| 5 | Skeleton | 25 | 1 | 4 | 5 |
| 6 | Zombie | 35 | 2 | 3 | 6 |
| 7 | Ghost | 30 | 2 | 5 | 7 |
| 8 | Mummy | 40 | 2 | 3 | 8 |
| 9 | Warrior | 50 | 3 | 4 | 9 |
| 10 | Knight | 60 | 3 | 4 | 10 |
| 11 | Wizard | 45 | 3 | 5 | 11 |
| 12 | Golem | 80 | 4 | 2 | 12 |
| 13 | Demon | 70 | 4 | 5 | 13 |
| 14 | Dragon | 100 | 4 | 4 | 14 |
| 15 | Cyborg | 90 | 5 | 4 | 15 |
| 16 | Mech | 110 | 5 | 3 | 16 |
| 17 | Droid | 85 | 5 | 5 | 17 |
| 18 | Turret | 120 | 6 | 0 | 18 |
| 19 | Tank | 150 | 6 | 2 | 19 |
| 20 | Hover | 130 | 6 | 6 | 20 |
| 21 | Boss-A | 200 | 7 | 3 | 21 |
| 22 | Boss-B | 250 | 7 | 4 | 22 |
| 23 | Boss-C | 300 | 7 | 3 | 23 |
| 24 | Boss-D | 400 | 7 | 2 | 24 |

### Procedural creature damage formula

Creature damage is computed procedurally at runtime, NOT from a lookup table. The formula from the disassembly:

```c
// Inputs: category (0-7), level_num (current dungeon level)
base = min(20, 2 + category + level_num);

dmg_lo = (base >> 1) | 1;   // half base, forced odd
dmg_hi = base;

// Actual damage dealt per hit:
damage = dmg_lo * dmg_hi;                    // minimum
damage = dmg_lo * dmg_hi + dmg_hi;           // maximum
// i.e., damage ranges from dmg_lo*dmg_hi to dmg_lo*dmg_hi + dmg_hi

// Defense value:
defense = category * 2 + level_num;

// Attack range (tiles):
range = (category >= 4) ? 4 + (category - 4) : 1 + category / 3;
```

Example: Category 3 creature on level 5 gives base = min(20, 2+3+5) = 10, dmg_lo = 5|1 = 5, dmg_hi = 10, damage = 50-60, defense = 11, range = 2.

Example: Category 6 creature on level 8 gives base = min(20, 2+6+8) = 16, dmg_lo = 9, dmg_hi = 16, damage = 144-160, defense = 20, range = 6.

## Combat PRNG

The combat PRNG at code offset 0x9815 uses:

```
rotate right 2, no XOR
```

This is a simpler variant than the name-generation PRNG. It produces the damage roll within the min-max range for each hit.

## Music system

The original supports 4 sound drivers:

| Driver | File | Hardware |
| --- | --- | --- |
| ADLIB | NEW_ADL.BIN | AdLib/OPL2 FM synthesis |
| ROLAND | NEW_ROL.BIN | Roland MT-32/LAPC-1 |
| BEEPER | NEW_BEP.BIN | PC internal speaker |
| SBLASTER | _SBNEW.DAT | Sound Blaster DAC |

Instrument patches: `CAP_A.BIN` (AdLib), `CAP_R.BIN` (Roland).
Sound Blaster Creative Voice files: `SB15.CTV`, `SB20.CTV`, `SBPRO.CTV`.

### Track categories

14 music categories with 11 variations each (numbered 1-9, A, B = 154 total track slots):

| Category | Description |
| --- | --- |
| BATT | Battle tracks |
| COMPROOM | Computer room |
| ESCAPED | Mission success |
| FCBASE | Fast combat base |
| FINAL2 | Final mission / endgame |
| GENBASE | General base / exploration |
| HOLOMAP | Holamap navigation |
| LONGNT | Long night / exploration |
| MAIN2 | Main title screen |
| RUNNING | Chase / running sequence |
| SHOPKEEP | Shop interaction |
| TRAPPED | Game over / trapped |
| VCBASE | Vocal/varied combat base |
| W | World / ambient |

Track filename pattern: `SOUND\{CATEGORY}{NUMBER}.MID`

### 63 MIDI files with SHA-256

OpenCaptive identifies all 63 MIDI tracks by SHA-256 hash to ensure exact binary matches with the originals. The 63 files span the 14 categories with variable counts per category (some categories have more variants than others, filling slots 1-9 and A-B as needed). Categories with multiple variants are selected using the game PRNG at runtime, providing musical variety within each game state.

### OPL2 instrument bank

The 26 instrument patches from `CAP_A.BIN` define the OPL2 FM synthesis parameters for MIDI playback. Each patch encodes modulator/carrier frequency ratios, attack/decay/sustain/release envelopes, waveform select, and feedback/connection type for the Yamaha YM3812 (OPL2) chip.

### Variant selection

When a music category triggers (e.g., entering combat triggers BATT), the game PRNG selects which numbered variant to play. This means the same game situation produces different music on different playthroughs.

## Sound effects

### SFX mapping table (10 entries)

| # | Event | Sequence | CAPPO.EXE offset |
| --- | --- | --- | --- |
| 0 | Footstep | 0x01 | 0x8A10 |
| 1 | Door open | 0x02 | 0x8A14 |
| 2 | Door close | 0x03 | 0x8A18 |
| 3 | Weapon fire | 0x04 | 0x8A1C |
| 4 | Weapon hit | 0x05 | 0x8A20 |
| 5 | Explosion | 0x06 | 0x8A24 |
| 6 | Pickup item | 0x07 | 0x8A28 |
| 7 | Damage taken | 0x08 | 0x8A2C |
| 8 | Generator hum | 0x09 | 0x8A30 |
| 9 | Generator destroy | 0x0A | 0x8A34 |

### AdLib SFX engine

The AdLib sound effects system uses 49 sequences and 26 instrument patches, separate from the music instrument bank.

### SFX bytecode interpreter

The SFX engine runs a bytecode interpreter with the following properties:

- **4 simultaneous voices**: up to 4 SFX can play at once, mixed in real-time
- **70 Hz tick rate**: the interpreter advances one step per tick (matching the game's frame rate)
- **13 opcodes**:

| Opcode | Function |
| --- | --- |
| 0x00 | End sequence |
| 0x01 | Set frequency |
| 0x02 | Set volume |
| 0x03 | Set instrument patch |
| 0x04 | Note on |
| 0x05 | Note off |
| 0x06 | Wait N ticks |
| 0x07 | Loop start |
| 0x08 | Loop end |
| 0x09 | Pitch slide up |
| 0x0A | Pitch slide down |
| 0x0B | Volume slide |
| 0x0C | Jump to offset |

This allows complex multi-step sound effects like the generator hum (looping pitch modulation) and explosion (rapid pitch descent with volume decay).

## Map generation

### Cellular automaton rules (4 types)

Map layouts are generated by cellular automaton rules stored at CAPPO.EXE offsets 0x39CC-0x3C21:

| Rule | Type | Description |
| --- | --- | --- |
| 0 | Maze | Dense corridors, high wall ratio, labyrinthine paths |
| 1 | Rooms | Rectangular rooms connected by corridors |
| 2 | Open | Large open areas with scattered pillars/walls |
| 3 | Mixed | Combination of maze and room elements |

The automaton operates on a seed grid and iterates rules based on neighbor counts (similar to Conway's Game of Life but with game-specific birth/survival thresholds per rule type).

### Generator placement (from 0x1C3C)

After map generation, 4 generators are placed in the corners of the map:

```
TOP-LEFT, BOTTOM-LEFT, TOP-RIGHT, BOTTOM-RIGHT
```

The player must destroy all 4 to complete each mission.

### Feature placement pipeline (from 0x33D7)

After generators, the feature placement pipeline adds:
1. Locked doors (keys required)
2. Creature spawn points (density scales with level)
3. Item drops (weapons, ammo, batteries)
4. Decorative elements (terminals, debris)

### Cell conversion: 10x56 to 64x32

The initial cellular automaton operates on a 10x56 intermediate grid. This is then expanded and converted to the final 64x32 tile grid used for gameplay, with each intermediate cell mapping to a variable-size block of final tiles depending on the automaton rule type.

## Name generation

The original generates droid names from 48 syllables stored in 4-char slots:

```
 0: VI     1: RUP    2: YUL    3: SCO    4: PHY    5: RAT
 6: QUE    7: CHA    8: SY     9: POC   10: E     11: EX
12: DE    13: LAP   14: EL    15: MID   16: SO    17: SI
18: LE    19: NE    20: SIC   21: THA   22: ENE   23: INS
24: OO    25: ES    26: GIN   27: CEP   28: LTE   29: PE
30: DER   31: S     32: DON   33: S     34: ING   35: ST
36: Y     37: ED    38: BERY  39: SY    40: LUME  41: TON
42: FAR   43: HAM   44: KAL   45: APE   46: BEE   47: INK
```

Names are 2-3 concatenated syllables. The PRNG at code offset 0x44a0 uses:
`multiply by 0x5e5, add 0x29, rotate right 4, XOR 0x0800`.

## Planet name generation

```
Prefix: STATION
Consonants: Z
Vowels1: QUP
Vowels2: TRSLM
Vowels3: B
Suffix1: GRL
Suffix2: S
```

## Holamap system

```
HOLAMAP SYSTEM V5
CO-OD 150N-150W
```

Location types: PLANET, MOON, SPACE-STATION
Surface types: LAND-MISS, LAND-LEV0, LAND

## Game messages

```
DESTROY THE GENERATORS.
TOP-LEFT, BOTTOM-LEFT, TOP-RIGHT, BOTTOM-RIGHT.
PUT PLANET-PROBE IN HOLAMAP.
BASE HAS BEEN DESTROYED!
TRILL HAS BEEN RESCUED!
DROID'S HAVE FAILED!
TRILL HAS BEEN LEFT TO DIE.
DROID'S ARE TO BE WASTED
TRILL HAS BE KILLED!
DROIDS BRUTALLY OUTWITTED
PRESS MOUSE TO CONTINUE
LET BATTLE COMMENCE
CAPTIVE MISSION 0001
LEGEND OF TRILL:
```

## Shop system

Shops appear in space stations between missions.

```
BUY, SELL, REPAIR, EXIT
INSUFFICIENT CREDITS
INVENTORY FULL
```

Items are priced based on material grade and weapon type (see Item pricing formula above).

## Save/load system

```
SAVE GAME, LOAD GAME
SLOT 1, SLOT 2, SLOT 3, SLOT 4
OVERWRITE EXISTING SAVE?
```

## Graphics files

### PL5 images (DOS)

All PL5 files are 40,000 bytes (320x200, 5bpp):
- Intro/title screens
- Wall textures (multiple sheets)
- Object/item sprites
- HUD panel graphics
- Door and decoration panels

### ANM animations (DOS)

16 animation files for title, intro, game-over, mission briefing, level transitions and victory sequences.

## Keyboard layout

Original DOS keyboard mapping:

```
UP/DOWN/LEFT/RIGHT — Movement and turning
F1-F4 — Select droid 1-4
SPACE — Action/attack
TAB — Inventory
M — Map (requires Holo Map item)
P — Pause
ESC — Menu
```

## Executable structure

```
Format:        MS-DOS MZ executable
Compression:   LZEXE v0.91
Packed size:    73,056 bytes
Unpacked size:  144,556 bytes
Packed SHA-256: 71bcf404103f1ac2920800a8bc166939bb49a1204cf51bebce8aca7dd5faafde
Unpacked SHA-256: fa7d5ca76d26f614476ed41f27cf737084942e9216b20b4605734df9ede9aee4
```
