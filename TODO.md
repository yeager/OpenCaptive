# OpenCaptive — TODO

## Captive parity

### Viewport
- [x] Viewport renders directly from 5-byte cell bitmasks (no separate descriptor table)
- [ ] Map CA cell bit positions to exact 3D wall segments in viewport renderer
- [ ] Verify panel compositing order against DOS VGA captures
- [x] Add creature sprite rendering (ALIEN1-6.PL5)
- [x] Add item/object rendering in viewport (OBJECTS.PL5)

### Combat
- [x] Disassemble combat formula section of CAPPO.EXE (hit check, damage calc, scaling)
- [x] Recover combat PRNG variant (ror 2, no XOR at 0x9815)
- [x] Recover real creature stat tables (DS:0xA1BF, 25 types, HP/category/speed/sprite)
- [x] Implement damage formula: lo*hi byte encoding, shift-left scaling
- [x] Recover real level-up and XP formulas
- [x] Verify spawn placement algorithm against original

### Map generation
- [x] Recover cellular automaton rule types from 0x39CC-0x3C21 (maze/rooms/open/mixed)
- [x] Recover generator placement algorithm from 0x1C3C
- [x] Wire CA map output to DungeonLevel conversion (10×56 → 64×32 cells)
- [ ] Implement feature placement pipeline from 0x33D7 (doors, puzzles, traps)
- [ ] Validate MapGen output against original dumps
- [ ] Verify exterior generation stage

### Items
- [x] Decode type code prefix bytes in item table (0x00/0x08/0x10/0x20/0x21/0x27/0x30/0x60/0x65)
- [x] Recover item stat computation formulas (procedural from type_code + grade)
- [x] Decode weapon variant/upgrade tier price format (0x1a220+ region)
- [x] Recover item pricing formula
- [x] Recover weapon damage stats from binary (melee 18 entries, ranged 20 entries at 0x1A006)

### Sound
- [x] Extract OPL2 instrument patches from CAP_A.BIN (26 patches)
- [x] Extract SFX sequences from CAP_A.BIN (49 sequences)
- [x] Extract OPL2 frequency table from CAP_A.BIN
- [x] Map all 63 MIDI files to 14 music categories with SHA-256
- [x] Implement variant selection with game PRNG
- [x] Implement OPL2 emulator (YM3812 FM synthesis, 9 channels, envelopes)
- [x] Implement SFX bytecode interpreter (4 simultaneous voices, 70 Hz tick)
- [x] Wire AdLib SFX player into game runtime
- [x] Fully reverse-engineer SFX bytecode interpreter from CAP_A.BIN driver code
- [x] Map SFX sequence indices to game events (disassembled INT 61h call sites)
- [x] Implement AdLib MIDI playback (OPL2 instrument bank for MIDI)

### Save/load
- [ ] Verify save format against original CAPTIVE1.SAV binary layout

### Planet/Holamap
- [x] Implement planet name generation from disassembled algorithm
- [x] Implement holamap display using real coordinate system

## Liberation parity

### CityGen (BuildingGen)
- [x] Disassemble BuildingGen Amiga executable (23,252 bytes code)
- [x] Recover PRNG (state * 0x5E5 + 0x29, same as Captive MapGen)
- [x] Recover grid parameter computation (density, columns, roads, cross-roads)
- [x] Recover building placement and road connection algorithm
- [x] Recover building type assignment (9 types: shop/bar/business/industrial/residence/library/police/records/special)
- [x] Recover city name generation (German syllable pairs + Greek letter suffix)
- [x] Recover building name generation (type-specific name tables)
- [ ] Verify grid topology against original game saves
- [ ] Implement street layout rendering from grid connections

### CityGen grid (64×64)
- [x] Disassemble CityGen 1.12 Amiga executable (10,824 bytes code)
- [x] Recover PRNG (state * 0x5E5 + 0x29, same as BuildingGen/MapGen)
- [x] Recover 8×8 meta-grid road generation (4 corners, direction bitmasks)
- [x] Recover 4×4 tile template expansion (13 templates from 0x2958)
- [x] Recover block placement with road adjacency check (templates A and B)
- [x] Recover difficulty-gated generation phases (0-4+)
- [x] Recover border wall placement and grid plane structure (3 planes)
- [x] Implement building shape resolution (sub_07D2)
- [x] Implement connection table init (sub_1766) and building connectivity (sub_097A)
- [x] Implement building record cleanup (sub_0A08)
- [x] Implement road feature placement — lamp post (0x21), post box (0x22), phone box (0x23)
- [x] Implement advanced feature placement with retry (sub_0A80, difficulty >= 4)
- [x] Implement road-adjacent wall placement (sub_0ECC, difficulty >= 4)
- [x] Implement entry point finder (sub_0180, difficulty >= 4)
- [x] Implement finalize pass — cell type conversion switch (sub_24B8, ~20 cases)
- [ ] Verify grid output against original game saves
- [ ] Implement building-to-grid mapping (sub_1352, requires external BuildingGen data)

### PlotGen
- [x] Disassemble ArcD decompressor from PlotGen (offsets 0x302-0x520)
- [x] Implement ArcD Huffman+LZSS decoder with full parity (arcd_decoder.c)
- [x] Verify decompression against all three text files (PGE.txt, DTE.txt, CTE.txt)
- [ ] Disassemble PlotGen main algorithm (building interiors, plot state machine)
- [ ] Decode text table opcodes and dialogue branching

### 3D rendering
- [x] Decode VGM wall texture format (4 concatenated AmSp banks, 152 sprites per file, 71 wall sets)
- [x] Decode x3g vector format header (IFF FORM O3DG, OFFS, VCDO objects with EXVL vertices + PLST polygons)
- [ ] Decode x3g polygon record format (variable-size records in PLST, vertex indices, texture/color refs)
- [ ] Implement 3D viewport renderer

### Assets
- [ ] Decode Img format (MainSP, 3dView, Taxi, backpack)
- [ ] Decode FNT font format (4 variants)
- [ ] Decode spr format (GameMenu)
- [ ] Extract and verify all 71 VGM wall texture sets from CD32 disc image

### Runtime
- [ ] Implement city navigation
- [ ] Implement shop/bar/business interaction
- [ ] Implement NPC dialogue system
- [ ] Implement save system (Lib-Saves directory)
