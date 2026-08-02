# Captive Game Data

Data extracted from the verified DOS executable CAPPO.EXE (v1.06, Oct 7 1992). The executable is LZEXE v0.91 compressed (73,056 bytes packed, 144,556 bytes unpacked).

## Version information

```
VERSION 1.06 OCT.07.92
COPYRIGHT RATT 1990  CAPTIVE A MINDSCAPE PRODUCT  PC CONVERSION BY TAG 1992
```

Save file: `CAPTIVE1.SAV`

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

Items found in the DOS executable at offsets 0x019c90-0x019fe4, with hex prefix bytes:

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

Note: CLIPBOARD appears three times with different prefix bytes — likely different clipboard types. The hex prefix bytes encode item flags, weapon class and stat modifiers. Full stat decoding of the prefix bytes is not yet complete.

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

OpenCaptive identifies 8 MIDI tracks by SHA-256 and plays them through a 32-voice square-wave synthesizer with ADSR envelope.

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

Items are priced based on material grade and weapon type.

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
