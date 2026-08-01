# Captive Game Data

Data extracted from the verified DOS executable CAPPO.EXE (v1.06, Oct 7 1992). The executable is LZEXE v0.91 compressed (73,056 bytes packed, 144,556 bytes unpacked).

## Version information

```
CAPTIVE V1.06 OCT.07.92
(C) 1990 MINDSCAPE INTERNATIONAL
PROGRAMMED BY TONY CROWTHER
```

## Droid materials

Droids are built from parts with one of 10 material grades, in ascending quality:

| # | Material |
| --- | --- |
| 1 | SHIT |
| 2 | TIN |
| 3 | BRASS |
| 4 | IRON |
| 5 | STEEL |
| 6 | CHROME |
| 7 | SILVER |
| 8 | GOLD |
| 9 | PLATINUM |
| 10 | TITANIUX |

Note: the original spells it "CROMIZE" in some contexts and "TITANIUX" (not Titanium).

## Combat skills

Each droid has 10 trainable combat skills:

| # | Skill |
| --- | --- |
| 1 | BRAWLING |
| 2 | CLOSE COMBAT |
| 3 | PROJECTILE |
| 4 | FIREARMS |
| 5 | LIGHT ARMS |
| 6 | HEAVY ARMS |
| 7 | AUTO ARMS |
| 8 | HEAVY AUTO |
| 9 | MECH. WEAPON |
| 10 | ENERGY WEAPON |

Skills range 1-8 points per tier. Experience threshold: `level * 100` XP.

## Droid devices (body parts)

12 device categories for droid construction:

```
HEAD, TORSO, LEFT ARM, RIGHT ARM, LEFT HAND, RIGHT HAND,
LEFT LEG, RIGHT LEG, LEFT FOOT, RIGHT FOOT, BACKPACK, POWER UNIT
```

6 body parts for armor slots: Head, Torso, Arms, Hands, Legs, Feet.

## Items and weapons

### Weapon categories

7 weapon classes corresponding to the combat skills:

1. Brawling weapons
2. Close combat weapons
3. Projectile weapons
4. Firearms
5. Light arms
6. Heavy arms / Auto arms
7. Mechanical / Energy weapons

### Known items from executable

Items found in the DOS executable string table, with their hex prefix bytes (inventory/shop classification):

```
08 03 SMALL ENERGY CELL       08 03 MEDIUM ENERGY CELL
08 03 LARGE ENERGY CELL       0a 01 SMALL CLIP
0a 01 MEDIUM CLIP             0a 01 LARGE CLIP
06 0c LIGHT SABRE             06 08 RAPIER
06 08 EPEE                    06 08 CUTLASS
06 07 AXE                     06 07 BATTLE AXE
06 07 HALBERD                 06 07 HAMMER
06 07 MACE                    06 09 ARROW
06 09 DART                    06 09 THROWING KNIFE
06 09 THROWING STAR           06 09 GRENADE
06 0a PISTOL                  06 0a LIGHT PISTOL
06 0a HEAVY PISTOL            06 0a TWIN PISTOL
06 0b RIFLE                   06 0b ASSAULT RIFLE
06 0b SNIPER RIFLE            06 0b LASER RIFLE
06 0b PLASMA GUN              06 0d CANNON
06 0d TWIN CANNON             06 0d LIGHT SPRAYGUN
06 0d HEAVY SPRAYGUN          06 0d FLAME THROWER
06 0d ROCKET LAUNCHER         04 02 SMALL MEDIPACK
04 02 LARGE MEDIPACK          04 02 SMALL SHIELD
04 02 LARGE SHIELD            04 02 OPTICAL DEVICE
04 02 HOLO MAP
```

### Quality tiers

Each weapon comes in 8 quality grades (derived from the material system).

### Ammo capacities

- Small/Medium/Large Energy Cells
- Small/Medium/Large Clips
- Grenade stacks

## Music system

The original supports 4 sound drivers:

| Driver | Hardware |
| --- | --- |
| ADLIB | AdLib/OPL2 FM synthesis |
| ROLAND | Roland MT-32/LAPC-1 |
| BEEPER | PC internal speaker |
| SBLASTER | Sound Blaster DAC |

### Track categories

14 music categories with 11 variations each (154 total track slots):

The categories map to game situations (exploration, combat intensity levels, puzzle, victory, danger, ambient). Specific mapping is under analysis.

OpenCaptive currently identifies 8 MIDI tracks by SHA-256 and plays them through a 32-voice square-wave synthesizer with ADSR envelope.

## Name generation

The original generates droid names from a syllable table. 48 syllables are combined to create names:

```
BA  BE  BI  BO  BU  BY
DA  DE  DI  DO  DU  DY
KA  KE  KI  KO  KU  KY
LA  LE  LI  LO  LU  LY
MA  ME  MI  MO  MU  MY
RA  RE  RI  RO  RU  RY
SA  SE  SI  SO  SU  SY
TA  TE  TI  TO  TU  TY
```

Names consist of 2-3 concatenated syllables (e.g., BADO, KIRAMU, SYTELI).

## Planet name generation

Planet names use a similar syllable-based system with a different table. The naming is seeded by the mission number.

## Holamap system

The HOLO MAP item displays a top-down view of the current dungeon level. It shows:
- Walls and corridors
- Current party position
- Explored/unexplored areas

## Shop system

Shops appear in space stations between missions. Shop UI strings from the executable:

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

Save state includes: party droids (parts, skills, equipment), current mission/level, map state, gold, objective counters.

## Graphics files

### PL5 images (DOS)

All PL5 files are 40,000 bytes (320x200, 5bpp):
- Intro/title screens
- Wall textures (multiple sheets)
- Object/item sprites
- HUD panel graphics
- Door and decoration panels

### ANM animations (DOS)

16 animation files for:
- Title sequence
- Intro cutscene
- Game-over sequence
- Mission briefing
- Level transitions
- Victory sequence

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
