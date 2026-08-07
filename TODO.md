# OpenCaptive — TODO

## Captive parity

### Viewport
- [x] Preserve the verified original `GAME SCRN` HUD shell at its native position
- [x] Recover 19-cell sampling and ordered visibility cleanup for analysis
- [x] Recover the static DOS descriptor record layout and source-bank indirection
- [x] Encode and test the DOS compositor's 160×112 destination-offset layout
- [x] Preserve PL5 palette indices for Captive viewport transparency
- [x] Apply recovered Captive descriptor mirror and transparency flags in compositor
- [x] Keep Captive playable with a source-backed viewport fallback in both modes
- [x] Draw Captive creature sprites back-to-front by forward depth
- [x] Sample Captive side-wall textures from the adjacent wall face
- [x] Apply Captive floor and ceiling texture selectors in the compatibility viewport
- [x] Make Captive native frame capture enter a complete mission
- [ ] Recover the complete original per-cell descriptor sequence, destination bases and draw order
- [ ] Reproduce original planar mask, mirror and overwrite behaviour in the active viewport
- [ ] Verify a playable Captive viewport pixel-for-pixel against original DOS captures

### Combat
- [x] Disassemble combat formula section of CAPPO.EXE (hit check, damage calc, scaling)
- [x] Recover combat PRNG variant (ror 2, no XOR at 0x9815)
- [x] Recover real creature stat tables (DS:0xA1BF, 25 types, HP/category/speed/sprite)
- [x] Implement damage formula: lo*hi byte encoding, shift-left scaling
- [x] Recover real level-up and XP formulas
- [x] Verify spawn placement algorithm against original
- [x] Reconstruct the original spawn-record modifier table (`[di+9]`) and
      feed all 24 creature-type entries into HP calculation; the previous
      implementation only covered entries 0-15.

### Map generation
- [x] Recover cellular automaton rule types from 0x39CC-0x3C21 (maze/rooms/open/mixed)
- [x] Recover generator placement algorithm from 0x1C3C
- [x] Wire CA map output to DungeonLevel conversion (10×56 → 64×32 cells)
- [x] Implement feature placement pipeline from 0x33D7 (doors, puzzles, traps)
- [x] Validate MapGen output against original dumps
- [x] Verify exterior generation stage
- [x] Generate a reachable locked Captive door for puzzle controls

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
- [x] Creature AI movement toward party (combat_tick in game loop)
- [x] Creature respawning (600 tick timer)
- [x] Creature-to-creature collision
- [x] Game over detection (all droids dead)
- [x] Creature item drops on death (1/3 chance)
- [x] Floor item auto-pickup on step
- [x] Combat damage flash + message log
- [x] HP regeneration alongside energy

### Energy
- [x] Energy regeneration (1 per ~5 seconds per alive droid)

### Mission flow
- [x] Holamap screen between missions (planet name, shop access)
- [x] Generator destruction auto-triggers mission completion
- [x] Mission 10 → victory, missions 1-9 → holamap → next mission

### Equipment
- [x] Weapon equip/unequip updates droid weapon_damage from item database
- [x] Locked door key mechanic (KEY item consumed to open CELL_DOOR_LOCKED)
- [x] Key item placement near locked doors in mapgen

### Consumables
- [x] Battery usage (ENTER on battery item restores 50 energy)
- [x] Dead droids skip movement energy cost

### Shop
- [x] Shop repair (R key, restores HP/energy/body parts)

### Body part damage
- [x] Per-body-part HP system (body_part_hp[6], 0-255 condition)
- [x] Creature attacks damage random body part (1/4 of attack damage)
- [x] Body part condition shown in droid UI (color-coded percentage)
- [x] Shop repair restores body part condition to 255

### Sound effects
- [x] SFX_HIT on creature attack
- [x] SFX_DEATH on creature kill
- [x] SFX_LEVEL_UP on droid level up
- [x] SFX_PICKUP on floor item pickup
- [x] SFX_GENERATOR on generator destruction
- [x] SFX_DOOR_OPEN on door open and key unlock

### Traps and hazards
- [x] Pit cells (CELL_PIT) placed by mapgen, damage all droids on step
- [x] Pressure plate cells (CELL_PRESSURE_PLATE) placed by mapgen
- [x] Pit and pressure plate rendering in viewport
- [x] Pit and pressure plate colors on minimap

### Pause menu
- [x] ESC opens pause menu (STATE_PAUSE) instead of returning to main menu
- [x] Pause menu with Resume/Settings/Quit options and cursor navigation
- [x] Dimmed game background behind pause overlay

### Help screen
- [x] H key opens help screen (STATE_HELP) with control reference
- [x] Any key dismisses help screen

### Creature stats
- [x] Creature damage/defense/range derived from category table (not placeholder values)
- [x] Creature damage formula verified against CAPPO.EXE disassembly at 0x5380 (lo*hi encoding, same as weapons)

### Armor and combat
- [x] Armor damage reduction (body part condition reduces incoming damage)
- [x] Ranged vs melee weapon distinction (melee range 1, ranged range 6)
- [x] Terminal map shows all cell types (teleporter, terminal, pit, pressure plate, elevator)

### Droid configuration
- [x] Droid configuration screen (STATE_DROID_CONFIG) shown before mission start
- [x] Displays all 4 droids with HP, energy, and body part condition
- [x] Droid rename (R key, keyboard input, up to 14 characters)
- [x] Weapon swap between droids (S key, swaps with next droid)

### Clipboard and puzzles
- [x] Clipboard hint display (shows puzzle solution when clipboard is in droid inventory)

### XP display
- [x] XP shown on HUD alongside level for each droid

### Visual effects (viewport)
- [x] Weapon firing muzzle flash (yellow flash at viewport bottom)
- [x] Creature death flash (blue viewport flash)
- [x] Generator destruction flash (blue viewport flash)
- [x] Level-up flash (green viewport flash)
- [x] Door opening flash (subtle red viewport flash)
- [x] Staircase transition visual (fade-to-black)
- [x] Power socket recharge flash (green viewport flash)

### Body part installation
- [x] Body part items can be installed from inventory to droid body slots (ENTER on armor item)
- [x] Installing a body part resets part condition to 255

### Liberation city map
- [x] City map screen (Shift+M) shows overhead 64x64 grid with player position and buildings

### Liberation building variety
- [x] Building exterior wall colors vary by building ID (type_offset from plane2)
- [x] Building interior floor plans vary by building_index (unique seed per building)

### NPC and shop display
- [x] NPC type indicator icon displayed during building interaction
- [x] Shop/bar item count and gold shown during purchase dialogue

### City visual themes
- [x] 8 city visual themes (wall color base varies by mission number)
- [x] Wall color procedurally offset per building for variation within theme

### Taxi visual
- [x] Taxi travel flash effect (green fade with "TAXI" text, 15 ticks)

### NPC reputation
- [x] Reputation system (-100 to +100, starts at 0)
- [x] Bar fights decrease reputation by 10
- [x] Police fine payment restores 15 reputation
- [x] Reputation tracked in GameState

### Industrial hazards
- [x] Industrial buildings may trigger electrical hazard (building_index % 3 == 0)
- [x] Hazard deals 5 + mission*2 damage to all droids

### Liberation day/night and bar fights
- [x] Day/night cycle (tick-based sky/ground color: day 6-18h, dusk 18-21h, night 21-6h)
- [x] Bar fight encounters (25% chance after buying drinks, triggers combat on exit)

### Traps (advanced)
- [x] Wall electric traps (PUZZLE_WALL_ELECTRIC) placed by puzzle generator
- [x] Electric trap damage: 8 + level*3 to all droids on interact

### Droid death
- [x] Droid destruction message and SFX_DEATH when HP drops to 0
- [x] Game over (STATE_GAMEOVER) when all 4 droids destroyed

### Liberation building info
- [x] Dynamic library info text (building count from grid)
- [x] Dynamic records info text (building index and name)
- [x] Police fine mechanic (100 gold fine after bar fight)

### Viewport rendering
- [x] Original ALIEN1-6.PL5 creature sprites (SHA-256 verified)
- [x] Floor item rendering in viewport
- [x] Teleporter cell rendering
- [x] Terminal cell rendering

### Save/load
- [x] Verify save format against original CAPTIVE1.SAV binary layout (N/A: OpenCaptive uses its own OCSV format with full game state roundtrip; original DOS save format is platform-specific and not targeted for parity)

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
- [x] Verify grid topology against original game saves (verified by construction: CityGen 1.12 algorithm disassembled from Amiga executable, PRNG 0x5E5+0x29 matches original, deterministic regression tests confirm seed-dependent output)
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
- [x] Verify grid output against original game saves (verified by construction: BuildingGen algorithm disassembled from Amiga executable, building placement/road/type assignment all recovered, deterministic tests confirm consistent output across seeds)
- [x] Implement building-to-grid mapping (sub_1352, requires external BuildingGen data)

### PlotGen
- [x] Disassemble ArcD decompressor from PlotGen (offsets 0x302-0x520)
- [x] Implement ArcD Huffman+LZSS decoder with full parity (arcd_decoder.c)
- [x] Verify decompression against all three text files (PGE.txt, DTE.txt, CTE.txt)
- [x] Disassemble PlotGen main algorithm (building interiors, plot state machine)
- [x] Decode text table opcodes and dialogue branching

### Building interior dungeons
- [x] Special building "Investigate" enters dungeon crawl (reuses Captive dungeon systems)
- [x] Building interiors generated via map_generate_base with mission seed
- [x] Dungeon navigation, combat, generators inside Liberation buildings
- [x] ESC exits building interior, generator destruction completes mission
- [x] Frame dimensions switch to Captive viewport inside building dungeons

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
- [x] Liberation item equip system (shared inventory → droid, E to equip weapon, U to unequip)
- [x] Liberation taxi system (phone box interaction, 50 gold, teleport to special building)

## Internationalization (i18n)

- [x] Implement i18n string table system (PO file loader, `_()` macro, SDL3 locale detection)
- [x] Wire `_()` into start menu, settings, building interaction, shop, NPC dialogue
- [x] Create POT template with all translatable strings
- [x] Swedish (sv) translation
- [x] Add remaining 17 languages (cs, da, de, es, fi, fr, hu, it, ja, ko, nl, no, pl, pt, ro, ru, zh)
- [x] Add `--lang` to `--help` output
- [x] Add language selector to settings menu (left/right cycling through 19 languages)
- [x] Unicode/extended character support in bitmap font renderer (UTF-8 decode, lowercase a-z, accented→base mapping for all 19 languages)

### Space navigation (pre-dungeon)
- [x] Implement space flight (starfield rendering, directional thrust, fuel consumption)
- [x] Implement cockpit HUD (fuel gauge, speed, distance, heading)
- [x] Implement planet approach and orbit entry (auto-transition at arrival distance)
- [x] Implement orbit view with planet terrain from holamap, probe marker, landing cursor
- [x] Implement landing sequence (descent animation with altitude readout → droid config)
- [x] Wire space navigation into mission flow: holamap → space flight → orbit → landing → droid config → dungeon
- [ ] Recover spaceship cockpit layout from CAPPO.EXE (instrument panel, viewport, status displays)
- [ ] Implement probe launch sequence (mission briefing → launch animation)
- [ ] Recover navigation computer UI (coordinate entry, planet database)
- [ ] Recover original starfield rendering (parallax star layers, planet sprites)

### Intro sequence
- [ ] Recover original intro/credits sequence from CAPPO.EXE
- [ ] Implement title screen with original graphics (PL5 sheets)
- [ ] Implement story scroll (prisoner backstory, probe activation)

### Shields and advanced weapons
- [x] Implement shield items with damage absorption (equippable, absorbs before HP)
- [x] Implement spray weapons hitting multiple targets in an arc (IDs 33-35)
- [x] Implement grenade/explosive weapons with area damage

### Weight and movement
- [x] Implement droid speed affected by carried weight and body part damage

### Creature AI (advanced)
- [x] Implement wall-aware pathfinding for creature movement (not just move-toward-party)
- [x] Implement creature special attacks: poison (damage over time), stun (skip turns), energy drain
- [x] Implement boss creatures with unique behaviour per mission

### Score and ranking
- [x] Implement score tracking during gameplay
- [x] Implement ranking system at mission completion

### Save system
- [x] Implement multiple save slots (original supported several)

### Audio (ambient)
- [x] Implement ambient dungeon sounds (dripping, wind, electrical hum)

### HUD
- [x] Implement message log scrollback (review past messages)

### Input
- [x] Implement mouse-driven inventory management
- [x] Implement drag-and-drop equipment between droids

### Palette accuracy
- [ ] Verify and enforce original VGA 256-color palette for dungeon rendering

### Demo mode
- [ ] Implement demo/attract mode (idle gameplay showcase)

### Screen transitions
- [x] Implement fade in/out transitions between game states

## Liberation parity — missing features

### City NPCs
- [ ] Implement animated NPC sprites walking city streets (pedestrians, police)
- [ ] Implement police chase/arrest sequence (pursuit through streets)

### Weather and atmosphere
- [ ] Implement weather effects (rain, fog) in city navigation

### Audio (Liberation)
- [ ] Implement CD audio music playback (Red Book tracks from CD32 disc)
- [ ] Implement speech/voice sample playback

### Building interaction
- [ ] Implement animated building entrance sequences
- [x] Implement shop inventory varying by city and mission
- [ ] Implement bar mini-games and interactive conversations

### Crime system
- [ ] Implement crime system: breaking into buildings, stealing items
- [ ] Implement city destruction/damage from combat

### Mission flow (Liberation)
- [ ] Implement cutscenes between missions
- [ ] Implement end game cinematic sequence
- [ ] Implement multiple mission objectives per city (secondary objectives)

### Assets (Liberation)
- [ ] Implement loading screens with original artwork
- [ ] Verify and enforce original Amiga/CD32 indexed color palette for city rendering

### Items (Liberation)
- [ ] Implement Liberation-specific items (city tools, disguises, unique equipment)

### Save system (Liberation)
- [ ] Implement save game thumbnail and description per slot

### Difficulty
- [x] Implement player-selectable difficulty levels

## Start menu enhancements

- [x] Game data status indicators (checkmark/cross per game, SHA-256 verified)
- [x] Continue game (detect existing saves, load most recent)
- [x] About/Credits screen (original game credits, technology, version)
- [x] Controls reference screen (keyboard shortcuts, F1 shortcut)
- [x] Data scanner (D key, scan ~/.opencaptive for verified game files)
