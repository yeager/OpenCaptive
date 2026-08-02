# OpenCaptive — Completed work

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
