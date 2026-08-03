# OpenCaptive — TODO

## Captive parity

### Viewport
- [x] Viewport renders directly from 5-byte cell bitmasks (no separate descriptor table)
- [x] Map CA cell bit positions to exact 3D wall segments in viewport renderer
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
- [x] Implement feature placement pipeline from 0x33D7 (doors, puzzles, traps)
- [x] Validate MapGen output against original dumps
- [x] Verify exterior generation stage

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

### Combat
- [x] Energy consumption per attack (3 energy per shot)
- [x] Energy consumption per movement step (1 energy per droid per step)

### Energy
- [x] Energy regeneration (1 per ~5 seconds per alive droid)

### Mission flow
- [x] Holamap screen between missions (planet name, shop access)
- [x] Generator destruction auto-triggers mission completion
- [x] Mission 10 → victory, missions 1-9 → holamap → next mission

### Equipment
- [x] Weapon equip/unequip updates droid weapon_damage from item database

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
- [x] Implement street layout rendering from grid connections

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
- [x] Implement building-to-grid mapping (sub_1352, requires external BuildingGen data)

### PlotGen
- [x] Disassemble ArcD decompressor from PlotGen (offsets 0x302-0x520)
- [x] Implement ArcD Huffman+LZSS decoder with full parity (arcd_decoder.c)
- [x] Verify decompression against all three text files (PGE.txt, DTE.txt, CTE.txt)
- [x] Disassemble PlotGen main algorithm (building interiors, plot state machine)
- [x] Decode text table opcodes and dialogue branching

### 3D rendering
- [x] Decode VGM wall texture format (4 concatenated AmSp banks, 152 sprites per file, 71 wall sets)
- [x] Decode x3g vector format header (IFF FORM O3DG, OFFS, VCDO objects with EXVL vertices + PLST polygons)
- [x] Decode x3g polygon record format (38-byte fixed header + variable vertex refs as byte offsets/16, closing ref)
- [x] Implement 3D viewport renderer (perspective projection, z-buffer, flat-shaded polygon rasterizer)
- [x] Add textured polygon rendering (VGM wall textures mapped via UV coords)
- [x] Wire viewport into city navigation loop (city_nav_render_textured)

### Assets
- [x] Decode Img format (MainSP 158, 3dView 23×6 multi-frame, Taxi 4, backpack 176)
- [x] Decode FNT font format (2 variants: CHAR magic, 114 glyphs, 2-plane proportional)
- [x] Decode spr format (GameMenu = standard AmSp bank)
- [x] Extract and verify all 71 VGM wall texture sets from CD32 disc image

### Runtime
- [x] Implement city navigation (grid-based movement, collision, smooth interpolation, wall rendering)
- [x] Wire city navigation into main game loop with 3D viewport rendering
- [x] Wire building entrance detection and interaction into main game loop
- [x] Wire Liberation input handling (WASD/arrows for city movement, F/Enter for building entry)
- [x] Implement shop/bar/business interaction
- [x] Implement NPC dialogue system
- [x] Implement save system (LSAV binary format, roundtrip, mission bitmap)
- [x] Shop purchase stores items in Liberation inventory
- [x] Wire Liberation save/load into main loop (F5 save, F9 load)
- [x] Building type interactions: library, police, records, residence, industrial, special
- [x] NPC dialogue Trade and Ask around options give useful responses
- [x] Liberation combat system (turn-based, random encounters, energy cost, XP rewards)
- [x] PlotGen wired into Liberation session (mission briefing, city name, victim objective)

## Internationalization (i18n)

- [x] Implement i18n string table system (PO file loader, `_()` macro, SDL3 locale detection)
- [x] Wire `_()` into start menu, settings, building interaction, shop, NPC dialogue
- [x] Create POT template with all translatable strings
- [x] Swedish (sv) translation
- [x] Add remaining 17 languages (cs, da, de, es, fi, fr, hu, it, ja, ko, nl, no, pl, pt, ro, ru, zh)
- [x] Add `--lang` to `--help` output
- [x] Add language selector to settings menu (left/right cycling through 19 languages)
- [ ] Unicode/extended character support in bitmap font renderer
