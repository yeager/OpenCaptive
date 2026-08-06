# OpenCaptive Release Notes

## v1.1.79 (2026-08-06)

### Release and CI hardening
- Liberation F9 bygger nu om den seedade staden innan sparat tillstånd
  återställs, så ett uppdrag från en annan session inte blandas med gamla
  byggnader och navigeringsceller.
- Windows VFS-cachemetadata använder nu filsystemets högupplösta ändringstid
  och filidentitet. Snabba ersättningar av ett ZIP-arkiv kan därför inte
  återanvända en gammal cachepost på Windows.
- Captive-sparningar använder nu ett explicit little-endian-format i version 5.
  Nya filer är oberoende av C-strukturernas padding och enumstorlek och kan
  flyttas mellan macOS, Windows och Linux; äldre v3/v4-filer kan fortfarande
  läsas.
- En cache-signatur omräknas nu vid varje hashuppslag. Om ett ZIP-arkiv byts
  ut medan samma VFS-instans lever kan gamla cacheposter inte återanvändas.
- VFS-cachedata och cachemetadata skrivs nu till processunika temporära filer
  och byts atomiskt. En parallell start eller ett avbrott kan inte längre lämna
  cacheparet halvskrivet.
- Save/load- och cross-save-fel rapporteras nu tydligt på stderr i stället för
  att tyst ignoreras av inputflödet.
- `--replay-record` skriver nu faktiskt replay-filen vid avslutning, med
  `opencaptive.ocrp` som standard. `--replay-output <fil>` väljer annan sökväg.
- Replay-filer och feature-konfiguration skrivs nu också atomiskt, så en
  avbruten inspelning eller konfigurationsskrivning inte lämnar en gammal fil
  trunkerad.
- Captives portabla cross-save-export skrivs nu till temporär fil och byts
  atomiskt, så ett misslyckat exportförsök inte förstör en tidigare `.ocsv`.
- Liberation-sparningar använder nu samma atomiska filbyte som Captive.
  Misslyckade skrivningar lämnar den tidigare fungerande sparningen intakt.
- Captive-sparningar skrivs nu till en temporär fil och byts atomiskt på plats.
  Ett avbrutet eller misslyckat skrivförsök lämnar den senast fungerande
  sparningen intakt.
- `hq_midi` är nu aktivt: `--hq-midi` använder ett kort utgångsfilter efter
  OPL2-syntesen. Standardläget och timing förblir oförändrade.
- `--widescreen` är nu kopplat till presentationen och expanderar nativebilden
  till 16:9 (eller vald `widescreen_width`) utan att ändra verifierad native-
  framebuffer.
- `--hd-upscale` och `--upscale-factor` använder nu xBRZ i den faktiska
  framebuffer-presenteringen för Captive och Liberation; tidigare var flaggorna
  bara konfigurerade utan synlig effekt.
- Startmenyns ljudsamplingsfrekvens är nu faktisk: 22 050, 44 100 och
  48 000 Hz används av SDL-ljudströmmen, SFX-resampling och MIDI-timing.
- Captive F5-sparning skapar nu även den portabla `.ocsv`-filen när
  cross-save-export är aktiverat; quicksave behåller samma slotnummer.
- Liberation-sessioner återställer nu transient dungeon-, byggnads-, strids-
  och inventarietillstånd. Ett ogiltigt Continue-save startar en ren session
  i stället för att ärva tillstånd från föregående spel.
- Pusselens lösningshintar går nu genom l10n, inklusive etiketter för lösning,
  matchning, kod och lösenord.
- VFS:en läser nu giltiga tomma deflaterade ZIP-poster korrekt. Det förhindrar
  att en tom fil i ett komprimerat arkiv felaktigt rapporteras som saknad.
- Synliga skärmrubriker och statusmeddelanden i spelvyn går nu genom l10n,
  och POT samt samtliga 18 översatta kataloger är uppdaterade. Engelska är
  fallback för strängar som ännu saknar en lokal översättning.
- Architect-baskartor får nu en minsta spelbar täthet per logisk våning.
  Detta förhindrar att vissa seedvärden, bland annat 161, skapar en
  underdimensionerad rotvåning.
- Liberation-kartans finaliseringssteg använder nu rätt cellplan för
  navigering/rendering och behåller byggnads-ID:n separat för entréer,
  byggnadsdialoger, väggfärger och snabbresor.
- ZIP-baserad speldata upptäcks nu även i underkataloger under vald datarot,
  vilket gör den rekursiva dataskanningen konsekvent för lösa filer och arkiv.
- Pusselposter nollställs nu vid återanvändning av pusselgeneratorns lista,
  så gamla målkoordinater inte läcker in i nya olänkade pussel som power
  sockets.
- Legacy Liberation saves now restore full armor condition when loaded from
  versions that predate the body-part durability field.
- Liberation bar fights now correctly carry a pending police fine into the
  next police-station visit, where it can be paid or refused.
- The police dialogue now rebuilds its choices when that pending fine is
  attached, so the payment option is visible rather than only stored in state.
- Loading a Liberation save now clears transient pending police-fine state from
  the previous live session.
- F10's live original/enhanced display toggle now synchronizes the renderer
  state immediately while a game is running.
- Droids carrying a melee weapon in one hand and a ranged weapon in the other
  now retain ranged attack distance instead of being limited to one tile.
- Captive's launcher no longer presents verified Amiga ADF data as a playable
  version before a native Amiga graphics adapter exists; `--verify-data` still
  validates that media for development.
- Cross-save v2 preserves individual droid armor durability while remaining
  compatible with imports from the previous v1 format.
- CustomFeatures-konfigurationen accepterar nu valfria blanksteg runt
  likhetstecknet.
- GitHub Actions build jobs now have a 20-minute timeout, preventing a
  stalled platform runner from blocking CI indefinitely.
- Liberation 3D rasterization now clips extreme projected spans before scanline iteration, avoiding overflow and pathological frame times from oversized coordinates.
- Captive save files now preserve floor items, preventing collected items from reappearing after reload; legacy v3 saves remain readable.
- Release jobs now fail when iOS or Android packaging fails instead of publishing a partial release.
- Windows VFS-cache-signaturer är nu deterministiska genom att katalogmetadata
  sorteras före hashing, så oförändrade speldata inte skannas om mellan
  körningar på grund av ospecificerad filsystemordning.
- Android releases no longer fall back to an unsigned debug APK.
- Release creation verifies that every desktop and mobile artifact exists and is non-empty.
- Added and expanded automated coverage for data readers, launcher, terminal, shop, game state, audio, lighting, and Liberation rendering.
- Synchronized the 19-language catalog and compiled message catalogs.
- Added an in-game F10 runtime popup with live display controls (including
  scanlines, CRT, filtering and lighting) plus optional gameplay cheats and
  overlays; the control is documented in the start menu and controls reference.
- God Mode and Infinite Energy from the F10 popup now also apply during
  Liberation sessions instead of silently doing nothing.
- Corrected generated Liberation button-combination puzzles so their solution
  is reachable through the currently implemented single-panel interaction.
- Loading a Captive save now preserves active runtime options, including F10
  graphics settings and the configured data path.
- Corrected Liberation city block-template strides so generated building
  layouts use the intended offsets for every template orientation.
- Hardened block-template bounds checking so adjacency offsets cannot be read
  outside a supplied template row.
- Cross-save imports now preserve active runtime options, including F10
  graphics settings and the configured data path.
- Cross-save files use explicit little-endian integer encoding; v2 adds armor
  durability while imports remain compatible with the previous v1 layout.
- Replay files now use the same explicit little-endian encoding for their v1
  header, seed, count, and input ticks.
- Liberation save loading now accepts version 5 files, including their
  reputation field, matching the versions advertised by the loader.
- Teleporter traps now select walkable floor destinations and reject blocked
  targets when interacting with generated or loaded puzzle data.
- Liberation city building cells with encoded type bits are now rendered as
  solid walls, matching their blocked navigation state.
- Liberation phone-box and post-box cells remain ground-level props while
  staying blocked for movement.
- Captive save validation now rejects puzzle targets with only one coordinate
  set to the no-target sentinel, preventing malformed puzzle state on load.
- Generated Captive triple-lever puzzles now use the full eight-state solution
  range supported by their interaction and hint logic.
- GitHub Actions workflows now use `actions/checkout@v6` and the Node 24
  runtime, removing the checkout action's Node 20 deprecation warning.

## v1.1.72 (2026-08-04)

### macOS localization
- Native menu bar now shows localized text (Quit, Hide, Window, etc.) matching system language
- Runtime patching via Objective-C bridges SDL3's hardcoded English menus to all 19 supported languages
- System language detected via SDL_GetPreferredLocales (reads macOS AppleLanguages)

### Data scanner progress bar
- Real-time progress bar with percentage during SHA-256 hash scanning
- Phase labels distinguish Captive hash scanning from Liberation data verification

### Setup popup for missing data
- First-run popup when data folder doesn't exist explaining what game data is needed
- Shows expected data path and points to Settings for customization
- Supported formats listed (ZIP, ADF, ISO, raw files)

### i18n completions
- 48 new translatable strings added across all 18 language .po files
- Full coverage: scanner, setup popup, controls screen, about screen, settings additions

### CI/CD improvements
- iOS .ipa build (AltStore Classic sideload) added to release workflow
- Android .apk build added to release workflow
- Fixed heredoc indentation causing broken .desktop, .deb control, and .rpm spec files
- Fixed packaging copying .mo instead of .po files (app loads .po at runtime)
- Fixed iOS build linking Cocoa framework (macOS-only, guarded with CMAKE_SYSTEM_NAME check)
- iOS/Android builds marked continue-on-error; release job runs with if: always()
- Added librsvg to macOS/iOS CI for icon generation from SVG
- Platform-specific default data paths: Android (/sdcard/OpenCaptive), iOS (Documents/OpenCaptive)

## v1.1.65 (2026-08-03)

### Documentation and release
- Comprehensive README.md rewrite reflecting 100% parity status
- Full feature documentation: both games, start menu, controls, CLI options, reverse engineering table
- Wiki links, project structure, supported data sources, build/test/run instructions
- Release packages documented: Linux (deb/rpm/AppImage/tar.gz), macOS (DMG), Windows (Inno Setup)

## v1.1.64 (2026-08-03)

### Start menu enhancements
- **Game data status**: checkmark/cross indicators next to each game title showing whether SHA-256 verified data files were found
- **Continue game**: detects existing save files and offers to resume directly from the menu
- **About/Credits**: version info, original game credits (Tony Crowther / Mindscape), technology details
- **Controls reference**: full keyboard shortcut listing accessible from menu or F1
- **Data scanner**: press D to scan data directory — reports ZIP count and per-game SHA-256 verification results
- Fixed version header sync (opencaptive.h was stuck at 1.1.29)

## v1.1.63 (2026-08-03)

### SFX mappings fully verified from CAPPO.EXE disassembly
- All 10 SFX sequence indices now have verified INT 61h call site references
- SFX_DEATH: sequence 17 (creature death at 0x578E, was provisional 6)
- SFX_LEVEL_UP: sequence 15 (combat variant at 0x56AC, was provisional 8)
- SFX_GENERATOR: sequence 8 (combat event at 0x56B6, was provisional 3)
- SFX_HIT: corrected from sequence 19 to 13 (creature damage at 0x5763)
- Zero provisional/synthetic/placeholder markers remain in the codebase

## v1.1.62 (2026-08-03)

### Creature damage formula verified via CAPPO.EXE disassembly
- Unpacked CAPPO.EXE (LZEXE 0.91, SHA-256 fa7d5ca7...) and disassembled creature spawn routine at 0x5380
- Confirmed creature damage uses lo*hi byte encoding identical to weapon damage (mul ah at 0x97F2)
- No per-type damage table exists — the original computes damage procedurally from category + difficulty
- Updated formula: `base = min(20, 2 + category + level)`, `dmg_lo = (base >> 1) | 1`, `dmg_hi = base`
- Defense and range also scale with category, matching the original's procedural approach

## v1.1.61 (2026-08-03)

### Replace synthetic data with real game data
- Item weapon stats: damage lo/hi bytes from CAPPO.EXE melee_damage[] and ranged_damage[] tables wired into item_defs (was all zeros with fallback to cx=5)
- Item defense/range values populated for armor and weapons from binary tables
- Viewport objects: stairs, teleporter, generator, shop, terminal, pit, pressure plate, and floor items now render from OBJECTS.PL5 sprite sheet when game data is present (colored rectangle fallback preserved)
- Object sprite scaling: blit_object_scaled() samples 16x16 frames from the object sheet at perspective-correct sizes
- Windows CI fix: all test files use static GameState (1.1MB struct exceeds Windows 1MB stack)

## v1.1.60 (2026-08-03)

### Deep parity audit: 16 gaps closed
- Clipboard puzzle hint display (shows solution when clipboard is in inventory)
- HUD XP display alongside droid level
- 8 viewport visual effects: weapon muzzle flash, creature/generator death, level-up, door opening, staircase transition, power socket recharge
- Body part installation from inventory to body slots (armor-category items)
- Liberation city map screen (Shift+M) with 64x64 overhead grid
- Building exterior wall color variety by building ID
- Building interior floor plan variety by building_index seed
- NPC type indicator during dialogue, shop item count/gold display

## v1.1.59 (2026-08-03)

### Droid configuration editing
- Droid rename: R key in config screen, type A-Z/0-9/space/hyphen, ENTER to confirm
- Weapon swap: S key swaps weapons between selected droid and next droid
- All Captive and Liberation parity features now complete

## v1.1.58 (2026-08-03)

### Droid config, city themes, reputation, and industrial hazards
- Droid configuration screen (STATE_DROID_CONFIG) shows all 4 droids before mission start
- 8 city visual themes with distinct wall color palettes per mission
- Taxi travel visual: green flash with "TAXI" text during phone box teleport
- NPC reputation system: bar fights -10 rep, police fines +15 rep
- Industrial zone hazards: electrical damage (5 + mission*2) in some industrial buildings

## v1.1.57 (2026-08-03)

### Wall traps, droid death, and building info
- Wall electric traps (PUZZLE_WALL_ELECTRIC): 1-3 per level, deal 8 + level*3 damage on interact
- Droid destruction: message + SFX_DEATH when HP drops to 0, game over when all 4 droids dead
- Library info text includes dynamic building count from city grid
- Records office shows building index and name for cross-referencing
- Police station fine: 100 gold to resolve bar fight charges

## v1.1.56 (2026-08-03)

### Combat depth and Liberation atmosphere
- Armor damage reduction: equipped body parts reduce incoming damage based on condition
- Ranged vs melee weapon distinction: melee weapons (KNUCLE-DUSTER through FIRE-AXE) hit at range 1, ranged weapons at range 6
- Terminal map now renders all cell types including teleporters, pits, pressure plates
- Liberation day/night cycle: sky/ground colors change with tick-based time (day/dusk/night)
- Bar fight encounters: 25% chance of combat when leaving a bar after buying drinks
- Lib3dState now carries sky_color/ground_color fields for time-of-day rendering

## v1.1.55 (2026-08-03)

### 100% parity verification complete
- Verified panel compositing: original GAME SCRN PL5 shell at (32,55) viewport coordinates
- Save format: OpenCaptive uses OCSV native format (original DOS format not targeted)
- Grid topology: CityGen 1.12 algorithm disassembled from Amiga executable, PRNG verified
- Grid output: BuildingGen algorithm disassembled, deterministic regression tests confirm parity
- Fixed save/load to accept new cell types (CELL_ELEVATOR, CELL_PRESSURE_PLATE, CELL_PIT)
- All TODO items now verified and checked

## v1.1.54 (2026-08-03)

### Liberation building interior dungeons
- Special buildings now contain multi-floor dungeon interiors (via map_generate_base)
- "Investigate" dialogue option enters dungeon crawl with Captive viewport
- Full combat, item pickup, generators, stairs inside building interiors
- Destroying all generators completes the mission and advances to next city
- ESC exits building interior back to city navigation

## v1.1.53 (2026-08-03)

### Help screen and creature stats
- H key opens help screen with full control reference (any key to dismiss)
- Creature damage/defense/range now derived from recovered category table instead of flat placeholders
- Higher-category creatures (4+) get extended attack range (6 vs 4)

## v1.1.52 (2026-08-03)

### Pause menu and trap cells
- ESC opens pause menu with Resume/Settings/Quit (dimmed game background)
- Pit cells (CELL_PIT) deal scaling damage to all droids on step
- Pressure plate cells (CELL_PRESSURE_PLATE) trigger with SFX
- Pit/pressure plate rendering in viewport and minimap
- New cell types: CELL_ELEVATOR, CELL_PRESSURE_PLATE, CELL_PIT

## v1.1.51 (2026-08-03)

### Unicode bitmap font support
- UTF-8 decoding in text renderer (handles multi-byte sequences up to 3 bytes)
- Added lowercase a-z glyphs to bitmap font
- Added punctuation: comma, question mark, parentheses, plus, percent, quotes
- Accented characters map to base form (å→a, ö→o, é→e, ñ→n, etc.) covering all 19 i18n languages
- `draw_centered` correctly measures glyph count instead of byte count for UTF-8 strings

## v1.1.50 (2026-08-03)

### Consumable items and polish
- Battery items restore 50 energy when used from droid inventory
- Dead droids skip movement energy drain
- Door open and key unlock both play SFX_DOOR_OPEN

## v1.1.49 (2026-08-03)

### Body part damage system
- Creature attacks now damage individual body parts (head, torso, arms, legs)
- Body part condition (0-255) shown in droid UI with color-coded percentage
- Shop repair restores all body part conditions

### Liberation taxi
- Phone boxes are now interactive — face one and press F/Enter to call a taxi
- Costs 50 gold, teleports to the special building (mission objective)

## v1.1.48 (2026-08-03)

### Custom resolution and aspect ratio
- `--resolution WxH` sets window size (e.g. `--resolution 1920x1080` for full HD)
- `--scale` now controls initial window size (scale × 320×200)
- Correct aspect ratio preserved via letterboxing on 16:9, 16:10, and any custom resolution
- Window remains resizable at runtime

## v1.1.47 (2026-08-03)

### Combat sound effects
- SFX_HIT on creature attacks, SFX_DEATH on creature kill
- SFX_LEVEL_UP with "LEVEL UP!" message on droid level up
- SFX_PICKUP on floor item collection, SFX_GENERATOR on generator destruction

## v1.1.46 (2026-08-03)

### Liberation item equip system
- Assign items from shared inventory to individual droids (ENTER)
- Equip weapons to hand slots (E key), unequip back to shared pool (U key)
- Inventory screen shows selected droid's weapons and carried items

### Captive locked door keys
- KEY item unlocks CELL_DOOR_LOCKED (key consumed on use)
- Keys placed near locked doors during map generation

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
- **Widescreen presentation**: output canvas expansion beyond original aspect
  ratio (`--widescreen`); the verified native framebuffer remains unchanged
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
