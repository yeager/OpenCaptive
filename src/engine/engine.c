#include "game_state.h"
#include "captive_data.h"
#include "map_gen.h"
#include <stdlib.h>
#include <string.h>

void game_state_init(GameState *gs, GameType type, int mission) {
    if (!gs) return;
    if (mission < 1) mission = 1;
    memset(gs, 0, sizeof(*gs));
    gs->game_type = type;
    gs->mode = STATE_MENU;
    gs->mission = mission;
    gs->base_id = 0;
    gs->selected_droid = 0;
    gs->party_dir = DIR_NORTH;
    gs->gold = 100;

    uint32_t name_seed = 0;
    for (int i = 0; i < 4; i++) {
        captive_generate_name(&name_seed, gs->droids[i].name,
                              sizeof(gs->droids[i].name));
        gs->droids[i].hp = 100;
        gs->droids[i].hp_max = 100;
        gs->droids[i].energy = 100;
        gs->droids[i].energy_max = 100;
        gs->droids[i].xp = 1024;
        /* Every fresh droid starts with the basic KNUCKLE-DUSTER (item 13).
         * Its CAPPO damage bytes are 04/21, stored low byte first.  Keeping
         * the item ID and cached damage together makes the initial state
         * valid for both Captive and Liberation combat. */
        gs->droids[i].weapons[0] = 13;
        gs->droids[i].weapon_damage = 0x2104;
        for (int p = 0; p < 6; p++) {
            gs->droids[i].body_parts[p] = (uint8_t)(p + 1);
            gs->droids[i].body_part_hp[p] = 255;
        }
    }
}

bool game_state_new_mission(GameState *gs, int mission) {
    if (!gs) return false;
    if (mission < 1) mission = 1;
    uint32_t seed = (uint32_t)((uint64_t)(mission - 1) * UINT64_C(11) +
                               (uint64_t)(int64_t)gs->base_id);
    /* Let the seeded implementation commit mission metadata only after its
     * temporary dungeon allocation succeeds.  Updating these fields here
     * would reintroduce a partially changed state on allocation failure. */
    return game_state_new_mission_seeded(gs, mission, seed);
}

bool game_state_new_mission_seeded(GameState *gs, int mission, uint32_t seed) {
    if (!gs) return false;
    if (mission < 1) mission = 1;

    // Architect creates one flattened 64x32 base whose 16x8 sections are
    // connected by stairs and elevators.  map_generate_base exposes those
    // logical floors through levels[] for the current game-state API.
    int base_levels = 0;
    /* map_generate_base() may legally emit MAX_LEVELS logical floors even
       though only MAX_LEVELS - 1 can be copied after the exterior level. */
    /* A DungeonLevel contains the complete 64x32 map.  Keeping all possible
       base levels automatic here overflows the default Windows thread stack
       even though the caller only needs the copied levels below. */
    DungeonLevel *base_buf = calloc(MAX_LEVELS, sizeof(*base_buf));
    if (!base_buf) return false;
    map_generate_base(base_buf, &base_levels, seed);

    /* Do not partially replace the active mission before the temporary map
       storage exists.  If allocation fails, callers must retain a coherent
       old game state rather than seeing the new mission header over its old
       dungeon. */
    gs->mission = mission;
    gs->mission_seed = seed;
    gs->current_level = 0;

    /* Find base entrance x on floor 0 row 0 */
    int entrance_x = MAP_WIDTH / 2;
    for (int x = 0; x < MAP_WIDTH; x++) {
        if (base_buf[0].cells[0][x].type != CELL_WALL) {
            entrance_x = x;
            break;
        }
    }

    /* Exterior landing zone is level 0; base levels follow */
    map_generate_exterior(&gs->levels[0], entrance_x, gs->mission_seed);
    for (int i = 0; i < base_levels && i + 1 < MAX_LEVELS; i++)
        gs->levels[i + 1] = base_buf[i];
    free(base_buf);
    gs->num_levels = (base_levels + 1 <= MAX_LEVELS) ? base_levels + 1 : MAX_LEVELS;

    /* Place a STAIRS_UP at the base entrance so the exterior connects back */
    if (gs->num_levels > 1) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (gs->levels[1].cells[0][x].type == CELL_FLOOR) {
                gs->levels[1].cells[0][x].type = CELL_STAIRS_UP;
                break;
            }
        }
    }

    gs->generators_total = 0;
    for (int i = 0; i < gs->num_levels; i++) {
        // Count generators
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                if (gs->levels[i].cells[y][x].type == CELL_GENERATOR)
                    gs->generators_total++;
    }
    gs->generators_destroyed = 0;

    // Place party at stairs up of level 0
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (gs->levels[0].cells[y][x].type == CELL_FLOOR ||
                gs->levels[0].cells[y][x].type == CELL_STAIRS_UP) {
                gs->party_x = x;
                gs->party_y = y;
                goto placed;
            }
        }
    }
    placed:
    gs->party_dir = DIR_NORTH;
    gs->mode = STATE_GAME;
    return true;
}

bool game_state_change_floor(GameState *gs, int direction) {
    if (!gs || (direction != -1 && direction != 1)) return false;
    if (gs->num_levels < 1 || gs->num_levels > MAX_LEVELS ||
        gs->current_level < 0 || gs->current_level >= gs->num_levels ||
        gs->current_level >= MAX_LEVELS ||
        gs->party_x < 0 || gs->party_x >= MAP_WIDTH ||
        gs->party_y < 0 || gs->party_y >= MAP_HEIGHT) return false;

    const DungeonLevel *current = &gs->levels[gs->current_level];
    CellType required = direction > 0 ? CELL_STAIRS_DOWN : CELL_STAIRS_UP;
    if (current->cells[gs->party_y][gs->party_x].type != required) return false;

    int destination = gs->current_level + direction;
    if (destination < 0 || destination >= gs->num_levels) return false;
    CellType arrival = direction > 0 ? CELL_STAIRS_UP : CELL_STAIRS_DOWN;
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (gs->levels[destination].cells[y][x].type == arrival) {
                gs->current_level = destination;
                gs->party_x = x;
                gs->party_y = y;
                return true;
            }
        }
    }
    return false;
}

bool game_state_complete_mission(GameState *gs) {
    if (!gs || gs->game_type != GAME_CAPTIVE || gs->generators_total <= 0 ||
        gs->generators_destroyed != gs->generators_total)
        return false;

    if (gs->mission >= 10) {
        gs->mode = STATE_VICTORY;
    } else {
        gs->mode = STATE_HOLAMAP;
    }
    return true;
}
