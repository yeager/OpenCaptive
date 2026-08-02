# OpenCaptive — TODO

## Captive parity

### Viewport
- [ ] Recover DOS descriptor table from 1MB memory dump (SHA-256 9003c4...)
- [ ] Replace approximate viewport projection with exact panel coordinates
- [ ] Verify panel compositing order against DOS VGA captures
- [x] Add creature sprite rendering (ALIEN1-6.PL5)
- [x] Add item/object rendering in viewport (OBJECTS.PL5)

### Combat
- [x] Disassemble combat formula section of CAPPO.EXE (hit check, damage calc, scaling)
- [x] Recover combat PRNG variant (ror 2, no XOR at 0x9815)
- [ ] Recover real creature stat tables (lookup at [BX+0x9C90])
- [x] Implement damage formula: lo*hi byte encoding, shift-left scaling
- [ ] Recover real level-up and XP formulas
- [ ] Verify spawn placement algorithm against original

### Map generation
- [ ] Implement all 30 documented Architect stages
- [ ] Validate MapGen output against original dumps
- [ ] Recover feature placement (generators, doors, puzzles, traps)
- [ ] Verify exterior generation stage

### Items
- [x] Decode type code prefix bytes in item table (0x00/0x08/0x10/0x20/0x21/0x27/0x30/0x60/0x65)
- [ ] Recover item stat computation formulas (procedural from type_code + grade)
- [x] Decode weapon variant/upgrade tier price format (0x1a220+ region)
- [ ] Recover item pricing formula
- [ ] Recover weapon damage/range/ammo stats from binary

### Sound
- [x] Extract OPL2 instrument patches from CAP_A.BIN (26 patches)
- [x] Extract SFX sequences from CAP_A.BIN (49 sequences)
- [x] Extract OPL2 frequency table from CAP_A.BIN
- [x] Map all 63 MIDI files to 14 music categories with SHA-256
- [x] Implement variant selection with game PRNG
- [x] Implement OPL2 emulator (YM3812 FM synthesis, 9 channels, envelopes)
- [x] Implement SFX bytecode interpreter (4 simultaneous voices, 70 Hz tick)
- [x] Wire AdLib SFX player into game runtime
- [ ] Fully reverse-engineer SFX bytecode interpreter from CAP_A.BIN driver code
- [x] Map SFX sequence indices to game events (disassembled INT 61h call sites)
- [ ] Implement AdLib MIDI playback (OPL2 instrument bank for MIDI)

### Save/load
- [ ] Verify save format against original CAPTIVE1.SAV binary layout

### Planet/Holamap
- [x] Implement planet name generation from disassembled algorithm
- [x] Implement holamap display using real coordinate system

## Liberation parity

### CityGen
- [ ] Disassemble CityGen 1.12 Amiga executable (10,896 bytes)
- [ ] Recover 64x64 grid generation algorithm
- [ ] Recover city name generation from syllable table
- [ ] Implement street layout and building placement

### PlotGen
- [ ] Disassemble PlotGen executable (12,388 bytes code)
- [ ] Recover building interior format
- [ ] Recover plot progression state machine
- [ ] Decode text table opcodes and dialogue branching

### 3D rendering
- [ ] Decode x3g vector format (city, room, bank, bar, shop, police, droids, monsters, people, objects)
- [ ] Decode VGM wall texture format
- [ ] Implement 3D viewport renderer

### Assets
- [ ] Decode Img format (MainSP, 3dView, Taxi, backpack)
- [ ] Decode FNT font format (4 variants)
- [ ] Decode spr format (GameMenu)

### Runtime
- [ ] Implement city navigation
- [ ] Implement shop/bar/business interaction
- [ ] Implement NPC dialogue system
- [ ] Implement save system (Lib-Saves directory)
