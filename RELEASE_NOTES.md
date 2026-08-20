# OpenCaptive Release Notes

## v1.1.177 (2026-08-20)

### Added

- **(Captive)** `captive_gm_run(mission)` — one call runs the whole native GM.EXE
  generator and leaves the finished 64x32 output map at `w->b[CAPTIVE_GM_OUTPUT_MAP]`
  (0x5A68).  `captive_gm_build_level(DungeonLevel*, mission)` bridges that output through
  `captive_dos_map_to_level` into the engine's level struct — the real, byte-exact game
  generator now feeds the renderer directly (no captured map, no DOSBox).  Covered by
  `test_build_level_from_generator` (wall/floor counts + determinism for missions 1/2/3).

## v1.1.176 (2026-08-20)

### Added

- **(Captive)** End-to-end verification of the GM.EXE native port: `test_full_pipeline_output`
  runs the complete pass chain followed by the 0xEE translate driver
  (`captive_gm_generate_output`) and asserts the resulting output level map
  (work[0x5A68..0x6288]) and second map (work[0x6288..0x6A00]) are byte-identical to the
  real GM.EXE's post-generate state (oracle 0x128) for missions 1/2/3.  **The native C
  dungeon generator now reproduces the game's playable level byte-for-byte, from mission
  seed to finished map, with no original code executed.**

## v1.1.175 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: the final decoration group (orchestrator
  0x43A..0x452) — `captive_gm_pass_2595` (second call, 0xFFC4->0xFFF6),
  `captive_gm_pass_157e` (type-0x2C markers via the 0x1513/0x1548 neighbour
  classifier), `captive_gm_pass_13e3` (stairs pairing 0x2E/0x2F), `captive_gm_pass_237f`
  (type-0x32 items), `captive_gm_pass_23b4` (type-0x30/0x31 items), and
  `captive_gm_pass_1460` (mission-0-only fixed objective + spawn inserts).  **This
  completes the entire GM.EXE generation pass chain (0x3B1..0x45E).**  The full chain is
  now verified byte-identical to the real GM.EXE across the whole work segment for
  missions 1/2/3 (`test_pass_group_1460`).

### Fixed

- **(Captive)** Two control-flow readings in the final group: the `jmp 0x683` targets
  inside helper 0x1548 are a shared `ret` (return from the helper, not the pass), so
  those neighbour types reject rather than abort; and pass 0x1460 runs its body only
  when the mission param is 0 (it is a no-op for the standard missions).

## v1.1.174 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: the post-A2A decoration/spawn group (orchestrator
  0x417..0x43A) — ten passes that scatter items, floor decorations, chests and wall
  creatures across the finished layout:
  - `captive_gm_pass_2595` — selector-map word replace (clears the 0xFFC3 nest markers).
  - `captive_gm_pass_9c3` — type-0x16 items at junctions (0x2675 centre-valid gate).
  - `captive_gm_pass_967` — type-0x1E markers (0x1BF5 mask scan).
  - `captive_gm_pass_f61` — wall creatures/items via the 0xFE2 spawn engine.
  - `captive_gm_pass_2284` / `0x2400` — floor items (types 0x24/0x2A and 0x25/0x2B).
  - `captive_gm_pass_22ba` / `0x22eb` / `0x2310` — chest placement (0x2346 -> 0x122C).
  - `captive_gm_pass_242e` — type-0x26 items.
  Verified byte-identical to the real GM.EXE across the whole work segment for missions
  1/2/3 (`test_pass_group_242e`).

### Fixed

- **(Captive)** GM 0x2675 gate in pass 0x9C3: the pass branches on 0x2675's ZF (set iff
  the centre cell's selector is VALID), not on the neighbour count it returns.  A valid
  cell with zero non-valid neighbours (count 0) is now correctly accepted instead of
  skipped, fixing one misplaced item on mission 3.

## v1.1.173 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: pass 0xA2A (`captive_gm_pass_a2a`), the item /
  creature-nest distribution pass.  A first loop places 25 plain type-0x15 items from
  the record list at word[0x3586]; a second loop runs 300 RNG-seeded probe walks that
  classify each cell (0xAB4/0x1BF5), skip cells adjacent to an existing item (0xAD6),
  and place an item (0xAEE -> 0xB00) whose sub-mode (item / counted item / creature
  nest) is chosen from the low 3 bits of a baked code table (0x6AF4).  The creature-nest
  case (0xB73) walks a trail, writes the nest anchor + a 0x2C marker, records into the
  0x3582/0x3594 buffers, and spawns creatures through the 0xFCB/0xFE2 engine.  Verified
  byte-identical to the real GM.EXE across the whole work segment for missions 1/2/3
  (`test_pass_a2a`).

### Fixed

- **(Captive)** GM 0xAB4 classifier: the caller branches on the CPU flags 0xAB4 leaves
  (the SF from its internal `cmp mask,0x11` / `cmp type,7`), not on the returned AX, so
  a cell whose adjacency mask is below 0x11 now correctly breaks the probe walk instead
  of stepping.  Only surfaced with pass 0xA2A.

## v1.1.172 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: pass 0x2ABC (`captive_gm_pass_2abc`), which places
  up to 0x20 guard creatures at 2-then-1 junctions off dead ends — placing a chest via
  0x168D, inserting into the spawn list (0x233D/0x1226 with the discard flag), and
  spawning through the 0xFE2 engine.  Verified byte-identical to the real GM.EXE (maps +
  entity buffers) for missions 1/2/3 (`test_pass_2abc`).

## v1.1.171 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: pass 0x2A9D (`captive_gm_pass_2A9D`), which places up
  to 0x10 chests (type 0x22 / selector 0xFFC3) at random dead ends by walking a corridor
  (0x2B79) and picking an adjacent empty cell (0x168D), writing 6-byte path records.
  Verified byte-identical to the real GM.EXE for missions 1/2/3 (`test_pass_2a9d`).

## v1.1.170 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: pass 0x1806 (`captive_gm_pass_1806`), the **main
  creature/item distribution** — the largest generator pass (~500 instructions across
  0x1840/0x1A3A/0x1226 and helpers).  For up to 0x1E0 random dead ends it finds a
  corridor spot (0x1A3A), records the path cells, optionally places a door and searches
  for a guard cell, then stamps the level markers and spawns creatures/items via the
  0x1226 sorted-list inserter and the 0xFE2 spawn engine, writing full entity records.
  Verified byte-identical to the real GM.EXE (cell-type + selector maps, entity buffers,
  both RNG states) for missions 1/2/3 (`test_pass_1806`).

### Fixed

- **(Captive)** GM 0xFE2 spawn engine: the word[0x356A] scalar uses an 8-bit `ror al,3`
  (not a 16-bit rotate) — exercised for the first time by 0x1806's creature spawns.

## v1.1.167 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: the creature/item **spawn engine** (0xFCB/0xFE2)
  and its placement pass 0xE12 (`captive_gm_pass_e12`).  0xFE2 selects a record from the
  0x6B16 spawn database (matching category/mission/flags) and writes creature/item entity
  records — position, type, RNG-scaled HP with signed saturation, and stats — into the
  entity buffers, drawing the secondary RNG.  0xE12 places two RNG-sized waves of
  creatures/items at surviving dead ends, spawning where the flow neighbour is a valid
  door/junction cell (helpers 0x2468, 0xE61; also exercises 0x16D7/0x2A59).  Verified
  byte-identical to the real GM.EXE across the whole 0..0x6A00 work region — maps, entity
  buffers, and both RNG states — for missions 1/2/3 (`test_pass_e12`).

## v1.1.165 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: pass 0x1314 (`captive_gm_pass_1314`) — the
  longest-dead-end objective search (helpers 0x1BF5 adjacency mask, 0x13D2 accept
  test).  The orchestrator runs it only when word[0x307C]==1 (standard missions skip
  it), so it is outside the normal pass chain; its found branch (objective/guardian
  spawn via the 0xAB4/0xB00/0xFCB entity subsystem) is left for later.  gm_1BF5 is
  verified byte-for-byte against GM via gm_call.py.

## v1.1.164 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: pass 0x2940 (`captive_gm_pass_2940`), the
  objective / special-item placer — stamps a 3x3 block of code 9 around the first
  qualifying room record (or a single cell at a random surviving dead-end), completing
  the first post-0xD12 pass group.  Verified byte-identical to the real GM.EXE
  (cell-type map, RNG in sync) for missions 1/2/3.

## v1.1.163 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: the post-0xD12 pass group 0x28B2 / 0x29F6 / 0x2888
  / 0x164C — dead-end (3-wall spur) detection & listing, probabilistic dead-end pruning
  (RNG-driven), the dead-end list shuffle, and the mode-2 "fill" room drawer.  Verified
  byte-identical to the real GM.EXE (cell-type + selector maps, and the RNG state stays
  in sync) for missions 1/2/3 (`test_pass_group_164c`).
- **(Captive)** Baked GM constant table block at ws:0x6AB2 (the 4-direction step deltas
  at 0x6AE4/0x6AEC and neighbouring lookup tables the post-0xD12 passes read).

### Changed

- **(Captive)** GM 0x2055 now supports mode 2 (cell-check validation + word[0x355A]
  valid-cell gate + 0x23/0xFFC4 fill draw), so 0x164C and later fill passes work.

## v1.1.162 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: pass 0x26BE (`captive_gm_pass_26be`), the per-cell
  wall / flow-direction flag pass — encodes into the aux map (0x2058) the direction to
  each cell's lowest-selector neighbour, fills empty cell-type cells with 7, and stamps
  the entry cell + the 5 door-record lists (door codes 0x1A/0x1B).  Verified
  byte-identical to the real GM.EXE (type + aux) for missions 1/2/3 (`test_pass_26be`).

## v1.1.161 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: pass 0x2589 (`captive_gm_pass_2589`), which clears
  the 0xFFFD room-outline markers 0x1617 wrote from the selector map.  Verified
  byte-identical to the real GM.EXE for missions 1/2/3 (selector nz/ck 1063/0x1CE3F,
  1280/0x25745, 1254/0x20B4F — `test_pass_2589`).  Also identified the orchestrator's
  word[0x3070]=1 step (GM 0x3BD) and the full post-0xD12 pass sequence (0x2589, 0x26BE,
  0x28B2, 0x29F6, 0x28B2, 0x2888, 0x164C, 0x2940, ...).

## v1.1.160 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: pass 0xD12 (`captive_gm_pass_d12`), the full
  drunkard's-walk room/corridor placement machine, transcribed and verified byte-exact
  against the real GM.EXE.  After 0xD12 the cell-type map (0x1048), selector map (0x38),
  and aux map (0x2058) are byte-identical to GM for missions 1/2/3, and both RNG states
  match (map1048 m1 550/0x42D6, m2 280/0x17FE, m3 191/0x10DE — `test_pass_d12`).
- **(Captive)** Baked GM constant table at ws:0x6D5E (the 2055 room skip-pattern table).

### Fixed

- **(Captive)** GM 0x2055 mode-0 tail: reload the cursor from word[0x6DE4] (last placed
  cell) and step it with the draw-loop's leftover bp=0xFFFF before 0x25B5.
- **(Captive)** GM 0x2055 draw scan: decrement word[0x33CE] per row so the final room
  row carves every cell (skip pattern disabled), matching GM.
- **(Captive)** GM carve (0x1FC5): run 0x25B5's junction relocation on the *stepped*
  cursor, not the pre-step cursor.

## v1.1.159 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: pass 0x1617/0x2055 (`captive_gm_pass_1617`), the
  room-outline drawer, transcribed and verified byte-for-byte against the real GM.EXE.

### Fixed

- **(Captive)** GM.EXE native port: passes 0x1CB5 and 0x1617 now preserve the RNG state
  (word[0x3074]) across the pass, matching GM's push/pop wrapping.

## v1.1.158 (2026-08-20)

### Changed

- **(Captive)** GM.EXE native port: strengthened 0x1CB5 verification to also cover its
  gate-on map writes (map1048 + map38 checksums vs the real GM.EXE).

## v1.1.157 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: the 0x1CB5 room-outline validator + anchor
  placement subsystem (~15 routines), transcribed and verified byte-for-byte against
  the real GM.EXE for three missions.

## v1.1.156 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: shared map-index/validation primitives
  (`captive_gm_map_index`, `captive_gm_grid_cell`, `captive_gm_cell_check`), transcribed
  from GM.EXE and verified — the building blocks the remaining map passes reuse.

## v1.1.155 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: the final 0xEE translate driver
  (`captive_gm_generate_output`), producing the 64x32 output map from the cell-type
  map via the verified translator.

## v1.1.154 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: pass 0x5D4 (`captive_gm_pass_5d4`), which expands
  the room grid into the full 2048-word input map, verified against the real GM.EXE.

## v1.1.153 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: pass 0x526 (`captive_gm_pass_526`), the
  region-connection + entry-cell pass, transcribed and verified against the real GM.EXE.

## v1.1.152 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: pass 0x45F (`captive_gm_pass_45f`), the room-grid
  region generator, transcribed and verified byte-for-byte against the real GM.EXE
  across five missions.

## v1.1.151 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: the RNG position generator
  (`captive_gm_rng_pos`, GM 0x1C97), transcribed and verified.

## v1.1.150 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: the generation RNG (`captive_gm_rng_next`,
  GM 0x1C6E), transcribed and verified byte-for-byte against the real GM.EXE.

## v1.1.149 (2026-08-20)

### Added

- **(Captive)** GM.EXE native port: baked-table loader (`captive_gm_init`) and the
  first generation pass (`captive_gm_pass_14c9`), transcribed from GM.EXE and
  verified byte-for-byte against the real generator via the oracle.

## v1.1.148 (2026-08-20)

### Added

- **(Captive)** Started the faithful native port of GM.EXE's level generator:
  `captive_gm_generator` with the entry setup (pointer table + mission params) and
  the seed/constant init, transcribed from GM.EXE and verified byte-for-byte against
  the real generator's work-segment state (via `opencaptive-re/gm_oracle.py`).
- **(Captive)** `docs/CAPTIVE_GM_PORT_PLAN.md` — the full generation-pipeline roadmap
  and byte-exact verification strategy.

## v1.1.147 (2026-08-20)

### Added

- **(Captive)** `captive_gm_translate` — a byte-for-byte native transcription of
  GM.EXE's per-cell map code translator (the routine at GM_UNP.EXE 0x129), with a
  wall predicate for GM output codes. Verified against the full 256-entry table
  captured from the real GM.EXE, plus the door/aux/marker special cases.

### Changed

- **(Captive)** Reverse-engineering result: the Captive DOS dungeon level is
  generated by the child program **GM.EXE** (launched by CAPPO via INT 21h AH=4Bh
  EXEC), not by CAPPO.EXE itself. GM.EXE now runs headless in the RE harness and
  produces distinct real base layouts per mission. See
  `docs/CAPTIVE_DOS_DUNGEON_RE.md` for the full chain; `captive_dos_generator.h`'s
  previously-unconfirmed note is corrected accordingly.

## v1.1.142 (2026-08-14)

### Changed

- **(Captive)** DOSBox is no longer used anywhere inside OpenCaptive. Every path
  that could shell out to DOSBox-X (the default/menu Captive launch, the live
  session, and the legacy `--captive-authentic` opt-in) has been removed or
  neutralised. Captive now always renders natively, reading its assets directly
  from the archive (zip/ADF/ISO) via the SHA-256 VFS, with no external runtime
  and nothing written to disk. Verified: launching Captive against zipped data
  loads the holomap/navigation references straight from the zip and renders,
  and no DOSBox process is started.

# OpenCaptive Release Notes

## v1.1.141 (2026-08-14)

### Changed

- **(Captive)** Interactive Captive now renders **natively, reading its assets
  directly from the archive** (zip/ADF/ISO) via the SHA-256 VFS — OpenCaptive
  never unpacks game data to disk. Previously the default Captive launch shelled
  out to DOSBox, which can only mount a real directory and therefore opened to an
  empty prompt when the DOS game lived inside `Captive_DOS_EN.zip`. The original
  DOS runtime under DOSBox remains available only via the explicit
  `--captive-authentic` opt-in for users who have supplied loose DOS files.
  Verified: launching Captive against zipped data loads the holomap and
  navigation references straight from the zip and renders, with no DOSBox
  process and nothing written to disk.

# OpenCaptive Release Notes

## v1.1.140 (2026-08-14)

### Fixed

- **Startup:** the "Missing Game Data" dialog now distinguishes the two real
  causes so users can act: **no archives found** in the folder (files not placed
  yet, still copying, or the app cannot read the folder — e.g. macOS folder
  access) versus **archives found but none contain the verified content** (wrong
  edition or an incomplete copy). Previously both showed the same generic
  message. Verified that the current build loads a minimal real data set
  (Captive_DOS_EN.zip + the Liberation CD32 disc-image zip) — Captive DOS and
  Liberation both verify and render — so a correct, complete `~/.opencaptive`
  starts the game; the old generic dialog most often appeared when the 443 MB
  Liberation image had not finished copying at launch.

# OpenCaptive Release Notes

## v1.1.139 (2026-08-14)

### Added

- **(Liberation)** `liberation_service_mc` — the reverse-engineered
  generic-building-service `mc` derivation, implemented as tested C. The
  CITY_TEXT (CTE) conversation script branches on `mc` to pick a building's
  service dialogue; recovered from the game binary (radare2 + Ghidra decompiler,
  function 0xa738), a generic building's `mc` is `0xa738(category_byte) + 3`.
  This module implements that decode and is unit-tested to yield only the
  generic-service range (mc 0-9) and never a named-NPC profession — the property
  that proved bank/repair are named NPCs, not building categories. This extracts
  real game logic into code (permitted: the purpose is to reproduce authentic
  behaviour), the first implementation step toward wiring authentic building
  transaction dialogue.

# OpenCaptive Release Notes

## v1.1.138 (2026-08-14)

### Changed

- **(Liberation)** The library "Search archives / Ask around" and City Records
  Office "Look up records" responses now deliver authentic clue quotes instead
  of invented registry/briefing text. Liberation's information buildings give
  cryptic clues, not explicit target guidance, so the previous explicit hints
  were an invented convenience; this restores the authentic clue delivery.
  Mission logic is unchanged and navigation remains via the automap. With no
  game data bound, the previous text is kept as a fallback. All six
  informational building contexts (shop, bar, residence, police, library,
  records) now surface authentic Liberation clue text.

## v1.1.137 (2026-08-14)

### Changed

- **(Liberation)** Extended the authentic-clue wiring to the police station's
  "Ask for information" response, which previously showed an invented briefing.
  It now delivers a real clue quote (same `liberation_city_text` provider, with
  the no-data fallback preserved). The fine mechanics and mission logic are
  unchanged. Shop, bar, residence, and police informational lines now all
  surface authentic Liberation clue text instead of synthetic rumor.

## v1.1.136 (2026-08-14)

### Added

- **(Liberation)** First on-screen wiring of authentic CTE dialogue. A new
  provider (`liberation_city_text`) surfaces the game's 16 authentic clue quotes
  — the literary clues Liberation's informants deliver (CITY_TEXT sections
  300+n / 316+n) — and the building interaction now shows a real clue in place
  of the invented "rumor/news" flavor lines: the residence informant and the
  shop's "Any news?" response. The functional mission-hint nodes
  (library/records) are untouched. When no game data is bound (e.g. a unit
  test), the provider returns false and the previous fallback text is kept, so
  nothing regresses. New unit test covers the no-data fallback, a fixture clue,
  and all 16 authentic quotes against the real CITY_TEXT.

### Notes

This replaces synthetic text with authentic game data where that data exists,
per the project's no-synthetic-data rule. The broader transaction/confrontation
dialogue (bank, repair, police negotiation) still awaits the interaction-model
redesign and `BuildingType` taxonomy work tracked in TODO.

### Removed

None.

## v1.1.135 (2026-08-14)

### Fixed

- **CI:** `test_version_consistency` failed on every build since v1.1.132
  because `include/opencaptive.h` (pinned at 131) was not bumped alongside the
  CMake project version. Both are now synced to 1.1.135. The version header must
  be updated with the CMake version on every bump.

### Added

- **(Liberation)** The verified CITY_TEXT (CTE) interaction script is now parsed
  once at data open into `LiberationData.city_text` (via `cte_table_parse`),
  freed in `liberation_data_close`. This binds the authentic dialogue table so
  the interpreter can be driven from real data. Not yet routed to the building
  overlay — the bank interaction is confirmed self-contained (reaches only its
  own sections, no `mc` gate) and is the first wiring candidate.

### Removed

None.

## v1.1.134 (2026-08-14)

### Added

- **(Liberation)** CTE `^XM` player-menu expansion. `^XM[...]` holds the
  player's spoken choices (e.g. the bank menu "I'd like to make a withdrawal
  from AC/No…", the police "Get on with it."), so its options are authentic
  text and are now emitted (newline-separated) instead of skipped. Verified
  against the real CITY_TEXT: sections producing authentic text under an empty
  state rose from 66 to **188 of 198** — the bank, police-negotiation, and
  business menus now surface their real lines. `^Xf[...]` branches, which
  contain only flag-actions (no dialogue), remain correctly skipped. All 198
  still expand with no leaked markers. New unit test covers menu expansion.

### Notes

The remaining gap to on-screen wiring is unchanged: `^XI`/`^XC` branches gated
on plot flags need the game-state field model. An all-zero state gives the
correct first-encounter dialogue, so wiring will start from there once the
field model lands.

## v1.1.133 (2026-08-14)

### Fixed

- **(Liberation)** CTE section framing was reverse-engineered incorrectly. The
  section id is a **16-bit big-endian** value framed `0xD7 <id-hi> <id-lo> 0x00
  <len> [ content ]`, not a single byte; the one-byte `<len>` overflows for long
  sections so content is bracket-delimited. The corrected marker-based parser
  finds the true **198 sections** (the old bracket-scan parser truncated ids and
  merged sections, finding only 48).

### Added

- **(Liberation)** CTE cross-table call resolution. The `^XS`/`^XG` jump targets
  are the same 16-bit section ids, so calls resolve *within* the CTE table.
  `cte_expand` now inlines `^XS` (subroutine) and `^XG` (goto) with a
  per-expansion call stack that refuses to re-enter an active id (cycle-safe).
  Skipped-opcode handling was widened to consume the `^Xf*…[…]` flag family,
  `^XM[…]` menus, and the lowercase location/status templater (`^L` range,
  `^A/^Z/^s/^h/^w` substitutions). Verified against the real CITY_TEXT: all 198
  sections expand with no leaked markers; **66 emit authentic text under an
  empty state — including the game's real clue quotes** ("The tigers of wrath
  are wiser…" and the full set). Remaining sections gate on game flags/menus and
  need the state-field model before they render. New unit tests cover call
  inlining, goto, and cycle safety.

### Notes

Still not wired to the building overlay: correct branch selection for
flag-gated sections needs the game-state field model (mc, v, g, s, H, E, …).
Until then, wiring would show only unconditional text, so it stays behind the
interpreter — no partial/wrong-context output is shown.

## v1.1.132 (2026-08-14)

### Added

- **(Liberation)** CTE bytecode interpreter — `cte_expand()` expands an
  authentic CITY_TEXT section to text for a supplied game state. It is a
  deliberately read-only expander: it emits the real dialogue a section
  produces but performs none of the script's side effects. It handles the
  self-contained, text-producing opcodes — `^O<n>[a|b|…]` random variant
  (seeded, deterministic), `^XI<cond>[then|else]` conditional (with `~`
  negation, `= < >` comparisons, and compound `(a!b)` OR / `(a&b)` AND),
  `^XC<var>[c0|c1|…]` case switch, `^^` newline, `^;` comment — and safely
  skips side-effect/action opcodes by consuming their operands. Validated
  against the real CITY_TEXT: all 48 sections expand with no leaked opcode
  markers or branch groups; unit-tested for each opcode.
- **(docs)** `LIBERATION_CTE_INTERPRETER_RE.md` extended with the implemented
  opcode set and a key finding: `^XS`/`^XG` call targets are 5-digit label ids
  into a separate string table (disjoint from CTE section bytes), so
  cross-table call resolution — not the interpreter itself — is the remaining
  work before the dialogue is wired on-screen.

### Notes

The interpreter is not yet wired to the building overlay: call-only sections
depend on cross-table label resolution, so wiring now would show empty or
wrong-context text. It stays behind the parser until call resolution lands —
no partial/invented output is shown.

### Removed

None.

## v1.1.131 (2026-08-14)

### Added

- **(Liberation)** `liberation_cte` — a parser for the authentic CITY_TEXT (CTE)
  interaction table, the original shop/bank/police/clue conversation script.
  CTE uses a binary section frame (`<id> 0x00 <len> [ content ]`, len = content
  + 3) unlike the ASCII DTE table, so the existing parser could not read it.
  This makes the real sections addressable; it is step one of decoding the CTE
  and is not wired to on-screen text, so no partial output is shown while the
  bytecode interpreter that expands a section is still to come. Verified it
  parses the real CTE (48 sections) plus fixture tests for the framing.

### Removed

None.

## v1.1.130 (2026-08-14)

### Changed

- **(Liberation)** Bar items show the factual label "Drink" instead of invented
  brand names (Synthi-Ale, Neuro-Fizz, Grav-Tonic…). The original bar has no
  drink menu in the data; the items remain only because buying one drives the
  real bar-fight mechanic, so the fabricated lore is removed while the mechanic
  is preserved.

### Removed

None.

## v1.1.129 (2026-08-14)

### Changed

- **(Liberation)** Combat opponents are named "Security Droid" — the term the
  game's own plot text uses ("security droids", "security personnel") — instead
  of eight invented sci-fi names (Guard, Sentinel, Enforcer, Warden…) that the
  original does not have. The stat generation is byte-for-byte unchanged (the
  name roll is still consumed).

### Removed

None.

## v1.1.128 (2026-08-14)

### Added

- **(Liberation)** `liberation_descriptions` module: maps the game's building
  categories to the original DTE "location description" sections and expands the
  authentic room description via the existing `lib_text_expand`. The DTE table
  is parsed once at data open.

### Changed

- **(Liberation)** Entering a shop, bar, business, industrial, or records
  building now shows the authentic Liberation room description from the game's
  own DTE text ("A general store, selling all manner of household goods…",
  "This bar is rather poorly lit, and quite noisy…") instead of invented
  English. The game-only categories (residence, library, police, special) have
  no original DTE section and keep their existing text.

### Removed

None.

## v1.1.127 (2026-08-14)

### Added

- **(Liberation)** `fnt_blit_glyph`/`fnt_blit_text` render the authentic
  decoded font, with tests pinning ink/shadow placement, scaling, advance
  width, and clipping against the real glyph data.

### Changed

- **(Liberation)** Building-interaction dialogue, choice labels, the NPC
  indicator, and the shop line now draw with the real 0Liberation.FNT instead
  of the invented `simple_font`, so authentic letterforms — lowercase included —
  appear on the live path. The invented font remains only as a fallback for a
  source with no recovered font hash (the Amiga floppies).
- **(Liberation)** The shop shows the authentic item-database name for each item
  it stocks (id 18 = PISTOL, 21 = RIFLE, 30 = MONO-CANNON…) instead of a
  parallel table of invented product names that had been propagating into the
  purchase message and the player's inventory.

### Removed

- **(Liberation)** The invented shop product-name table and, effectively, the
  decoded-but-unused status of the real font.

## v1.1.126 (2026-08-13)

### Added

- **(Verification)** Added the corrected CAPPO arrow-scan mapping to the
  source-faithful Captive navigation verification coverage.

### Changed

- **(Captive)** Corrected the graphical holomap arrow mapping to CAPPO's
  original keypad controls: up/down use `0x48/0x50` and left/right use
  `0x4B/0x4D`.
- **(Captive)** Kept live Orbit/LAND transitions fail-closed until the
  original DOSBox-X runtime proves arrival; no synthetic planet, marker,
  landing point, dungeon, or status text is introduced.

### Verification

- Local CTest suite: 64/64 passed.
- Authentic DOSBox-X CAPPO target-route, navigation/VGA replay, and live
  replay gates passed with the supplied original game data.
- Orbit arrival, LAND, dungeon entry, and physical mouse parity remain
  explicitly unverified and are not claimed by this release.

### Release status

This release contains the verified CAPPO input-direction correction and the
real-data-only runtime boundary. It does not claim complete end-to-end
Captive parity.

### Removed

None.

## v1.1.125 (2026-08-13)

### Added

- **(Verification)** Added the real-data-only CAPPO dispatcher and
  target-route proof boundary to the release verification workflow.

### Changed

- **(Captive)** Live holomap mouse motion now uses an accumulation buffer and
  CAPPO's documented discrete cursor scans; control-bank clicks use the same
  original scan path.
- **(Captive)** Complete DOSBox-X memory dumps now render directly from
  CAPPO's original A000:0000 VGA surface; descriptor reconstruction is no
  longer used as a visual fallback.
- **(Verification)** Added a real-data-only CAPPO target-route gate for the
  authentic green target and `FLIGHT PATH SET` boundary; Orbit and LAND remain
  explicitly unclaimed until the original runtime proves them.

### Verification

- Authentic DOSBox-X navigation/VGA and replay checks pass with the
  player-supplied CAPPO data; interactive mouse and live Orbit/Land remain
  unverified; the full local suite remains 64/64.

### Release status

- This release packages the verified original-data CAPPO route boundary and
  its DOSBox-X proof tools. It does not claim Orbit, LAND, dungeon entry, or
  complete end-to-end Captive parity.

### Removed

None.

## Unreleased

No changes recorded.

## v1.1.124 (2026-08-13)

### Added

- **(Verification)** Added a real-data-only CAPPO target-route gate that
  drives the original Mission 0001 route to the authentic green target and
  verifies the original `FLIGHT PATH SET` boundary.

### Changed

- **(Captive)** Complete DOSBox-X VGA dumps are rendered directly from the
  original A000:0000 surface, preventing reconstructed or synthetic viewport
  pixels from replacing authentic output.
- **(Captive)** Live holomap motion uses accumulated discrete CAPPO cursor
  scans, preserving the original input model for mouse-to-navigation mapping.
- **(Documentation)** Release packages now include the authentic-data route
  verifier alongside the existing DOSBox-X probes.

### Removed

None.

### Verification

- Local CTest suite: 64/64 passed.
- Authentic CAPPO target-route and live replay gates pass with the supplied
  original game data.
- Orbit, LAND, dungeon entry, and physical mouse interaction remain
  explicitly unclaimed until proven by an unlocked original runtime.

## v1.1.123 (2026-08-13)

### Added

- **(Verification)** Added the corrected DOSBox-X integration-device encoding
  to the authentic CAPPO mouse regression path, covering both pointer motion
  axes and the left-button event.

### Changed

- **(Captive)** Authentic CAPPO mouse deltas now use the DOSBox-X payload
  layout decoded by its BIOS integration-device handler, so pointer movement
  reaches the original INT 33 runtime instead of being discarded.
- **(Captive)** The DOSBox-X profile now uses the original 320x200 INT 33
  coordinate range and explicit mouse integration mode.
- **(Documentation)** Updated the English technical reference with the exact
  real-data-only transport boundary and the DOSBox-X encoding used by the
  release artifacts.

### Removed

None.

### Verification

- GitHub Actions Build passed on `main` at commit `53bb7cd`.
- Authentic CAPPO navigation and replay checks pass with the player-supplied
  original game data. The former paused-debugger mouse check is transport-only
  and is not evidence of interactive mouse parity.
- The complete local CTest suite remains 64/64.

## v1.1.122 (2026-08-13)

### Added

- **(Verification)** Added an authentic DOSBox-X regression gate for rejected
  Captive landing input.

### Changed

- **(Captive)** A live LAND command is now fail-closed: OpenCaptive keeps the
  original CAPPO surface when the original runtime has not authenticated
  orbit, instead of entering a local landing or dungeon view.
- **(Documentation)** Updated the Captive parity notes to distinguish the
  verified holomap/flight-path state from the still-unverified orbit and
  landing states.

### Removed

None.

## v1.1.121 (2026-08-13)

### Added

- **(Captive)** Live mouse motion and left-button input now travel through
  DOSBox-X's authentic integration-device path into CAPPO's INT 33 mouse
  handler. The bridge does not write cursor state or gameplay state into the
  original process.
- **(Verification)** Added a real-data-only mouse transport probe. It does not
  claim interactive mouse parity while DOSBox-X is paused for memory dumps.

### Changed

- **(Captive)** The desktop runtime no longer translates mouse movement into
  local arrow actions or handles clicks with a replacement hit-test table.
  CAPPO receives the original mouse events and remains authoritative.
- **(Verification)** Navigation verification no longer interprets the
  holomap/Orbit surface as a dungeon map. Arrival, LAND, and dungeon entry
  remain unclaimed until the original runtime proves those states.
- **(Documentation)** Updated the Captive technical reference and holomap
  reference with the authentic DOSBox-X mouse transport and its limits.

### Removed

- **(Captive)** Removed the synthetic local mouse-control fallback from the
  live emulator path.

## v1.1.120 (2026-08-13)

### Added

- **(Captive)** The DOSBox-X CAPPO replay harness now sends authentic XT key
  make and break scans for every navigation command, matching CAPPO's pressed
  and released control state.

### Changed

- **(Captive)** Live navigation verification uses the original runtime input
  path and no longer relies on a held-key approximation.
- **(Verification)** The real-data-only replay gate now verifies the updated
  input transport against the authentic CAPPO executable and VGA frames.

### Removed

None.

## v1.1.119 (2026-08-13)

### Added

- **(Captive)** The DOSBox-X verification path now uses CAPPO's executable
  state segment when decoding the authentic runtime memory image.

### Changed

- **(Captive)** Runtime rendering, map diagnostics, descriptor checks, and the
  local navigation gate now agree on the original CAPPO memory layout.
- **(Verification)** The parity gate continues to accept only player-supplied
  original CAPPO data and real DOS VGA memory captured from DOSBox-X.
- **(Documentation)** Public documentation baselines are synchronized to
  v1.1.119.

### Removed

None.

## v1.1.118 (2026-08-13)

### Added

- **(Captive)** The live DOSBox-X bridge now exposes the authentic Mission
  0001 navigation/map surface immediately after the original DEL handoff.

### Changed

- **(Captive)** Mouse clicks and motion remain on CAPPO's original input path
  after startup; no local cursor or generated navigation state is introduced.
- **(Verification)** The navigation gate now reports only the verified
  Mission 0001 input/render boundary and does not claim Orbit, arrival, LAND,
  or dungeon entry without a later original-runtime proof.
- **(Documentation)** The wiki landing page, technical reference, and release
  metadata are synchronized to v1.1.118.

### Removed

None.

## v1.1.117 (2026-08-13)

### Added

- **(Captive)** Pyramid/Enter now reaches the original CAPPO keyboard queue in
  live DOSBox-X sessions using the documented XT scan code `0x1C`.

### Changed

- **(Captive)** Live holomap arrows and zoom controls no longer mutate a local
  compatibility cursor alongside the original runtime.
- **(Captive)** The live emulator bridge now returns only after the authentic
  DEL handoff, so the first OpenCaptive frame is the real Mission 0001
  navigation/map surface and subsequent mouse clicks are sent to CAPPO's
  original control grid.
- **(Verification)** Navigation checks now describe Mission 0001 input/render
  parity accurately and keep Orbit, arrival, LAND, and dungeon entry gated on
  a later original-runtime proof.
- **(Captive)** Visual controls use the original CAPPO direction scans while
  keypad controls retain their documented raw meanings.
- **(Captive)** Live Mission 0001 dump decoding uses the post-handoff data
  segment observed in DOSBox-X; the older descriptor fixture remains an
  explicitly selected offline analysis mode.
- **(Verification)** CAPPO parity checks compare the complete original VGA
  surface and do not mistake resident message-table strings for active state.

### Removed

None.

## v1.1.116 (2026-08-12)

### Added

- **(Captive)** The real DOSBox-X CAPPO startup handoff now continues with the
  original DEL/left-mouse input after FILEPLAY, preserving the authentic Mission
  0001 holomap and its live runtime state.
- **(Infrastructure)** The release workflow packages and verifies the current
  CAPPO harness and real-data-only verification helpers for every desktop
  package.

### Changed

- **(Captive)** Navigation verification now distinguishes the authentic
  `FLIGHT PATH SET` and `SWAN NOT YET IN ORBIT` states from arrival and landing;
  no synthetic planet, route, landing, or dungeon data is used.
- **(Documentation)** Captive runtime references and release metadata are
  synchronized with v1.1.116.
- **(Infrastructure)** Release publication remains gated by the complete local
  test suite, release artifact checks, and green GitHub Actions jobs.

### Removed

None.

## v1.1.115 (2026-08-12)

### Added

- **(Infrastructure)** The DOSBox-X CAPPO sequence harness now keeps its
  authentic timer breakpoint installed while advancing timer ticks, avoiding
  debugger churn during long real-runtime verification sessions.

### Changed

- **(Captive)** Verification documentation now reports the observed
  `FLIGHT PATH SET` and `SWAN NOT YET IN ORBIT` states accurately; a displayed
  coordinate is not treated as arrival or landing evidence.
- **(Infrastructure)** Release packaging remains gated by the full local test
  suite and all platform GitHub Actions jobs.

### Removed

None.

## v1.1.114 (2026-08-12)

### Added

- **(Captive)** Relative mouse movement now drives CAPPO's original cursor
  direction scans after startup, allowing the real Captive runtime to be
  navigated with the host mouse.
- **(Infrastructure)** The release is gated by the complete local build/test
  suite, DOSBox-X CAPPO replay checks, and all platform CI jobs.

### Changed

- **(Captive)** Mouse navigation is accumulated and translated only into the
  original CAPPO input queue; no local map, planet, landing, or dungeon state
  is synthesized.
- **(Documentation)** Captive technical documentation now describes the
  source-faithful mouse-to-scan handoff and its real-data-only boundary.

### Removed

None.

## v1.1.113 (2026-08-12)

### Added

- **(Captive)** The release contains the verified `main` runtime boundary for
  authentic CAPPO startup and DOSBox-X input handoff.
- **(Infrastructure)** The complete release workflow continues to build Linux
  tarball/deb/rpm/AppImage, macOS DMG, Windows installer, iOS IPA, and Android
  APK artifacts.

### Changed

- **(Captive)** Input remains tied to the original runtime's authenticated
  keyboard and mouse-scan path; no synthetic game state is used.
- **(Documentation)** Version references and release metadata are aligned with
  v1.1.113. The project remains real-data-only: original game media is never
  generated or bundled.
- **(Infrastructure)** Publishing is gated by the full local test suite, the
  DOSBox-X CAPPO verification gates, artifact checks, and green GitHub Actions.

### Removed

None.

## v1.1.112 (2026-08-12)

### Added

- **(Captive)** A local live-replay verification gate now drives repeated
  authentic CAPPO scan codes through DOSBox-X and validates every resulting
  1 MiB memory dump and 320×200 VGA frame.
- **(Infrastructure)** The release packages include the replay verification
  helper beside the original CAPPO DOSBox-X harness.

### Changed

- **(Captive)** Replay acknowledgement is counted per scan occurrence, so
  repeated keypad commands are synchronized with the original runtime rather
  than being mistaken for stale output.
- **(Infrastructure)** Release packaging and version checks now target
  v1.1.112; no original game data is included.

### Removed

None.

## v1.1.111 (2026-08-12)

### Added

- **(Captive)** The normal Captive start path now keeps the original CAPPO
  runtime active inside OpenCaptive and forwards authentic keyboard and mouse
  navigation commands through its IRQ1 input queue.
- **(Infrastructure)** Desktop release packages include the real DOSBox-X
  session harness required for the authenticated Captive handoff.

### Changed

- **(Captive)** Numpad 1–9 mappings now follow the original help table,
  including forward, backward, zoom, Orbit, and Land behavior.
- **(Captive)** OpenCaptive's native audio mixer is muted while CAPPO owns the
  runtime, preventing a duplicate soundtrack or continuous mixer noise.
- **(Infrastructure)** Release packaging verifies the CAPPO harness is present
  in Linux, macOS, and Windows desktop artifacts.

### Removed

None.

## v1.1.110 (2026-08-12)

### Added

- **(Captive)** The DOSBox-X CAPPO verification harness now accepts an explicit
  post-input timer interval, allowing the authentic landing dungeon checkpoint
  to be reproduced without treating a later title or mission surface as the
  viewport.

### Changed

- **(Captive)** The verified Orbit-to-Land sequence is documented as an
  emulator-timed handoff: keypad 7 and keypad 9 are delivered through CAPPO's
  real IRQ1 queue and the resulting VGA surface is taken from the original
  runtime memory.
- **(Infrastructure)** The release is gated by the complete local build/test
  verification and green GitHub Actions build and release workflows.

### Removed

None.

## v1.1.111 (2026-08-12)

### Added

- **(Captive)** The verified DOSBox-X Orbit-to-Land checkpoint now includes the
  original CAPPO dungeon VGA surface and the decoded live map/draw records.
- **(Infrastructure)** Release verification records the complete 61-test local
  suite and the green GitHub Actions build for the preceding `main` commit.

### Changed

- **(Captive)** The runtime renderer prefers the exact 320×200 VGA surface
  produced by CAPPO from a real DOSBox-X memory dump, preserving the original
  viewport pixels and control panel without synthetic replacement artwork.
- **(Documentation)** The user guide, wiki landing page, technical references,
  and format notes now identify v1.1.111 and retain the real-data-only boundary.

### Removed

None.

## Release notes policy

Each release must have a handwritten version section with **Added**, **Changed**,
and **Removed** headings. Write `None` when a category has no changes. The
GitHub release publishes exactly that section; autogenerated changelogs are not
used. Each item is prefixed with the game it applies to: **(Captive)**, **(Liberation)**,
**(Both)**, or **(Infrastructure)**.

## v1.1.108 (2026-08-12)

### Added

- **(Captive)** The technical reference now records the verified CAPPO relocation
  and input-queue boundaries recovered from the original DOS runtime.

### Changed

- **(Captive)** Mission 0001 navigation documentation now distinguishes a
  displayed flight coordinate from CAPPO's actual arrival and Orbit state.
- **(Infrastructure)** Release validation remains fail-closed: only authentic
  CAPPO/DOSBox-X VGA handoffs may unlock Orbit or landing progression.

### Removed

None.

## v1.1.107 (2026-08-12)

### Added

- **(Captive)** The live DOSBox-X memory bridge now accepts the authentic orbit VGA checkpoint while CAPPO remains in the holomap flight-path state.

### Changed

- **(Captive)** Orbit is entered only after an exact real CAPPO VGA handoff; keypad 7, the Orbit button, timers, and guessed state cannot fabricate arrival.
- **(Documentation)** Captive navigation and technical notes now describe the observed `FLIGHT PATH SET` phase and the real-data-only orbit boundary.
- **(Infrastructure)** Release verification includes the complete 64-test suite, byte-exact CAPPO dump verification, and GitHub Actions build/release gates.

### Removed

- **(Captive)** The incorrect local second-Orbit transition that could display an orbit checkpoint without a live original-runtime handoff.

## v1.1.106 (2026-08-12)

### Added

- **(Captive)** The first Orbit command at the authentic holomap target now records the CAPPO flight path and displays the original `FLIGHT PATH SET` message before leaving the holomap.

### Changed

- **(Captive)** Orbit now follows the verified two-step CAPPO input sequence: select the real target, set the flight path, then issue Orbit again to begin transit. No destination, terrain, marker, or runtime state is generated.
- **(Infrastructure)** The complete 64-test local suite and the GitHub Actions build/release gates are required before publishing this version.

### Removed

None.

## v1.1.105 (2026-08-12)

### Added

- **(Captive)** The authentic DOSBox-X profile is shipped with every desktop package, including the macOS application bundle, so the original CAPPO runtime can be launched without falling back to an older local profile.
- **(Infrastructure)** Release packaging now asserts that the authentic Captive profile is present before publishing Linux, macOS, and Windows artifacts.

### Changed

- **(Captive)** DOSBox-X mouse capture is enabled for the authentic runtime, preserving the original game-pointer path when the packaged game starts.
- **(Documentation)** Runtime and packaging notes are updated for v1.1.105 and keep the real-data-only boundary explicit.

### Removed

None.

## v1.1.104 (2026-08-12)

### Added

- **(Captive)** The authentic Mission 0001 target frame is selected when the holomap cursor reaches the real target coordinate, using the tracked CAPPO capture without generating a marker, terrain, text, or map pixels.
- **(Infrastructure)** Holomap target-frame selection has a regression test and is included in the release verification path.

### Changed

- **(Captive)** Holomap target selection now exposes the real target checkpoint before flight-path controls are used, keeping the navigation flow tied to captured CAPPO state.
- **(Documentation)** README, wiki pages, and technical references are aligned with v1.1.104 and the authenticated target-frame boundary.

### Removed

None.

## v1.1.103 (2026-08-11)

### Added

- **(Captive)** Native space-navigation controls now respond to keypad 8/2/4/6, the on-screen arrows, keypad 7 for the authenticated orbit checkpoint, and keypad 9 for the authenticated landing transition.
- **(Infrastructure)** Release verification keeps the strict real-DOSBox-X landing handoff and the complete VGA comparison in the packaged build.

### Changed

- **(Captive)** The native fallback no longer advances from the landing transition on a timer or invents a dungeon state; it waits for a live CAPPO/DOSBox-X dump whose complete 320×200 VGA frame matches the authentic landed checkpoint.
- **(Documentation)** Captive parity notes now state the verified boundary and the remaining live post-landing handoff requirement explicitly.

### Removed

- **(Captive)** Dead-end native space-flight input handling that ignored the movement cluster after flight-path selection.

## v1.1.102 (2026-08-11)

### Added

- **(Captive)** The macOS launcher starts the authentic DOSBox-X `CAPTIVE.BAT 1` chain directly, preserving the selected profile and real data path.
- **(Captive)** Emulator verification covers the original Mission 0001 holomap, target selection, and the authentic `FLIGHT PATH SET` response using DOSBox-X numpad input.

### Changed

- **(Captive)** DOSBox-X uses a source-faithful 640x400 surface configuration with memory-I/O optimization disabled, removing repeated glyph and arrow corruption.
- **(Infrastructure)** The complete CAPPO DOSBox-X memory bridge, raw keyboard queue probe, and byte-exact VGA verifier are included in the build and documentation.

### Removed

- **(Captive)** macOS `open -a` handoff, which could reuse an old DOSBox-X instance and discard the requested `-conf` and `-c` arguments.

## v1.1.101 (2026-08-10)

### Added

- **(Captive)** Runtime guards keep legacy demo, story, and loading states off
  the Captive path unless verified original CAPPO media supplies the frame.

### Changed

- **(Infrastructure)** `--data` now accepts a ZIP archive directly as well as a
  data directory; cached verification remains content-addressed and is
  invalidated when the VFS source contract changes.
- **(Captive)** Startup uses the decoded original ANM when present, then the
  verified DOSBox-X holomap and landing checkpoints.
- **(Captive)** Headless frame verification now selects the authenticated
  landed CAPPO checkpoint instead of exposing the unfinished descriptor
  viewport prototype.
- **(Captive)** The native path no longer paints generated stars, coordinates,
  or mission text while entering Captive.
- **(Documentation)** Public version markers and parity notes now distinguish
  real CAPPO captures from the still-gated live dungeon decoder.

### Removed

- **(Captive)** Synthetic startup prose and generated transition overlays from
  the active runtime path.

## v1.1.100 (2026-08-09)

### Added

- **(Infrastructure)** `renderer_map_point()`, an SDL-free inverse of the
  presentation transform, with regression tests covering Retina/HiDPI,
  letterbox with fractional integer scaling, the launcher canvas, and
  widescreen pillar-boxing.

### Changed

- **(Captive)** Fixed mouse hit-testing, reported from a MacBook Pro. The
  window-to-canvas mapping computed its own scale from logical window points
  and never floored to an integer, while the renderer scales in output pixels
  and floors the game canvas. The two agreed only when the window was an exact
  integer multiple of 320x200; on an external monitor, an odd `--resolution`,
  or a resized window they diverged and clicks — including the holomap, orbit,
  and landing navigation controls — landed off-target or were rejected. Both
  the draw and the hit-test now share one transform.

### Removed

None.

## v1.1.98 (2026-08-09)

### Added

- **(Infrastructure)** A regression test for the `.po` parser overflow below,
  verified to fail against the unfixed parser.

- **(Liberation)** Verified SHA-256 entries for the original `0Liberation.FNT`
  and `1Liberation.FNT`, computed from the CD32 disc. Both load through the
  manifest and decode to 114 proportional glyphs in two bitplanes, covering the
  full printable ASCII range including lowercase.
- **(Liberation)** A regression test asserting the fonts stay OPTIONAL
  resources, plus a decode test against the real font file, enabled by pointing
  `OPENCAPTIVE_TEST_FNT` at it.

### Changed

- **(Infrastructure)** Fixed a one-byte out-of-bounds write in the `.po`
  translation parser. An unrecognized escape sequence emits two bytes, but the
  guard reserved only one and could never fire, so a catalog string that filled
  the buffer to its last free slot pushed the terminator past the end of the
  parser's stack buffer. Reachable from any shipped translation catalog.
- **(Liberation)** Source verification now requires only the seven core
  resources rather than every manifest entry. Optional resources are listed
  after `LIBERATION_RESOURCE_REQUIRED_COUNT`, so a gap in one cannot make an
  otherwise complete install fail to load. Without this split, adding the font
  entries would have broken Amiga floppy loading outright, since no Amiga font
  hash is known.

### Removed

None.

## v1.1.97 (2026-08-09)

### Added

- **(Captive)** A regression test asserting the shop draws nothing at all
  without the original backdrop.
- **(Liberation)** A regression test asserting the city renders no geometry
  without the original palette.

### Changed

- **(Captive)** The shop no longer synthesizes a backdrop. It previously
  invented a panel, border, and title bar in place of the original SHOP1
  artwork whenever that artwork was missing.
- **(Liberation)** The 3D viewport no longer synthesizes polygon colours. It
  previously invented a colour ramp for every polygon whenever the original
  palette was absent or shorter than the colour index; unresolvable polygons
  are now left undrawn.
- **(Liberation)** `city_nav_render` and `city_nav_render_textured` now honour
  their `palette`/`pal_size` arguments. Both accepted them and then read
  `render->palette` instead, so a caller that passed the palette only as an
  argument rendered with no palette at all.
- **(Infrastructure)** Three viewport tests and one shop test asserted only
  that a pixel was "not black" or that two buffers differed, which passed even
  when nothing was drawn. They now assert the exact palette colour, so an
  inverted depth sort or a blank frame fails.

### Removed

None.

## v1.1.96 (2026-08-09)

### Added

- **(Liberation)** The ten CD32 Red Book audio tracks now have SHA-256 entries
  in the data manifest, computed from the original disc image.
- **(Infrastructure)** `include/png_loader.h` gives the PNG loader a shared
  declaration instead of a prototype hand-copied into another translation unit.

### Changed

- **(Liberation)** CD audio is located by content hash through the VFS. It was
  loaded by hardcoded filename, bypassing verification entirely — any file with
  the right name was played as game audio.
- **(Captive)** The creature sprite atlas now requires the six ALIEN sheets to
  be present, like every other sheet. A missing ALIEN sheet previously passed as
  a complete atlas and reached a procedural fallback that drew invented
  rectangles instead of original sprite data. That fallback is removed, as is
  the invented ceiling/floor gradient.
- **(Captive)** The game-over and endgame screens use the original CAPPO.EXE
  strings, which were already transcribed but unused. The invented wording and
  the LEGENDARY/ELITE/VETERAN/SKILLED/ROOKIE rank ladder, which Captive does not
  have, are gone.
- **(Liberation)** Quicksave works inside building interiors. Interiors run the
  Captive dungeon loop, so the key reached the Captive save path, which rejects
  a Liberation session and discarded the interior's progress silently.
- **(Liberation)** Entering a building interior resets the puzzle list. Stale
  puzzles kept coordinates from the previous map, firing traps on unrelated
  cells and swallowing the interact key in front of generators.
- **(Liberation)** Purchases that do not fit the 40-slot city inventory stay
  pending instead of being destroyed after the gold was already debited.
- **(Captive)** Shop purchases, repairs, and grenade throws act once per key
  press. Auto-repeat previously drained gold and the whole grenade stock.
- **(Both)** Clicking anywhere in the settings screen commits the data-path
  field. Leaving settings with the mouse left an invisible active text field
  that swallowed all later key input and corrupted the configured path.

### Removed

None.

## v1.1.95 (2026-08-09)

### Added

None.

### Changed

- **(Infrastructure)** SHA-256 hash comparison is now case-insensitive. The data
  scanner's validator accepted uppercase hex digits while the comparison only
  matched lowercase, so an uppercase expected hash never matched a real file.
  The lookup then scanned every directory and archive fruitlessly and wrote a
  negative cache entry, making present game data appear permanently missing.

### Removed

None.

## v1.1.94 (2026-08-09)

### Added

None.

### Changed

- **(Liberation)** City NPCs now spawn and wrap across the full 64x64 city grid.
  They previously used a hardcoded 32x32 space, confining every NPC to one
  quadrant and teleporting them at the wrong boundary.
- **(Liberation)** City generation now guarantees at least one SPECIAL building.
  Seeds that produced none left the mission objective unreachable.
- **(Captive)** The automap is reset when starting a new game, continuing from a
  save, and landing on a new planet. Visited-cell markers from a previous
  dungeon previously bled onto the new map.
- **(Captive)** Floor traps no longer apply environmental damage to a droid that
  is already dead, in both the interact and step-on paths.

### Removed

None.

## v1.1.93 (2026-08-09)

### Added

None.

### Changed

- **(Both)** Difficulty is now persisted across all three save paths: the main
  save state extension (version 3), the portable cross-save (version 4), and the
  Liberation quicksave. It was previously lost on every load.
- **(Liberation)** Version 7 quicksave files are accepted again; the version was
  missing from the loader whitelist.
- **(Liberation)** The fade effect covers the full PAL canvas. It used the
  Captive pixel count, leaving the bottom 56 rows unfaded.
- **(Liberation)** The bar screen uses the correct canvas dimensions.
- **(Both)** Exiting the attract-mode demo re-syncs the menu from the active
  configuration instead of leaving stale values on screen.
- **(Both)** The CDDA player no longer enters a silent infinite loop on tracks
  whose size is not 4-byte aligned.

### Removed

None.

## v1.1.92 (2026-08-08)

### Added

None.

### Changed

- **(Both)** The settings menu Up key moves from the first right-column item to
  the last left-column item instead of getting stuck on the GAMMA row.
- **(Liberation)** Crime level and wanted status persist through quicksave and
  restore. The save format is version 9, with backward compatibility for
  versions 1 through 8.
- **(Captive)** Shop repair no longer charges for unequipped body-part slots.
- **(Captive)** Electric traps no longer damage already-dead droids.
- **(Captive)** Unequipping a weapon into a full inventory now correctly reports
  failure instead of silently succeeding and discarding the weapon.

### Removed

None.

## v1.1.91 (2026-08-08)

### Added

- **(Captive)** Added disassembly-backed documentation for the packed creature
  placement record and its subcell field at `0x53D1`.
- **(Infrastructure)** Moved the repository's technical and user documentation
  into `docs/` and updated relative links.

### Changed

- **(Infrastructure)** Expanded localization validation to cover the scanner,
  version-selection popup, and all F10 runtime-option labels across the shipped
  gettext catalogs.
- **(Captive)** Corrected spawn category documentation and type `0x0F` spawn
  count regression coverage against the verified CAPPO disassembly.
- The start menu now uses the restored Captive and Liberation card artwork
  from the original project assets.
- Start-menu card artwork now preserves its source aspect ratio when rendered.
- Captive ANM frame and command offsets are now documented and regression
  tested as little-endian values, matching the verified decoder behavior.
- Captive version selection now maps mouse clicks to the filtered rows shown
  in the popup, so a single remaining source can be selected reliably even
  when it is not the first platform candidate.
- Bar drinks are now consumed at purchase time instead of being copied into
  the shared inventory with non-runtime item IDs. Buying drinks still charges
  gold and performs the documented bar-fight roll.
- A full shared inventory no longer blocks bar purchases.

### Removed

- None.

## v1.1.90 (2026-08-08)

### Added

- **(Captive)** Creature respawns now wait for the original cell to become
  available instead of overlapping the party or another active creature.
- **(Captive)** Added regression coverage for safe respawn retry behavior.

### Changed

- **(Launcher)** Captive and Liberation start-menu cards now use the supplied
  restored artwork, with source aspect ratios preserved by the existing
  centered fit renderer.
- **(Captive)** Grenade kills now use the shared CAPPO kill bookkeeping path,
  so explosive kills correctly award score, gold, drops, shared XP, respawn
  state, and the per-attack kill event.
- **(Captive)** Kill registration is now idempotent for alternate weapon
  paths, preventing duplicate score, gold, drops, or XP if a finalized target
  is presented again.
- **(Captive)** Spray splash kills now use the same reward and progression path
  as direct kills, including XP for living droids, score, gold, drops, respawn
  state, and the per-attack kill event.
- **(Liberation)** The verified CD32 city presentation now decodes its real
  six-bitplane PACK frame. The previous defensive five-plane limit incorrectly
  hid the source-backed city frame during data verification.
- **(Liberation)** BuildingGen records now preserve the disassembly-documented
  building category bits in the flags byte while retaining connection counts.
- **(Captive)** Creature rendering now resolves the disassembly's `DS:0xA16E`
  graphic IDs to the correct ALIEN1–6 sheets instead of treating creature enum
  values as direct sheet indexes.
- **(Captive)** Descriptor source-bank resolution now routes bank 4 to
  `ROOFS.PL5`, matching the recovered CAPPO descriptor table, with regression
  coverage for wall, roof, and invalid banks.
- **(Captive)** The active compatibility viewport now samples floor and ceiling
  strips from `ROOFS.PL5` instead of a range-indexed wall sheet.
- **(Captive)** Wall rendering now selects the `WALLA–WALLE` source set from
  each cell's `wall_tex` value rather than incorrectly using view depth.
- **(Captive)** Creature rendering now honors the disassembly `frame_index`
  from `DS:0xA16E` instead of always drawing ALIEN sheet frame zero.
- **(Captive)** Creature kills now award recovered CAPPO XP to every living
  droid, matching the shared progression loop at `0x9621`.
- **(Launcher)** Captive and Liberation card artwork now preserves its source
  aspect ratio with centered letterboxing instead of stretching to the card.
- **(Infrastructure)** CI now runs the complete 61-test suite on Windows,
  including large `GameState` fixtures, with portable temporary test paths.
- **(Infrastructure)** Windows release packaging now includes the portable
  `save_load` regression instead of excluding it from the release gate.
- **(Infrastructure)** All 18 translated PO catalogs are now valid gettext
  catalogs, and CTest validates their required launcher strings to prevent
  duplicate or missing entries from reaching a build.
- **(Captive)** Puzzle floor and wall-electric hazards now route through the
  shared shield-aware environmental damage path.
- **(Captive)** Malformed shield records with negative durability are rejected
  before they can corrupt runtime equipment state.
- **(Infrastructure)** The release workflow now creates a valid macOS
  `Info.plist`, installs Inno Setup on Windows runners, and includes launcher
  card assets in the Windows installer.

### Removed

- None.

## v1.1.89 (2026-08-07)

### Added

- **(Captive)** Complete 112-entry viewport descriptor table extracted from
  CAPPO.EXE, with a verified descriptor decoder and validation test. The
  live viewport still uses the compatibility renderer; descriptor draw-order
  integration remains open work.
- **(Liberation)** CD audio music playback — 10 Red Book tracks from CD32 disc
  image via dedicated SDL3 stereo audio stream.
- **(Liberation)** Speech/voice sample playback system (8SVX loader).
- **(Liberation)** Original Amiga/CD32 indexed color palette wired into city
  rendering (walls, ground, building entrance objects).
- **(Liberation)** NPC system (pedestrians, police, vendors), crime system,
  city destruction, bar mini-game, mission cutscenes, endgame credits.
- **(Liberation)** Save game thumbnails (80×50, CTMB trailer format).
- **(Documentation)** Public status now distinguishes verified data and runtime
  slices from the remaining Captive viewport and Liberation campaign parity
  work.

### Changed

- **(Liberation)** Fixed NULL deref in lib3d_render_object when palette is set
  before state validation.
- **(Liberation)** Fixed CDDA player exhausting all 64 sound sample slots by
  switching to a dedicated SDL3 stereo audio stream.
- **(Liberation)** Fixed 240KB stack allocation in building dialogue by building
  the dialogue tree directly in the destination struct.
- **(Liberation)** Fixed droid UI equip click acting on stale cursor for
  invalid equipment rows 8-9.
- **(Liberation)** Fixed cutscene negative mission index causing out-of-bounds
  array read.
- **(Liberation)** Added bounds clamp in citygrid cleanup_building_records.
- **(Infrastructure)** Updated all documentation and wiki pages to v1.1.89.

### Removed

None.

## v1.1.83 (2026-08-07)

### Added

- **(Liberation)** Regression coverage for bar purchases with a full shared
  inventory.
- **(Infrastructure)** New wiki pages: CLI Reference, Controls, Custom Features;
  per-platform installation guides; Internationalization, Energy and Body Parts,
  Mission Flow, Save and Load, Start Menu, disassembly notes. 25 wiki pages
  total, 11 technical reference documents.

### Changed

- **(Captive)** ANM frame and command offsets are now documented and regression
  tested as little-endian values, matching the verified decoder behavior.
- **(Captive)** Version selection now maps mouse clicks to the filtered rows
  shown in the popup, so a single remaining source can be selected reliably.
- **(Liberation)** Bar drinks are now consumed at purchase time instead of being
  copied into the shared inventory with non-runtime item IDs. Buying drinks
  still charges gold and performs the documented bar-fight roll.
- **(Liberation)** A full shared inventory no longer blocks bar purchases.
- **(Captive)** SHOP1/SHOP2/KEYBOARD PL5 sheets are now wired into the texture
  atlas, replacing synthetic placeholder graphics.
- **(Captive)** Replaced synthetic screens with real GAMESCRN.PL5 background
  when verified game data is present.
- **(Both)** Updated all 19 language catalogs with droid configuration screen
  strings.

### Removed

- **(Liberation)** Removed dead `citygen_connect_roads` declaration from header.

### Technical details

- Comprehensive code review (5 iterations, ~80 source files, 76 headers, 25
  tools): fixed ~60 bugs total across three review rounds.
- Fixed unchecked malloc/calloc in 7 tool programs (rnc_decode, dump_pge,
  anm_extract, pl5_to_bmp, pl5_analyze, liberation_presentation_capture,
  liberation_presentation_inventory).
- Fixed integer overflow in rnc_decode (unp_size + 4096 wraparound) and two
  liberation_presentation tools (packed_size > INT_MAX - 12).
- Fixed file handle leak in visual_compare on fwrite failure.
- **(Liberation)** Fixed save preflight missing reputation_size in bounds check.
- **(Captive)** Fixed test_game_state using invalid level index for mission 5.
- **(Captive)** Fixed test_spawn asserting type 0x0F spawns 3 creatures (correct: 2).
- **(Captive)** Fixed shop gold overflow with INT_MAX guard on large costs.
- **(Both)** Fixed MIDI voice stealing to always send note-off before reuse.
- All 58 CTest tests passing on macOS, Linux, and Windows.

## v1.1.80 (2026-08-07)

### Added

- **(Captive)** Enemies now use the correct increasing encounter density: three
  groups at level 0 and two additional groups per subsequent level, capped by
  the recovered eight-step profile.
- **(Captive)** Combat now has regression tests for level density, attack events,
  spray-weapon range, XP, respawning, and floor transitions.
- **(Both)** The shop menu now shows item names together with price and selection
  state.
- **(Infrastructure)** The release check verifies Captive data against both DOS
  and Amiga sources.

### Changed

- **(Captive)** `creature_killed` and `level_up_occurred` are now reported per
  attack and cannot leak from a previous shot.
- **(Captive)** Enemy HP now uses the dungeon level as the zero-based difficulty;
  levels 0 and 1 no longer share the same HP profile.
- **(Captive)** Spray weapons use range 4, while other ranged weapons use range 6.
- **(Captive)** Seeded saves now restore their authoritative mission seed and can
  continue floor transitions correctly after loading.
- **(Both)** The start menu synchronizes version and platform selection after
  rescanning, and new launcher and droid strings are localized in all 19
  languages.
- **(Infrastructure)** The release workflow now requires the requested release
  version to match the CMake version before artifacts are published.

### Removed

- No user-facing functionality was removed in this version.

### Technical details

- **(Captive)** Scanning, saves, puzzle progression, viewport, and combat now
  have broader deterministic regression coverage across multiple seeds.
- Full local verification covers 58/58 CTest tests.
- **(Infrastructure)** GitHub Actions builds and tests macOS, Linux, and Windows
  before the release job is allowed to create the GitHub release.

## v1.1.79 (2026-08-06)

### Added

- **(Both)** The F10 menu now provides direct control over visual enhancements,
  filters, scanlines, CRT, lighting, help overlays, and optional cheats.
- **(Both)** Replay recordings can be saved on exit with `--replay-record`; the
  path can be selected with `--replay-output`.
- **(Infrastructure)** Linux packages, RPM, and AppImage now ship the resources
  required by the start menu and find them after installation.
- **(Infrastructure)** The release workflow can be started manually with an
  explicit semantic version and creates the corresponding GitHub tag.

### Changed

- **(Both)** Saves, cross-save, replay, and configuration are written atomically
  so an interruption cannot destroy the last working file.
- **(Captive)** Saves use a portable little-endian format, while older versions
  remain readable.
- **(Both)** `--hd-upscale`, `--upscale-factor`, `--widescreen`, and `--hq-midi`
  now have an observable effect during gameplay.
- **(Liberation)** Correctly restores transient gameplay state on Continue and
  rebuilds the seeded city before restoring state.
- **(Infrastructure)** The data cache and ZIP search are more robust when
  archives are replaced, files change rapidly, or compressed entries are empty.
- **(Infrastructure)** Release builds verify that all desktop and mobile packages
  exist before a release is published.

### Removed

- No user-facing functionality was removed in this version.

## v1.1.72 (2026-08-04)

### Added

- **(Both)** macOS native menu bar now shows localized text (Quit, Hide, Window,
  etc.) matching system language via Objective-C runtime patching.
- **(Both)** Real-time progress bar with percentage during SHA-256 hash scanning.
- **(Both)** First-run popup when data folder doesn't exist explaining what game
  data is needed and pointing to Settings.
- **(Both)** 48 new translatable strings added across all 18 language .po files.

### Changed

- **(Infrastructure)** iOS .ipa build (AltStore Classic sideload) added to
  release workflow.
- **(Infrastructure)** Android .apk build added to release workflow.
- **(Infrastructure)** Fixed heredoc indentation causing broken .desktop, .deb
  control, and .rpm spec files.
- **(Infrastructure)** Fixed packaging copying .mo instead of .po files (app
  loads .po at runtime).
- **(Infrastructure)** Fixed iOS build linking Cocoa framework (macOS-only).
- **(Infrastructure)** Platform-specific default data paths: Android
  (/sdcard/OpenCaptive), iOS (Documents/OpenCaptive).

### Removed

- No user-facing functionality was removed in this version.

## v1.1.65 (2026-08-03)

### Added

- **(Infrastructure)** Comprehensive README.md rewrite reflecting current
  project status.
- **(Infrastructure)** Full feature documentation: both games, start menu,
  controls, CLI options, reverse engineering table.
- **(Infrastructure)** Wiki links, project structure, supported data sources,
  build/test/run instructions.
- **(Infrastructure)** Release packages documented: Linux (deb/rpm/AppImage/
  tar.gz), macOS (DMG), Windows (Inno Setup).

### Changed

- None.

### Removed

- None.
