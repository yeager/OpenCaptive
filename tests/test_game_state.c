#include "game_state.h"
#include "combat.h"
#include "puzzle.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static int generator_count(const GameState *gs) {
    int count = 0;
    for (int level = 0; level < gs->num_levels; level++)
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                count += gs->levels[level].cells[y][x].type == CELL_GENERATOR;
    return count;
}

static void test_puzzle_rejects_invalid_level(void) {
    PuzzleList puzzles;
    DungeonLevel level;
    puzzle_init(&puzzles);
    memset(&level, 0, sizeof(level));
    puzzle_generate(&puzzles, &level, -1, 1);
    assert(puzzles.num_puzzles == 0);
    puzzle_generate(&puzzles, &level, MAX_LEVELS, 1);
    assert(puzzles.num_puzzles == 0);
    puzzles.num_puzzles = -1;
    puzzle_generate(&puzzles, &level, 0, 1);
    assert(puzzles.num_puzzles == -1);
}

static void test_button_combo_solution_is_reachable(void) {
    GameState gs;
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;

    Puzzle *combo = &puzzles.puzzles[0];
    *combo = (Puzzle){
        .type = PUZZLE_BUTTON_COMBO,
        .x = 4, .y = 4, .level = 0, .face = DIR_NORTH,
        .solution = (uint8_t)(1U << DIR_NORTH),
        .target_x = 5, .target_y = 5,
    };
    puzzles.num_puzzles = 1;
    gs.levels[0].cells[5][5].type = CELL_DOOR_LOCKED;

    assert(puzzle_interact(&puzzles, &gs, 4, 4, DIR_NORTH));
    assert(combo->state == combo->solution);
    assert(combo->solved);
    assert(gs.levels[0].cells[5][5].type == CELL_DOOR);
}

static void test_teleporter_never_enters_blocked_cell(void) {
    GameState gs;
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    puzzles.puzzles[0] = (Puzzle){
        .type = PUZZLE_TELEPORTER_TRAP, .x = 3, .y = 3, .level = 0,
        .target_x = 8, .target_y = 8
    };
    puzzles.num_puzzles = 1;
    gs.levels[0].cells[8][8].type = CELL_WALL;
    gs.party_x = 3;
    gs.party_y = 3;
    assert(puzzle_interact(&puzzles, &gs, 3, 3, DIR_NORTH));
    assert(gs.party_x == 3 && gs.party_y == 3);
    puzzle_check_step(&puzzles, &gs, 3, 3);
    assert(gs.party_x == 3 && gs.party_y == 3);

    gs.levels[0].cells[8][8].type = CELL_FLOOR;
    assert(puzzle_interact(&puzzles, &gs, 3, 3, DIR_NORTH));
    assert(gs.party_x == 8 && gs.party_y == 8);
}

static void test_generated_teleporters_target_floor(void) {
    GameState gs;
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    puzzle_generate(&puzzles, &gs.levels[5], 5, 0x12345678);
    for (int i = 0; i < puzzles.num_puzzles; i++) {
        const Puzzle *p = &puzzles.puzzles[i];
        if (p->type != PUZZLE_TELEPORTER_TRAP) continue;
        assert(p->target_x >= 0 && p->target_x < MAP_WIDTH);
        assert(p->target_y >= 0 && p->target_y < MAP_HEIGHT);
        assert(gs.levels[5].cells[p->target_y][p->target_x].type == CELL_FLOOR);
    }
}

static void test_generated_power_socket_has_no_stale_target(void) {
    DungeonLevel level;
    PuzzleList puzzles;
    memset(&level, 0, sizeof(level));
    memset(&puzzles, 0xA5, sizeof(puzzles));
    puzzles.num_puzzles = 0;
    for (int y = 1; y < MAP_HEIGHT - 1; y++)
        for (int x = 1; x < MAP_WIDTH - 1; x++)
            level.cells[y][x].type = CELL_FLOOR;
    for (int x = 0; x < MAP_WIDTH; x++) {
        level.cells[0][x].type = CELL_WALL;
        level.cells[MAP_HEIGHT - 1][x].type = CELL_WALL;
    }
    for (int y = 0; y < MAP_HEIGHT; y++) {
        level.cells[y][0].type = CELL_WALL;
        level.cells[y][MAP_WIDTH - 1].type = CELL_WALL;
    }
    for (int x = 8; x < MAP_WIDTH - 2; x += 8)
        for (int y = 2; y < MAP_HEIGHT - 2; y++)
            level.cells[y][x].type = CELL_WALL;

    bool found_socket = false;
    for (uint32_t seed = 1; seed <= 128 && !found_socket; seed++) {
        puzzles.num_puzzles = 0;
        puzzle_generate(&puzzles, &level, 2, seed);
        for (int i = 0; i < puzzles.num_puzzles; i++) {
            const Puzzle *p = &puzzles.puzzles[i];
            if (p->type == PUZZLE_POWER_SOCKET) {
                found_socket = true;
                assert(p->target_x == -1 && p->target_y == -1);
            }
        }
    }
    assert(found_socket);
}

static void test_generated_triple_levers_use_all_eight_states(void) {
    bool found_extended_solution = false;
    for (uint32_t seed = 1; seed <= 128 && !found_extended_solution; seed++) {
        DungeonLevel level;
        PuzzleList puzzles;
        memset(&level, 0, sizeof(level));
        memset(&puzzles, 0, sizeof(puzzles));
        for (int y = 1; y < MAP_HEIGHT - 1; y++)
            for (int x = 1; x < MAP_WIDTH - 1; x++)
                level.cells[y][x].type = CELL_FLOOR;
        for (int x = 3; x < MAP_WIDTH - 1; x += 4)
            for (int y = 1; y < MAP_HEIGHT - 1; y++)
                level.cells[y][x].type = CELL_WALL;

        puzzle_generate(&puzzles, &level, 0, seed);
        for (int i = 0; i < puzzles.num_puzzles; i++) {
            if (puzzles.puzzles[i].type == PUZZLE_TRIPLE_LEVER &&
                puzzles.puzzles[i].solution > 1U) {
                found_extended_solution = true;
                break;
            }
        }
    }
    assert(found_extended_solution);
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

static void test_combat_creatures_cannot_enter_party_tile(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 20, .hp_max = 20,
        .range = 0, .x = 2, .y = 1, .level = 0,
        .active = true, .alerted = true,
    };
    combat_tick(&creatures, &gs);
    assert(creatures.creatures[0].x == 2 && creatures.creatures[0].y == 1);
}

static void test_combat_gold_reward_saturates(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_EAST;
    gs.gold = INT_MAX;
    gs.droids[0].weapons[0] = 13;
    gs.droids[0].weapon_damage = 0xFFFF;

    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 1, .hp_max = 30,
        .x = 2, .y = 1, .active = true,
    };
    assert(combat_droid_attack(&gs, &creatures, 0));
    assert(gs.gold == INT_MAX);
}

static void test_combat_level_up_uses_pre_attack_xp(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_EAST;
    gs.droids[0].xp = 0;
    gs.droids[0].weapons[0] = 13;
    gs.droids[0].weapon_damage = 0xFFFF;

    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 1, .hp_max = 32760,
        .x = 2, .y = 1, .active = true,
    };
    assert(combat_droid_attack(&gs, &creatures, 0));
    assert(gs.droids[0].xp == 3276);
    assert(gs.droids[0].hp_max == 110);
}

static void test_combat_uses_ranged_hand_when_melee_is_first(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_EAST;
    gs.droids[0].weapons[0] = 13; /* melee */
    gs.droids[0].weapons[1] = 18; /* handgun */
    gs.droids[0].weapon_damage = 0x0505;

    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 30, .hp_max = 30,
        .x = 3, .y = 1, .level = 0, .active = true,
    };
    assert(combat_droid_attack(&gs, &creatures, 0));
}

static void test_combat_does_not_treat_non_weapon_as_ranged(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_EAST;
    gs.droids[0].weapons[0] = 1;  /* armour item, not a weapon */
    gs.droids[0].weapons[1] = 13; /* melee weapon */
    gs.droids[0].weapon_damage = 0x0505;

    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 30, .hp_max = 30,
        .x = 3, .y = 1, .level = 0, .active = true,
    };
    assert(!combat_droid_attack(&gs, &creatures, 0));
    assert(creatures.creatures[0].hp == 30);
    assert(gs.droids[0].energy == gs.droids[0].energy_max);
}

static void test_combat_requires_equipped_weapon(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_EAST;
    gs.droids[0].weapon_damage = 0xFFFF;

    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 30, .hp_max = 30,
        .x = 2, .y = 1, .level = 0, .active = true,
    };
    assert(!combat_droid_attack(&gs, &creatures, 0));
    assert(creatures.creatures[0].hp == 30);
    assert(gs.droids[0].energy == gs.droids[0].energy_max);
}

static void test_combat_skips_destroyed_droids(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    gs.droids[0].hp = 0;
    int living_hp[4];
    for (int i = 0; i < 4; i++) living_hp[i] = gs.droids[i].hp;
    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 20, .hp_max = 20,
        .damage_min = 4, .damage_max = 4, .range = 4,
        .x = 2, .y = 1, .level = 0, .active = true, .alerted = true,
    };
    combat_tick(&creatures, &gs);
    assert(gs.droids[0].hp == 0);
    bool one_living_droid_was_hit = false;
    for (int i = 1; i < 4; i++)
        if (gs.droids[i].hp < living_hp[i]) one_living_droid_was_hit = true;
    assert(one_living_droid_was_hit);
}

static void test_combat_ignores_invalid_creature_health(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_EAST;

    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 0, .hp_max = 0,
        .x = 2, .y = 1, .active = true,
    };
    assert(!combat_droid_attack(&gs, &creatures, 0));
    assert(gs.droids[0].energy == gs.droids[0].energy_max);
    int hp_before = gs.droids[0].hp;
    combat_tick(&creatures, &gs);
    assert(gs.droids[0].hp == hp_before);
    assert(!creatures.creatures[0].active);
}

static void test_combat_extreme_damage_does_not_wrap_hp(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 20, .hp_max = 20,
        .damage_min = INT16_MAX, .damage_max = INT16_MAX,
        .range = 4, .x = 2, .y = 1, .level = 0, .active = true,
        .alerted = true,
    };
    combat_tick(&creatures, &gs);
    int living_hp = 0;
    for (int i = 0; i < 4; i++)
        if (gs.droids[i].hp > 0) living_hp++;
    assert(living_hp == 3);
}

static void test_combat_does_not_heal_from_invalid_damage(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 10, .hp_max = 10,
        .damage_min = -100, .damage_max = -50, .range = 4,
        .x = 2, .y = 1, .level = 0, .active = true, .alerted = true
    };
    int hp_before = gs.droids[0].hp;
    combat_tick(&creatures, &gs);
    assert(gs.droids[0].hp <= hp_before);
}

static void test_combat_spawn_extreme_level_seed_is_defined(void) {
    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    CreatureList creatures;
    combat_init(&creatures);
    combat_spawn_for_level(&creatures, &gs.levels[0], INT_MIN, UINT32_MAX);
    assert(creatures.num_creatures == 0);
    combat_init(&creatures);
    combat_spawn_for_level(&creatures, &gs.levels[0], INT_MAX, 0);
    assert(creatures.num_creatures == 0);
}

static void test_combat_spawn_normalizes_negative_creature_count(void) {
    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);

    CreatureList creatures;
    combat_init(&creatures);
    creatures.num_creatures = -1;
    combat_spawn_for_level(&creatures, &gs.levels[0], 0, 1234);

    assert(creatures.num_creatures >= 0);
    assert(creatures.num_creatures <= MAX_CREATURES);
}

static void test_generator_counter_does_not_overflow(void) {
    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_EAST;
    gs.generators_destroyed = INT_MAX;
    gs.generators_total = INT_MAX;
    gs.levels[0].cells[1][2].type = CELL_GENERATOR;
    combat_interact(&gs, NULL);
    assert(gs.generators_destroyed == INT_MAX);
}

static void test_combat_rejects_invalid_position_state(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.party_dir = 99;
    combat_interact(&gs, NULL);
    assert(!combat_droid_attack(&gs, &creatures, 0));
    combat_tick(&creatures, &gs);
    gs.party_dir = DIR_NORTH;
    gs.current_level = MAX_LEVELS;
    combat_interact(&gs, NULL);
    assert(!combat_droid_attack(&gs, &creatures, 0));
}

static void test_first_mission_uses_architect_seed_zero(void) {
    static GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    assert(gs.base_id == 0);
    game_state_new_mission(&gs, 1);
    assert(gs.mission_seed == 0);
    assert(gs.num_levels > 0);
}

static void test_extreme_mission_seed_is_defined(void) {
    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    gs.base_id = 17;
    game_state_new_mission(&gs, INT_MAX);
    assert(gs.mission == INT_MAX);
    assert(gs.mission_seed == (uint32_t)((uint64_t)(INT_MAX - 1) * 11U + 17U));
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

static void test_mission_completion_rejects_overshot_generator_count(void) {
    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    gs.generators_total = 3;
    gs.generators_destroyed = 4;
    assert(!game_state_complete_mission(&gs));
    assert(gs.mode == STATE_MENU);
}

static void test_floor_change_rejects_corrupt_level_count(void) {
    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    gs.num_levels = MAX_LEVELS + 1;
    gs.current_level = 0;
    assert(!game_state_change_floor(&gs, 1));
}

static void test_generated_floor_connections(void) {
    for (uint32_t seed = 0; seed < 64; seed++) {
        GameState gs;
        game_state_init(&gs, GAME_CAPTIVE, 1);
        game_state_new_mission_seeded(&gs, 1, seed);
        assert(gs.mission_seed == seed);
        for (int level = 0; level + 1 < gs.num_levels; level++) {
            int down_x = -1, down_y = -1;
            for (int y = 0; y < MAP_HEIGHT; y++) {
                for (int x = 0; x < MAP_WIDTH; x++) {
                    if (gs.levels[level].cells[y][x].type == CELL_STAIRS_DOWN) {
                        down_x = x;
                        down_y = y;
                        break;
                    }
                }
                if (down_x >= 0) break;
            }
            assert(down_x >= 0 && down_y >= 0);
            gs.current_level = level;
            gs.party_x = down_x;
            gs.party_y = down_y;
            assert(game_state_change_floor(&gs, 1));
            assert(gs.current_level == level + 1);
            assert(game_state_change_floor(&gs, -1));
            assert(gs.current_level == level);
        }
    }
}

static void test_combat_los_rejects_corrupt_level_count(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.num_levels = MAX_LEVELS + 1;
    gs.current_level = MAX_LEVELS;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_EAST;
    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 10, .hp_max = 10,
        .x = 2, .y = 1, .level = MAX_LEVELS, .active = true
    };
    assert(!combat_droid_attack(&gs, &creatures, 0));
}

static void test_init_normalizes_invalid_mission(void) {
    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 0);
    assert(gs.mission == 1);
    game_state_init(&gs, GAME_CAPTIVE, -7);
    assert(gs.mission == 1);
}

int main(void) {
    test_init_normalizes_invalid_mission();
    test_puzzle_rejects_invalid_level();
    test_button_combo_solution_is_reachable();
    test_teleporter_never_enters_blocked_cell();
    test_generated_teleporters_target_floor();
    test_generated_power_socket_has_no_stale_target();
    test_generated_triple_levers_use_all_eight_states();
    test_first_mission_uses_architect_seed_zero();
    test_extreme_mission_seed_is_defined();
    test_combat_respects_closed_doors();
    test_combat_creatures_cannot_enter_party_tile();
    test_combat_gold_reward_saturates();
    test_combat_level_up_uses_pre_attack_xp();
    test_combat_uses_ranged_hand_when_melee_is_first();
    test_combat_does_not_treat_non_weapon_as_ranged();
    test_combat_requires_equipped_weapon();
    test_combat_skips_destroyed_droids();
    test_combat_ignores_invalid_creature_health();
    test_combat_extreme_damage_does_not_wrap_hp();
    test_combat_does_not_heal_from_invalid_damage();
    test_combat_spawn_extreme_level_seed_is_defined();
    test_combat_spawn_normalizes_negative_creature_count();
    test_generator_counter_does_not_overflow();
    test_combat_rejects_invalid_position_state();
    test_campaign_progression();
    test_mission_completion_rejects_overshot_generator_count();
    test_floor_change_rejects_corrupt_level_count();
    test_generated_floor_connections();
    test_combat_los_rejects_corrupt_level_count();
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
