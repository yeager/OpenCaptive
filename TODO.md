# OpenCaptive — TODO

## Captive parity

### Viewport
- [ ] Recover DOS descriptor table from 1MB memory dump (SHA-256 9003c4...)
- [ ] Replace approximate viewport projection with exact panel coordinates
- [ ] Verify panel compositing order against DOS VGA captures
- [ ] Add creature sprite rendering (ALIEN1-6.PL5)
- [ ] Add item/object rendering in viewport (OBJECTS.PL5)

### Combat
- [ ] Disassemble combat formula section of CAPPO.EXE
- [ ] Recover real creature stat tables and damage formulas
- [ ] Recover real level-up and XP formulas
- [ ] Verify spawn placement algorithm against original

### Map generation
- [ ] Implement all 30 documented Architect stages
- [ ] Validate MapGen output against original dumps
- [ ] Recover feature placement (generators, doors, puzzles, traps)
- [ ] Verify exterior generation stage

### Items
- [ ] Decode hex prefix bytes in item table (flags, class, stat modifiers)
- [ ] Recover item pricing formula
- [ ] Recover weapon damage/range/ammo stats from binary

### Sound
- [ ] Obtain CTV/VOC files and verify SHA-256 hashes
- [ ] Map CTV entries to game sound events
- [ ] Implement AdLib OPL2 emulation for FM synthesis effects (alternative)
- [ ] Music track category-to-game-event mapping

### Save/load
- [ ] Verify save format against original CAPTIVE1.SAV binary layout

### Planet/Holamap
- [ ] Implement planet name generation from disassembled algorithm
- [ ] Implement holamap display using real coordinate system

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
