#include "game_state.h"
#include "combat.h"
#include <assert.h>
#include <stdio.h>

static int generator_count(const GameState *gs) {
    int count = 0;
    for (int level = 0; level < gs->num_levels; level++)
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                count += gs->levels[level].cells[y][x].type == CELL_GENERATOR;
    return count;
}

static void move_to_stair(GameState *gs, CellType stair) {
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            if (gs->levels[gs->current_level].cells[y][x].type == stair) {
                gs->party_x = x;
                gs->party_y = y;
                return;
            }
    assert(!"expected stair is missing");
}

static void test_combat_respects_closed_doors(void) {
    static GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    gs.base_id = 3;
    game_state_new_mission(&gs, 17);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_EAST;
    gs.droids[0].weapons[0] = 18;
    gs.droids[0].weapon_damage = 0x0505;

    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 30, .hp_max = 30,
        .defense = 0, .range = 4, .x = 3, .y = 1,
        .level = 0, .active = true,
    };
    assert(combat_droid_attack(&gs, &creatures, 0));

    creatures.creatures[0].hp = 30;
    gs.levels[0].cells[1][2].type = CELL_DOOR;
    assert(!combat_droid_attack(&gs, &creatures, 0));

    int hp_before = gs.droids[0].hp;
    creatures.creatures[0].alerted = true;
    creatures.creatures[0].cooldown = 0;
    combat_tick(&creatures, &gs);
    assert(gs.droids[0].hp == hp_before);
}

static void test_first_mission_uses_architect_seed_zero(void) {
    static GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    assert(gs.base_id == 0);
    game_state_new_mission(&gs, 1);
    assert(gs.mission_seed == 0);
    assert(gs.num_levels > 0);
}

static void test_campaign_progression(void) {
    static GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    gs.base_id = 3;
    game_state_new_mission(&gs, 9);
    assert(!game_state_complete_mission(&gs));
    gs.generators_destroyed = gs.generators_total;
    assert(game_state_complete_mission(&gs));
    assert(gs.mission == 9 && gs.mode == STATE_HOLAMAP);

    game_state_new_mission(&gs, 10);
    gs.generators_destroyed = gs.generators_total;
    assert(game_state_complete_mission(&gs));
    assert(gs.mode == STATE_VICTORY);
}

int main(void) {
    test_first_mission_uses_architect_seed_zero();
    test_combat_respects_closed_doors();
    test_campaign_progression();
    static GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    gs.base_id = 3;
    game_state_new_mission(&gs, 17);

    assert(gs.mission_seed == 179);
    assert(gs.num_levels >= 3 && gs.num_levels <= 6);
    assert(gs.generators_total == generator_count(&gs));
    assert(gs.generators_total > 0);
    /* Level 0 is now the exterior landing zone; party starts at y=1 */
    assert(gs.party_y >= 1);

    /* Interacting with a generator is the only action that increments the
     * objective counter.  Position it in front of the party to test the
     * gameplay path, rather than mutating the counter directly. */
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_EAST;
    gs.levels[0].cells[1][2].type = CELL_GENERATOR;
    int before = gs.generators_destroyed;
    combat_interact(&gs, NULL);
    assert(gs.generators_destroyed == before + 1);
    assert(gs.levels[0].cells[1][2].type == CELL_FLOOR);

    /* Doors are barriers until the interaction path opens them. */
    gs.levels[0].cells[1][2].type = CELL_DOOR;
    combat_interact(&gs, NULL);
    assert(gs.levels[0].cells[1][2].type == CELL_FLOOR);

    int initial_level = gs.current_level;
    move_to_stair(&gs, CELL_STAIRS_DOWN);
    assert(game_state_change_floor(&gs, 1));
    assert(gs.current_level == initial_level + 1);
    assert(gs.levels[gs.current_level].cells[gs.party_y][gs.party_x].type == CELL_STAIRS_UP);
    assert(game_state_change_floor(&gs, -1));
    assert(gs.current_level == initial_level);
    assert(!game_state_change_floor(&gs, -1));

    puts("All game state tests passed");
    return 0;
}
