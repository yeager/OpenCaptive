# OpenCaptive Release Notes

## v1.1.45 (2026-08-03)

### Viewport object rendering
- Floor items visible as cyan blocks in 3D viewport
- Teleporter cells rendered as purple/magenta striped columns
- Terminal cells rendered as green screen rectangles
- All objects scale with perspective distance

## v1.1.44 (2026-08-03)

### Item drops and floor pickup
- Killed creatures have 1/3 chance to drop an item on their tile
- Walking over items auto-picks up to selected droid's inventory
- Pickup message shown in combat log

## v1.1.43 (2026-08-03)

### Combat feedback system
- Red flash on viewport when droids take damage
- Combat message log shows attack events (4 messages, fade over 3 seconds)
- "Droid N fires!" on player attack, "Droid N hit for X!" on creature attack
- Messages rendered over viewport at bottom of 3D view

## v1.1.42 (2026-08-03)

### Liberation inventory screen
- I key opens inventory showing all 4 droids' HP/energy and purchased items
- Gold display, ESC to close
- Droid selection (1-4 keys) during city exploration
- M key toggles map overlay in Liberation

## v1.1.41 (2026-08-03)

### Original ALIEN PL5 creature sprites
- SHA-256 verified loading of ALIEN1-6.PL5 from game data
- Creatures render with original sprite art when available
- Procedural colored silhouettes as fallback without game data
- Front-facing 64×100 region scaled to viewport perspective

## v1.1.40 (2026-08-03)

### Creature viewport rendering
- Creatures now visible in Captive 3D viewport as colored silhouettes
- 6 creature types with distinct colors (green, blue, red, yellow, magenta, cyan)
- Perspective-correct scaling: creatures shrink with distance (range 0-4)
- Red eyes on creature heads for visual feedback

### Liberation mission completion
- Special building investigation completes mission and advances to next city
- New city generated with fresh seed and mission briefing
- 256 missions total before victory

### Creature collision
- Creatures can no longer stack on the same cell during AI movement

## v1.1.39 (2026-08-03)

### Liberation mission completion
- Investigating the special building completes the current mission
- Mission advances to next city with new PlotGen seed and briefing
- 256 missions total, then victory screen
- Creature-to-creature collision prevents stacking during movement

## v1.1.38 (2026-08-03)

### Liberation mission briefing
- PlotGen wired into Liberation session — generates city name, victim, news source
- Mission briefing screen shown before city entry with objective details
- ENTER dismisses briefing and starts city exploration
- City HUD uses PlotGen-generated city name

## v1.1.37 (2026-08-03)

### Creature respawning
- Dead creatures respawn after 600 combat ticks (~100 seconds)
- Respawned creatures reset to full HP, unalerted state

## v1.1.36 (2026-08-03)

### Creature AI movement (live combat)
- Creatures now move toward party during gameplay (combat_tick wired into game loop)
- Creatures attack when in range with line of sight, chase when alerted
- Tick rate: every 10 game ticks (~6 Hz)

### Teleporter and floor trap activation
- Stepping on teleporter traps teleports party to destination
- Floor traps deal damage on step
- puzzle_check_step() called after every party movement

## v1.1.35 (2026-08-03)

### Game over detection
- All droids dead triggers STATE_GAMEOVER in both Captive and Liberation combat

### HP regeneration
- Droids regenerate 1 HP every ~5 seconds alongside energy regen (Captive)

### Shop repair
- R key in shop repairs selected droid (restores HP, energy, body parts)
- Repair cost: damage * 2 (minimum 10 gold)

## v1.1.34 (2026-08-03)

### Rendering fixes
- Window starts at 1280x800, no longer resizes on canvas switch
- Liberation 3D viewport scales to fill screen (256x160 → 320x216)
- Smooth scaling by default (integer_scaling off)
- Window centered on launch

## v1.1.33 (2026-08-03)

### Captive energy regeneration
- Droids regenerate 1 energy every ~5 seconds (300 game ticks)
- Only alive droids regenerate, capped at energy_max

### Captive mission flow (holamap)
- Destroying all generators now transitions to STATE_HOLAMAP
- Holamap screen shows mission number, next planet name, shop access
- ENTER launches next mission, S opens shop, ESC returns to menu
- Mission 10 completion still leads to victory screen
- Generator destruction in combat auto-checks mission completion

## v1.1.32 (2026-08-03)

### Liberation combat system
- Turn-based combat with random street encounters (~1/32 chance per move)
- Enemy generation from PRNG seed with difficulty scaling (HP, damage, defense)
- Droid attack uses weapon_damage (lo*hi encoding), costs 3 energy per attack
- Enemy turn targets random droids with fallback to alive droids
- 8 enemy types (Guard, Soldier, Enforcer, Drone, Sentinel, Trooper, Agent, Warden)
- Combat UI: 1-4 attack with droid, TAB cycle target, ESC flee
- Victory awards 50 XP per kill
- Deterministic encounters from position-based seed

## v1.1.31 (2026-08-03)

### Liberation save/load
- F5 saves city position, facing, droid stats, gold, tick to LSAV format
- F9 loads and restores Liberation game state

### All building types interactive
- Library, police, records, residence, industrial, special each have unique dialogue
- NPC Trade and Ask around options give contextual responses instead of dead-ends

## v1.1.30 (2026-08-03)

### Liberation city navigation
- City grid generates from mission seed with 3D viewport rendering
- WASD/arrow movement with smooth interpolation and wall collision
- Building entrance detection and interaction (shop, bar, dialogue)
- Purchased items stored in 40-slot Liberation inventory
- City name and position HUD overlay

### Captive equipment and energy parity
- Equipping weapons updates droid weapon_damage from item database (lo*hi encoding)
- Energy consumption: 3 per attack, 1 per movement step per droid

### CI cross-platform fixes
- Windows MSVC compatibility (math library, M_PI, unistd.h)
- SDL3_ttf vendored submodules for Windows/Linux builds
- Cross-platform CMake with find_package/pkg-config fallback

## v1.1.29 (2026-08-03)

### High-resolution UTF-8 menu
- Menu renders at 960x600 with SDL3_ttf (DejaVu Sans Mono Bold, OFL licensed)
- Full UTF-8 text rendering: proper ÅÄÖ, Ü, é, č, 日本語, 한국어, Русский, 中文
- All 19 PO translation files updated with real Unicode characters
- captivelogo.png logo displayed at top of start menu
- stb_image.h for PNG image loading (logo + card art)
- Renderer buffer overflow fix for large canvas sizes

## v1.1.28 (2026-08-03)

### Language selector and 19-language support
- Language selector in settings menu with left/right cycling
- 18 new PO translation files (cs, da, de, es, fi, fr, hu, it, ja, ko, nl, no, pl, pt, ro, ru, sv, zh)
- Romanized ASCII for CJK/Cyrillic languages (bitmap font constraint)

### Card-based start menu
- Redesigned start menu with procedural game art cards
- Captive: dungeon corridor with flickering torches (Amiga purple palette)
- Liberation: cityscape with gradient sky, procedural buildings, blinking windows
- 2x2 grid navigation (arrow keys + mouse click)

## v1.1.26 (2026-08-03)

### Internationalization (i18n) system
- Added PO-based translation system with `_()` macro and SDL3 locale detection
- `--lang` CLI flag for manual language override
- Swedish (sv) translation — first non-English language
- All UI strings in start menu, settings, building interaction, shop, and NPC dialogue are now translatable

### Wiki documentation
- Updated all 10 wiki pages with current project state
- PlotGen, x3g, VGM, FNT, and spr formats fully documented

## v1.1.25 (2026-08-02)

### Captive: CA cell-to-wall segment mapping + full feature pipeline
- Map 5-byte CA cell bits to 5 viewport wall segments per cell
- `ca_segments` and `ca_thickness` fields in MapCell preserve original wall data
- Viewport renders partial walls using per-segment column drawing
- Wall thickness from CA rules: 1px (0x10), 2px (0x18), 3px (0x80/0xC0)
- Full puzzle/trap feature pipeline:
  - Bars puzzles (number-matching, level 3+)
  - Button combos (8 blue buttons, level 4+)
  - Hidden buttons in grates (level 2+)
  - Floor damage traps (level 3+)
  - Teleporter traps (level 5+)
  - Interaction handlers for all new types

## v1.1.24 (2026-08-02)

### Liberation: Textured 3D viewport + missing modules
- Perspective-correct UV-mapped texture rendering for wall quads
- `lib3d_render_textured_quad()` with per-pixel z-buffer and transparency
- `city_nav_render_textured()` for VGM wall texture support in city view
- Added missing dialogue and shop modules that broke CI

## v1.1.23 (2026-08-02)

### Liberation: Save/load system
- LSAV binary format with big-endian serialization
- Full game state roundtrip: seeds, difficulty, mission, gold, tick, position, facing
- Up to 4 droids with name, HP, energy, level, 8 skills, 8 equipment slots
- 256-mission completion bitmap
- Generator progress tracking (destroyed/total)
- `lib_save_from_state()` convenience builder from live game state
- Magic/version validation on load, rejects bad files

## v1.1.22 (2026-08-02)

### Liberation: City navigation system
- Grid-based movement on CityGen 64×64 layout with wall collision
- Forward/backward movement, 90° turns, 180° turn-around
- Smooth position interpolation for animated movement between cells
- 3D wall rendering from grid data: visible faces only, per-cell color variation
- Building entrance detection (cell type 0x0A)
- x3g object placement at building entrances
- 8-cell view range with frustum culling via viewport renderer

## v1.1.21 (2026-08-02)

### Liberation: 3D viewport renderer
- Software 3D renderer with perspective projection and z-buffer
- Triangle fan rasterizer for convex x3g polygons (3–8 vertices)
- Normal-based flat shading with directional light
- Painter's algorithm z-sorting + per-pixel z-buffer for correct overlap
- Camera transform with yaw rotation, near-plane culling
- 256×160 viewport with sky/ground split, blit to destination framebuffer
- Tested: triangle rendering, z-sorting, camera rotation, behind-camera culling

## v1.1.20 (2026-08-02)

### Liberation: FNT font decoder + wiki updates
- `CHAR` container: 114 proportional-width glyphs, 7 rows, 2 bitplanes (foreground + drop-shadow)
- 2 font variants: 0Liberation (large, up to 9px) and 1Liberation (small, up to 6px)
- gamemenu.spr confirmed as standard AmSp bank
- Updated all wiki pages with decoded format documentation

## v1.1.19 (2026-08-02)

### Liberation: Img sprite format decoded
- `ImgA` container: uint16 sprite count + uint32 offset table
- Simple sprites: width, height, 1–6 color bitplanes (planar Amiga layout) + mask plane
- Multi-frame sprites: frame count + offset table, each frame a sub-sprite (LOD variants)
- Verified all 361 sprites across 4 files: MainSp (158), backpack (176), taxi (4), 3dView (23 multi×6 frames)
- FNT font format identified: `CHAR` magic, 114 glyphs × 16 bytes, 8px wide 2-plane bitmap
- gamemenu.spr confirmed as standard AmSp bank (already decoded)

## v1.1.18 (2026-08-02)

### Custom features system
All features behind flags, parity mode stays pristine. Enable individually or `--all-features`.

- **HD upscaling**: xBRZ 2x/3x/4x edge-aware pixel art upscaler (`--hd-upscale`, `--upscale-factor`)
- **Widescreen viewport**: extended viewport width beyond original bounds (`--widescreen`)
- **Multi-slot quicksave**: 10 save slots, F5 save, F6 cycle slot, F9 load (`--quicksave`)
- **Minimap overlay**: real-time minimap with configurable size/opacity, F8 toggle (`--minimap`)
- **Mouse-look**: FPS-style mouse turning with configurable sensitivity (`--mouse-look`)
- **Debug HUD**: cell type, position, direction, droid stats, PRNG seed, F7 toggle (`--debug-hud`)
- **Speed control**: adjustable game speed 0.25x–4x, numpad +/- (`--speed`)
- **Fast travel**: skip traversal in cities (`--fast-travel`)
- **Automap**: permanently reveals visited cells on minimap (`--automap`)
- **Dynamic lighting**: distance and normal-based per-pixel shading (`--dynamic-lighting`)
- **Audio reverb**: 4-tap delay reverb on dungeon audio (`--reverb`)
- **Replay system**: record and playback input sequences (`--replay-record`, `--replay-play`)
- **Cross-save**: portable binary save format for cross-platform transfer (`--cross-save-export`)
- **Texture filtering**: bilinear filtering on wall textures (`--bilinear`)
- **Config persistence**: save/load feature settings from file (`--features-config`)

## v1.1.17 (2026-08-02)

### Liberation: x3g polygon record format decoded
- 36-byte fixed header: type, record size, normals, render flags, color/texture, UV rect
- Variable vertex refs as EXVL byte offsets (÷16 = vertex index), with closing ref
- Record sizes: 40 (point/sprite), 44 (triangle), 46 (quad)
- Full X3gPolygon struct with parsed fields (normals, flags, color, UV, vertex indices)
- Verified against all 3 x3g test files (Objects, people, 0CityVectors)

## v1.1.16 (2026-08-02)

### Liberation: VGM wall texture decoder
- Decoded VGM format: 4 concatenated AmSp (AMOS Sprite Bank) banks per file
- 152 total sprites per VGM file (42+45+24+41 across banks), 4bpp with mask
- 71 wall texture sets (Wall01–Wall71.VGM, each 167,766 bytes)
- Delegates per-sprite decoding to existing AmSp decoder

### Liberation: x3g 3D vector format parser
- Decoded IFF FORM O3DG container: OFFS header (object count + offsets), VCDO sub-forms
- EXVL vertex lists: 16 bytes per vertex (x, y, z, group, 4 reserved int16s)
- PLST polygon list data preserved as raw buffer (record format TBD)
- Tested against Objects.x3g (3 objects), people.x3g (4), 0CityVectors.x3g (33)

## v1.1.15 (2026-08-02)

### Liberation: CityGen grid — remaining subroutines
- Building shape resolution (sub_07D2): walks building records, resolves origin via directional traversal
- Connection table init (sub_1766): 51-byte table, seed-derived coordinates
- Building connectivity (sub_097A): PRNG-based direction assignment for unconnected buildings
- Building record cleanup (sub_0A08): mask AND 0x0FFF/0xBF/0x2F on 4-byte records
- Road feature placement: 3 variants — lamp post (0x21, ×10), post box (0x22, ×4), phone box (0x23, ×1)
- Road feature inner loop (sub_160E): directional walk from PRNG position, target cell matching, adjacent-cell placement
- Advanced feature placement (sub_0A80, difficulty ≥ 4): retry limit 0xFF, up to 51 iterations with plane0 backup/restore
- Road-adjacent wall placement (sub_0ECC, difficulty ≥ 4): walk from random position to cell 0x1F, search for connectable cells
- Entry point finder (sub_0180, difficulty ≥ 4): 30 attempts to find road cell via feature placement, walk to building origin
- Finalize pass (sub_24B8): iterates all 4096 cells, ~20-case type dispatch converting generation cells to output values, sets entry point to 0x0A

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
