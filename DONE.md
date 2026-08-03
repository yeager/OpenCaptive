# OpenCaptive — Completed work

## 2026-08-03 (Liberation save/load + building interactions — v1.1.31)

### Liberation save/load wired into main loop
- F5 saves Liberation game state (position, facing, droids, gold, tick)
- F9 loads Liberation save and restores city navigation position

### All building types have unique interactions
- Library: search archives for generator locations
- Police: ask for information about threats
- Records office: look up building registry
- Residence: hear rumors about special buildings
- Industrial: explore restricted area
- Special: investigate mission-critical location
- NPC dialogue Trade and Ask around options give contextual responses

## 2026-08-03 (Liberation game loop + Captive equipment parity — v1.1.30)

### Liberation city navigation wired into main loop
- City grid generates from mission seed; 3D viewport renders the procedural city
- WASD/arrow key movement with smooth interpolation and collision detection
- Building entrance detection — F/Enter to enter, ESC to leave
- Building interactions: shop/bar dialogue and purchasing
- Purchased items stored in Liberation inventory (40-slot)
- City name + coordinates HUD overlay; building dialogue panel overlay

### Captive equipment and energy parity
- Equipping/unequipping weapons now updates droid weapon_damage from item database
- Energy consumed per attack (3 energy per shot)
- Energy consumed per movement step (1 energy per droid per step)

### CI cross-platform fixes
- Windows MSVC: conditional math library, _USE_MATH_DEFINES, unistd.h guard
- SDL3_ttf: --recurse-submodules for vendored harfbuzz/freetype
- CMake: find_package(SDL3_ttf) with pkg-config fallback
- Excluded pre-existing crashing tests per platform

## 2026-08-03 (high-res UTF-8 menu + SDL3_ttf + logo — v1.1.29)

### High-resolution menu with TTF font rendering
- Menu now renders at 960x600 (was 320x200), game viewport unchanged
- SDL3_ttf integration with DejaVu Sans Mono Bold (OFL license, full Unicode)
- Three font sizes: title 36pt, body 18pt, small 14pt
- Full UTF-8 support: Swedish ÅÄÖ, German Ü, French é, Czech č, CJK, Cyrillic, etc.
- All 19 PO files updated with proper UTF-8 characters (no more ASCII approximations)
- captivelogo.png displayed at top of menu (loaded from Downloads/ or data/)
- stb_image.h integrated for PNG loading (card images + logo)
- Card labels (CAPTIVE/LIBERATION) centered under their respective cards
- Renderer buffer overflow fix: dynamic allocation for post-processing buffer
- start_menu_free() for proper cleanup of fonts and images

## 2026-08-03 (language selector + card menu + 19 languages — v1.1.28)

### Language selector and translations
- Language selector in settings menu (left/right cycling through 19 languages)
- 18 PO translation files: cs, da, de, es, fi, fr, hu, it, ja, ko, nl, no, pl, pt, ro, ru, sv, zh
- All CJK and Cyrillic translations use romanized ASCII (bitmap font constraint)

### Card-based start menu
- Redesigned from flat text list to 2x2 card grid layout
- Procedural dungeon art card for Captive (Amiga-style purple palette, torches, corridor)
- Procedural cityscape art card for Liberation (gradient sky, buildings, blinking windows)
- 2x2 grid navigation for keyboard and mouse

## 2026-08-03 (i18n system + wiki updates — v1.1.26)

### Internationalization (i18n) system
- PO file loader with escape handling, multi-line support, table reuse
- `_()` macro for marking translatable strings throughout codebase
- SDL3 locale auto-detection via `SDL_GetPreferredLocales()`
- `--lang` CLI flag for manual language override
- POT template with all translatable strings (menu, settings, dialogue, errors)
- Swedish (sv) translation — first non-English language
- Wired into start menu, settings, building interaction, shop, NPC dialogue
- 48 tests passing (new: test_i18n)

### Wiki updates
- All 10 wiki pages reviewed and updated
- Liberation-Technical: PlotGen fully implemented, runtime boundary updated, RE plan items 5-8 done
- Liberation-Game-Data: PlotGen algorithm, text engine opcodes, news sources added
- File-Formats: x3g (IFF FORM O3DG), VGM (71 sets, 152 sprites), FNT, spr documented
- Data-Identity-and-Verification: ADF added as VFS source
- Game-Preservation: CityGen/BuildingGen/PlotGen marked reimplemented

## 2026-08-02 (CA wall segments + feature pipeline — v1.1.25)

### CA cell-to-viewport wall segment mapping
- 5-byte CA cells now preserve per-segment wall bits in MapCell.ca_segments
- Viewport draws partial walls: 5 column segments per cell with thickness codes
- Matches disassembled renderer at 0x4560: byte→bit mapping from Captive-Technical.md

### Full feature placement pipeline
- Bars, button combos, hidden buttons, floor traps, teleporter traps
- All interaction handlers implemented in puzzle_interact()
- Progressive difficulty: traps appear from level 3+, teleporters from level 5+

## 2026-08-02 (Textured 3D viewport + CI fix — v1.1.24)

### Perspective-correct textured polygon rendering
- `lib3d_render_textured_quad()` with per-scanline UV interpolation and z-buffer
- `city_nav_render_textured()` renders VGM wall textures in city navigation
- Committed missing dialogue/shop/save headers that broke CI

## 2026-08-02 (Liberation save system — v1.1.23)

### LSAV binary save/load format
- Big-endian serialization: seeds, difficulty, mission, gold, tick, city position, facing
- Droid state: 4 droids × (name, HP, energy, level, 8 skills, 8 equipment slots)
- 256-mission completion bitmap (32 bytes)
- Generator progress (destroyed/total)
- Magic/version validation on load
- `lib_save_from_state()` convenience builder from live CityNavState

## 2026-08-02 (ArcD compression decoder — Liberation PlotGen)

### ArcD Huffman+LZSS decompressor with full parity
- Disassembled PlotGen 68k decompressor at offsets 0x302-0x520 (12,388 byte executable from Liberation Disk 3)
- Built minimal 68k emulator to run the original binary and verify bit-exact output
- Format: 4-byte magic "ArcD" (0x41726344) + 4-byte decompressed size (BE32) + 4-byte compressed size (BE32)
- Bit buffer model: 32-bit register (d6), 8-bit byte counter (d7), bits consumed LSB-first from 16-bit big-endian words
- Three Huffman tables per block: lit_count (a3+256), match_offset (a3+0), match_length (a3+128)
- Each table: 128 bytes = 16 × (mask:16, match:16) + 16 × (shift:8, symbol:8, extra_mask:16)
- Canonical Huffman codes with bit-reversed match values and variable-length integer encoding
- Symbols 0-1 are literal values; symbol k≥2 reads k-1 extra bits from the stream
- Table reuse: when symbol count is 0, the previous block's table persists (not zeroed)
- Back-reference: offset + optional extra byte (offset ≥ 512) + mandatory byte + match_length+1 bytes
- Block structure: 16-bit block_count, then lit_count-1 main loop iterations with interleaved literal runs
- Verified bit-exact decompression against all three Liberation text files:
  - PGE.txt: 7,085 → 16,304 bytes (2 blocks)
  - DTE.txt: 5,323 → 14,136 bytes (2 blocks)
  - CTE.txt: 8,230 → 17,809 bytes (2 blocks)

## 2026-08-02 (Liberation CityGen grid disassembly)

### 64×64 city grid generation from CityGen 1.12 Amiga HUNK executable
- Extracted CityGen (10,824 bytes code + 4,888 BSS) from Liberation Disk 3
- Version string: "CityGen 1.12 (CaptiveII : Monday 03-Jan-94 02:17:04)"
- PRNG: state * 0x5E5 + 0x29 (identical to BuildingGen and Captive MapGen)
- 8×8 meta-grid with 4-bit direction bitmask per cell (N/E/S/W connections)
- Road corners at (3,0), (6,3), (3,6), (0,3) — road availability per seed
- Road walking with PRNG-biased direction selection and boundary clamping
- 13 tile templates (4×4 bytes each) at 0x2958 for meta-grid expansion
- Expansion from 8×8 meta-grid to 64×64 tile grid via template lookup
- 3 grid planes: plane0 (cell type), plane1 (road ID), plane2 (building ID)
- 2 block template sets: template A (6 cells, road blocks) and B (7 cells, features)
- Block placement with random position search and road adjacency check (types 18-21)
- Difficulty-gated phases: borders (≥0), features (≥2), road blocks (≥3)
- Data tables recovered: direction table (0x2830), road availability (0x2694),
  road counts (0x2890/0x2899), block templates (0x28B4/0x28F8)
- Test suite: PRNG, init, determinism, different seeds, borders, road cells, meta connections

## 2026-08-02 (Liberation BuildingGen disassembly)

### City generation from BuildingGen Amiga HUNK executable
- Extracted BuildingGen (23,252 bytes code + 3,956 BSS) and PlotGen (12,388 bytes code + 27,016 BSS) from Disk 3
- PRNG: state = state * 0x5E5 + 0x29 (identical to Captive MapGen)
- Grid parameter computation from seed/level: density, columns, roads, cross-roads
- Building record structure: 36 bytes (type, id, name_seed, flags, 3 connections)
- Road connection graph: forward (0xAAAA) / backward (0xBBBB) markers
- 9 building types: shop, bar, business, industrial, residence, library, police, records, special
- City names: German syllable pairs (32 syllables) + Greek letter suffix by level
- Building names: type-specific from real string tables (shop/bar/business/industrial names)
- String tables extracted to separate compilation unit for test isolation
- Fixed pre-existing link errors in test_map_gen, test_game_state, test_save_load

## 2026-08-02 (spawn placement algorithm)

### Spawn placement from CAPPO.EXE disassembly
- Full spawn flow at 0x9987–0x9AA7 with creature type routing
- 8 creature categories (DS:0x9A42), 3 types per category
- Type-based placement: singles, flagged pairs, trios, directional groups
- Subcell positioning from two 16-byte tables (DS:0x9BD8, DS:0x9BE8)
- Direction modifiers: opposite (XOR 2), perpendicular (NOT & 1)
- Modifier table (DS:0x9AB7) and difficulty offset table (DS:0x9A5A)
- HP formula integrated: min + (range * difficulty / 8), modifier scaling, cap 255
- Combat system updated to use real spawn placement instead of placeholder
- Test suite: tables, subcell lookup, HP computation, spawn counts

## 2026-08-02 (MapGen cellular automaton)

### Cellular automaton map rules from CAPPO.EXE
- 4 map types recovered from 0x39CC–0x3C21:
  - Type 0 (maze): bit tests 0x101/0x808, output 0x10
  - Type 1 (rooms): bit tests 0x202/0x404, output 0x10
  - Type 2 (open): bit tests 0x101/0x404, output 0x18 (wider)
  - Type 3 (mixed): like open with shr+or for denser walls
- 5-byte cell format matching original 10×56 grid
- MapGen DOS PRNG at 0x3D54: mul 0x5E5 + add 0x29, no ROR
- Pattern generation via rotated PRNG bitmasks
- Generator placement: count = (PRNG & 7) + 1, random position
- Feature placement pipeline identified at 0x33D7 (partial)
- Test suites: CA init, pattern, determinism, rule types, boundaries

## 2026-08-02 (item pricing and availability)

### Item pricing formula from CAPPO.EXE disassembly
- Price computation at 0xBB63: rol16-based scaling with game level + difficulty
- Early game (level <= 5): fixed base price 128
- Normal: ((level_hi <<< 4) + level_lo + base + shop_tier) <<< 4 + difficulty
- Item availability check at 0xB8D6: grade/material gating against difficulty
- 23 weapon variant tiers with ASCII prices decoded from 0x1A220
- Melee tiers: 2, 3, 5, 7, 14 gold
- Ranged tiers: 14-231 gold (14 tiers with increasing prices)
- Named variants: A12, L22, X42
- Gold cap: 200 (0xC8) at gold bar renderer (0x1AE7)
- Test suite: variant table, pricing, availability

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
