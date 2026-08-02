# OpenCaptive — Completed work

## 2026-08-02 (viewport & palette)

### Viewport and HUD verification
- Verified viewport 144×112 at (32,55) from VGA blit at 0x485F
- Recovered 9 HUD panel blit rectangles with exact screen coordinates
- Verified VGA palette at file offset 0xEFCE matches existing pl5_default_palette
- Rendered ALIEN1.PL5 sprite sheet with real palette (creature frames confirmed)

## 2026-08-02 (combat & PRNG)

### PRNG correction from disassembly
- Fixed main PRNG: ror 3 + xor 0x0800 (was incorrectly ror 4)
- Changed to 16-bit arithmetic matching original x86 MUL/ADD
- Documented all 4 PRNG variants in the executable

### Combat system disassembly
- Recovered hit check function at 0x97D9 (creature target list lookup)
- Recovered damage formula: lo_byte * hi_byte encoding at [di+6]
- Recovered damage scaling: shift-left up to ×8 with overflow sentinel
- Combat loop: 4 droid slots, struct size 0x10E, base 0x8DC7

### SFX event mapping from disassembly
- Disassembled INT 61h AH=0x40 call sites for real SFX indices
- Mapped 7 static game events to driver sequences
- Identified variable weapon-type and action-type SFX dispatch

## 2026-08-02 (item database)

### Item type codes from DOS executable
- Decoded type_code prefix bytes for all items from unpacked CAPPO.EXE (0x1a090)
- Record format: 00 type_code [grade] NAME 0x20
- Nine type codes: 0x00=misc, 0x08=consumable, 0x10=ammo, 0x20=chip,
  0x21=equipment, 0x27=body part, 0x30=ranged, 0x60=explosive, 0x65=body variant
- Fixed item categories (ARM, GOLD, BATTERY, EXPLOSIVES, automatics, utilities)
- Discovered weapon variant/upgrade tier format at 0x1a220 with ASCII prices
- Documented item record format in wiki

## 2026-08-02 (continued)

### AdLib sound effect data recovery
- Extracted 26 OPL2 instrument patches from CAP_A.BIN (AdLib driver)
- Extracted 49 SFX bytecode sequences (2,132 bytes total)
- Extracted 128-entry OPL2 frequency number table
- Created adlib_data.c/h with full patch and sequence data
- Test suite verifying patch count, SFX termination, frequency table

### Music system expansion
- Added all 63 MIDI file SHA-256 hashes (was 8, now 63)
- Added 6 new music categories: FCBASE, VCBASE, LONGNT, W, COMPROOM, RUNNING
- Implemented PRNG-based variant selection for categories with 11 variants
- Music system now matches original game's random track selection behavior

### Planet name generation
- Implemented captive_generate_planet_name() from DOS disassembly
- Uses consonant/vowel character tables to generate "STATION XXXX" names

## 2026-08-02

### Captive game data recovery
- Corrected all game data tables to match DOS disassembly exactly
- Real syllable table (48 syllables: VI, RUP, YUL, SCO, etc.)
- Real material names (SHIT, HUMAN, TINDRON, COPPATOR, BRONZITE, IRONIDE, CROMIZE)
- Real skill names (ROBOTICS, BRAWLING, SWORDS, etc.)
- Real device names (AG-SCAN, ROOT-FINDER, MAPPER, etc.)
- Real body part names (HEAD, CHEST, ARM, LEG, FOOT, HAND)
- Real item database (KNUCLE-DUSTER, BATTLE-GLOVE, WAR-BLADE, COLT, UZIE, etc.)
- 25 game messages, 13 shop dialogue strings from executable
- 14 music track categories with names
- 23 graphics filenames

### Viewport renderer
- Created viewport.c with 19-cell trapezoid compositing
- 5 depth ranges with perspective-correct cell sizing
- Wall, floor/ceiling, door, ornament and special cell rendering
- Wired into main.c game loop (replaces black viewport clear)
- Uses real PL5 panel sheets exclusively

### Sound system
- Created CTV/VOC decoder (Creative Voice File format)
- Wired sfx system to load CTV sound effects
- CTV decoder test suite

### PRNG parity
- All subsystems now use original PRNG (mul 0x5e5, add 0x29, ror 4, xor 0x800)
- Combat, shop, puzzle systems all converted from synthetic LCG

### Combat system
- Renamed creatures to ALIEN1-6 matching PL5 sprite sets
- Uses original PRNG for all randomness

### Liberation string tables
- Added all CD32 executable string data to code
- 32 German city syllables, 20 street types
- 35 first + 32 last names, 8 NPC titles
- 9 shop + 12 bar + 11 business + 12 industrial types

### Documentation
- Wiki updated: Captive-Technical, Captive-Game-Data, File-Formats
- CTV format documented in File-Formats wiki page
- All wiki pages synced to GitHub wiki
