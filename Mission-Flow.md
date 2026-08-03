# Mission Flow

OpenCaptive has two distinct mission structures: Captive's 10-mission dungeon campaign and Liberation's city-based missions.

## State Machine

The game progresses through states defined by `GameStateMode`:

```
STATE_MENU
STATE_INTRO
STATE_DROID_CONFIG
STATE_GAME
STATE_SHOP
STATE_TERMINAL
STATE_INVENTORY
STATE_PAUSE
STATE_HELP
STATE_HOLAMAP
STATE_CITY_MAP
STATE_GAMEOVER
STATE_VICTORY
```

### Captive State Flow

```
STATE_MENU --> STATE_INTRO --> STATE_DROID_CONFIG --> STATE_GAME
                                                        |
                                                        v
                                              (destroy generators)
                                                        |
                                                        v
                                                  STATE_HOLAMAP
                                                   (shop access)
                                                        |
                                                        v
                                                  STATE_GAME (next mission)
                                                        |
                                              (mission >= 10?)
                                                        |
                                                        v
                                                  STATE_VICTORY
```

### Liberation State Flow

Liberation uses boolean flags for substates rather than separate `GameStateMode` values:

- `liberation_intro_active` -- decoded CD32 intro frame
- `liberation_mission_menu_active` -- AMOS sprite mission menu
- `lib_mission_briefing` -- city name, victim, news source
- `lib_in_building` -- building interaction (shop, bar, police, etc.)
- `lib_in_combat` -- turn-based combat
- `lib_in_dungeon` -- reuses Captive dungeon engine inside Liberation buildings

```
STATE_MENU --> liberation_intro --> liberation_mission_menu --> STATE_GAME (city)
                                                                   |
                                                          (enter building)
                                                                   |
                                                          lib_in_building
                                                                   |
                                                     (special building found)
                                                                   |
                                                          lib_in_dungeon
                                                                   |
                                                     (destroy generators)
                                                                   |
                                                     mission complete
```

## Captive Campaign

Captive is a 10-mission campaign. Each mission follows the same structure:

### Mission Generation

1. **Seed calculation**: `mission_seed = (mission - 1) * 11 + base_id`
2. **Dungeon generation**: procedural cellular automaton rules generate the dungeon layout.
   - Cell data uses a 5-byte format with `ca_segments` (5-bit wall segments) and `ca_thickness` (wall thickness codes: 0x10, 0x18, 0x80, 0xC0).
3. **Generator placement**: placed using the disassembled algorithm from address 0x1C3C in the original binary.
4. **Feature placement**: doors, puzzles, and traps placed via the pipeline from address 0x33D7.
5. **Level linking**: `STAIRS_UP` connections link levels; an exterior level is prepended as the base.
6. **Party placement**: the party starts at the first `CELL_FLOOR` or `CELL_STAIRS_UP` cell found.

### Mission Objective

Destroy all generators on all levels. The game tracks `generators_total` and `generators_destroyed`. When `generators_destroyed >= generators_total`, the mission is complete.

### Between Missions: Holamap

After completing a mission, `game_state_complete_mission` transitions to `STATE_HOLAMAP`:

- Displays the planet name for the next mission.
- Provides shop access for equipment upgrades.
- Advancing from Holamap starts the next mission.

### Victory

When mission 10 is completed (`mission >= 10`), the game transitions to `STATE_VICTORY`.

### Dungeon Parameters

- Map size: 64 x 32 cells per level (`MAP_WIDTH` x `MAP_HEIGHT`)
- Maximum levels per mission: 15 (`MAX_LEVELS`)
- Cell types: wall, floor, door, locked door, stairs up/down, shop, teleporter, generator, terminal, elevator, pressure plate, pit

## Liberation Campaign

Liberation uses a city-based structure with up to 256 missions.

### City Navigation

- 8 city themes cycling by `(mission - 1) % 8`: blue, desert, industrial, coastal, twilight, forest, arid, tundra.
- Day/night cycle: `hour = (tick / 60) % 24`. Day is 6:00-18:00, sunset 18:00-21:00, night otherwise.
- Random encounters: triggered when `(encounter_roll & 0x1F) == 0` after movement.

### Mission Structure

1. **Mission briefing**: PlotGen-driven briefing with city name, victim name, and news source.
2. **City exploration**: navigate the city, enter buildings.
3. **Special building**: investigating the special building triggers a dungeon crawl (reusing the Captive dungeon engine).
4. **Generator destruction**: destroying all generators in the dungeon completes the mission.

### Taxi System

- Activated via phone box interaction.
- Costs **50 gold**.
- Teleports the party directly to the special building (building type 8).

### Liberation Victory

Victory is achieved when `mission >= 256`.

### Industrial Hazards

Liberation dungeons apply environmental damage: `5 + mission * 2` per hazard encounter.

## Game Over

The game transitions to `STATE_GAMEOVER` when all 4 droids are destroyed (all have `hp <= 0`).

## Droid Configuration

`STATE_DROID_CONFIG` is entered before the first mission. It displays all 4 droids with options to:

- **R**: rename a droid
- **S**: swap weapons

Initial droid stats: HP 100, energy 100, XP 1024, weapon_damage 0x050A, all body parts at item ID 1 with condition 255, starting gold 100. Names are generated procedurally via `captive_generate_name` with seed 42.

## Source Files

- Main loop and state machine: `src/main.c`
- Game state and mission logic: `src/engine/engine.c`, `include/game_state.h`
