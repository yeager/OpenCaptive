# OpenCaptive Release Notes

## v1.1.14 (2026-08-02)

### Liberation: ArcD compression decoder
- Disassembled PlotGen 68k decompressor (offsets 0x302-0x520) with full parity
- Huffman+LZSS format: 3 tables per block, canonical codes, bit-reversed match values
- Verified bit-exact decompression of all 3 text files: PGE.txt (16,304), DTE.txt (14,136), CTE.txt (17,809)

### Liberation: CityGen grid (64x64)
- Disassembled CityGen 1.12 Amiga executable (10,824 bytes code)
- 8x8 meta-grid with PRNG-biased road walking, 13 tile templates for expansion
- 3 grid planes, difficulty-gated generation phases, block placement with road adjacency

### Liberation: BuildingGen (city generation)
- Disassembled BuildingGen Amiga executable (23,252 bytes code)
- 9 building types, road connection graph, city/building name generation
- German syllable pairs + Greek letter suffix for city names

### Captive: combat and creature systems
- Combat damage formula (lo*hi encoding, shift-left scaling)
- 25 creature types with HP/speed/sprite tables from CAPPO.EXE
- Spawn placement algorithm (8 categories, subcell positioning, direction modifiers)
- XP/level-up formulas (per-skill thresholds, growth rates, caps)
- Weapon damage tables (18 melee, 20 ranged entries)

### Captive: MapGen cellular automaton
- 4 CA rule types recovered from CAPPO.EXE (maze/rooms/open/mixed)
- Generator placement algorithm
- DOS PRNG variant at 0x3D54

### Captive: sound system
- AdLib MIDI playback with OPL2 FM synthesis (26 instrument patches)
- Complete SFX bytecode interpreter (13 opcodes, 4 simultaneous voices)
- All 63 MIDI files mapped to 14 music categories
- SFX event mapping from INT 61h call sites

### Captive: items and pricing
- Item database with type code classification from CAPPO.EXE
- Item pricing formula (rol16-based scaling)
- 23 weapon variant tiers with prices
- Item availability gating by difficulty

### Documentation
- Updated README with comprehensive status tables
- Updated all wiki pages with reverse engineering progress
- ArcD format fully documented in File-Formats wiki

## v1.1.13

Initial public release with Captive and Liberation format decoders, verified
original presentation, viewport renderer, sound system, and CI/CD pipeline.
