# OpenCaptive — Completed work

## 2026-08-02 (AdLib MIDI playback)

### OPL2-based MIDI player
- Rewrote MIDI player from sine/square synthesis to real OPL2 FM synthesis
- Uses Captive's 26 instrument patches from CAP_A.BIN for all MIDI programs
- 9-voice OPL2 polyphony with oldest-voice stealing
- MIDI program change maps to Captive patch index
- Tempo tracking from MIDI meta events, tick-accurate rendering
- Standard MIDI format 0 and 1 parsing (64 .MID files in game data)
- Loop mode for background music
- Test suite: load, parse, render, stop, loop

## 2026-08-02 (XP and level-up formulas)

### XP/level system from CAPPO.EXE disassembly
- XP award formula (0x9621): `skill * (creature_xp + 3 + difficulty) * 12`
- Per-skill base XP table at DS:0xB708 (file 0x19EF8): 10 entries
- XP threshold function (0xAB92): compound growth per level
  - ROBOTICS (skill 0): +12.5% + 8 per level, caps at level 66
  - Other skills: +6.25% (rounded) per level, caps at level 24
  - Max threshold: 0x8A47 (35,399)
- Display level = XP >> 10 (from display code at 0xB142)
- XP overflow protection (0x87EC): cap at 0xF8FFFFFF, minimum +1
- 32-bit XP accumulator replaces old 16-bit field in Droid struct
- Test suite: threshold growth, caps, award formula, display, overflow

## 2026-08-02 (SFX bytecode interpreter — full 13 opcodes)

### Complete SFX bytecode interpreter from CAP_A.BIN disassembly
- Implemented all 13 opcodes (was 5): 0x80-0x8A, 0xC8, 0xFF
- New opcodes: 0x81 (delay), 0x82 (volume), 0x83 (note offset), 0x85 (delay variant),
  0x86 (call subroutine), 0x87 (return), 0x88 (PRNG note), 0x89 (PRNG delay), 0x8A (jump)
- Voice struct extended: note_offset, key_playing, current_note, loop table, subroutine support, PRNG state
- Fixed opcode semantics: 0x80 is key-on (not delay), 0x82 is volume (not pitch), 0x83 is note offset (not key-on/off)

## 2026-08-02 (weapon damage tables)

### Weapon damage encoding from CAPPO.EXE
- Melee: 18 entries at file 0x1A006 (lo,hi pairs → lo×hi base damage)
- Ranged: 20 entries at file 0x1A02A (base, modifier, flag, 0 records)
- Grade progression: hi bytes 33,42,51,60,69 (increment 9)
- Ranged tiers: base 72-120 (×16), modifier 45→5 (÷10 per tier)

## 2026-08-02 (creature stat tables)

### Creature stat tables from CAPPO.EXE disassembly
- HP table at DS:0xA1BF (file 0x189AF): 25 creature types with min/max HP
- Category table at DS:0x9A42 (file 0x18232): 8 categories, 3 types each
- Speed table at DS:0xA1A4 (file 0x18994): movement speed per type
- Sprite map at DS:0xA16E (file 0x1895E): graphic_id + frame_index
- HP formula: base interpolation by difficulty, modifier scaling, cap at 255
- DS segment base discovery: 0x0E3F → file offset 0xE7F0 + DS_offset
- Test suite: table integrity, HP formula, bounds, sprite map, categories

## 2026-08-02 (object sprites)

### Object sprite rendering (OBJECTS.PL5)
- 16×16 frame grid (20 cols × 12 rows = 240 frames per sheet)
- Same transparency and scaling as creature sprites
- Test suite: load, blit, null safety

## 2026-08-02 (creature sprites)

### Creature sprite rendering
- PL5 sprite sheet frame extractor (32×40 grid, 10×5 = 50 frames)
- Scaled blitting with transparency (palette index 0 = transparent)
- Frame validity detection (empty frames marked invalid)
- Test suite: synthetic load, blit, scaled blit, null safety

## 2026-08-02 (holamap)

### Holamap display implementation
- 256×128 planet surface map with 5 terrain types (water to mountain)
- Base placement using mission PRNG with terrain constraints
- Reveal mechanic for PLANET PROBE item usage
- Crosshair cursor and base markers (red=active, grey=destroyed)
- STATE_HOLAMAP game mode added
- Test suite: init, determinism, reveal, render, surface variety

## 2026-08-02 (damage formula)

### Combat damage formula implementation
- Implemented lo*hi byte damage encoding from CAPPO.EXE at 0x97F4
- Shift-left scaling (×2/×4/×8) based on droid level, capped at 0xFFFD
- Added weapon_damage field to Droid struct (lo*hi encoded uint16_t)

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
