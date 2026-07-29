#include "game_state.h"
#include "map_gen.h"
#include <string.h>

static const char *droid_names[4] = { "DORIS", "DONNA", "DOLLY", "DIANA" };

void game_state_init(GameState *gs, GameType type, int mission) {
    memset(gs, 0, sizeof(*gs));
    gs->game_type = type;
    gs->mode = STATE_MENU;
    gs->mission = mission;
    /* Architect's first base is map/seed zero.  Its sparse section mask and
     * player column are explicit original special cases, handled by
     * map_generate_base(). */
    gs->base_id = 0;
    gs->selected_droid = 0;
    gs->party_dir = DIR_NORTH;
    gs->gold = 100;

    for (int i = 0; i < 4; i++) {
        strncpy(gs->droids[i].name, droid_names[i], 15);
        gs->droids[i].hp = 100;
        gs->droids[i].hp_max = 100;
        gs->droids[i].energy = 100;
        gs->droids[i].energy_max = 100;
        gs->droids[i].level = 1;
        for (int p = 0; p < 6; p++)
            gs->droids[i].body_parts[p] = 1; // basic parts
    }
}

void game_state_new_mission(GameState *gs, int mission) {
    gs->mission = mission;
    gs->mission_seed = ((mission - 1) * 11) + gs->base_id;
    gs->current_level = 0;

    // Architect creates one flattened 64x32 base whose 16x8 sections are
    // connected by stairs and elevators.  map_generate_base exposes those
    // logical floors through levels[] for the current game-state API.
    map_generate_base(gs->levels, &gs->num_levels, gs->mission_seed);

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
}

bool game_state_change_floor(GameState *gs, int direction) {
    if (!gs || (direction != -1 && direction != 1)) return false;
    if (gs->current_level < 0 || gs->current_level >= gs->num_levels ||
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
        gs->generators_destroyed < gs->generators_total)
        return false;

    if (gs->mission >= 10) {
        gs->mode = STATE_VICTORY;
    } else {
        game_state_new_mission(gs, gs->mission + 1);
    }
    return true;
}
