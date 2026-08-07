#include "game_state.h"
#include "combat.h"
#include "puzzle.h"
#include "xp_level.h"
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

static void test_new_mission_rejects_null_state(void) {
    assert(!game_state_new_mission(NULL, 1));
    assert(!game_state_new_mission_seeded(NULL, 1, 0));
}

static void test_generated_missions_link_puzzles_to_locked_doors(void) {
    for (uint32_t seed = 0; seed < 32; seed++) {
        GameState gs;
        PuzzleList puzzles;
        game_state_init(&gs, GAME_CAPTIVE, 1);
        assert(game_state_new_mission_seeded(&gs, 1, seed));

        /* Level zero is the exterior landing zone and intentionally has no
         * locked base door; Architect floors begin at level one. */
        for (int level = 1; level < gs.num_levels; level++) {
            int locked_doors = 0;
            for (int y = 0; y < MAP_HEIGHT; y++)
                for (int x = 0; x < MAP_WIDTH; x++)
                    locked_doors += gs.levels[level].cells[y][x].type ==
                                    CELL_DOOR_LOCKED;
            assert(locked_doors > 0);

            puzzle_init(&puzzles);
            puzzle_generate(&puzzles, &gs.levels[level], level, seed);
            bool linked = false;
            for (int i = 0; i < puzzles.num_puzzles; i++) {
                const Puzzle *p = &puzzles.puzzles[i];
                if (p->target_x < 0 || p->target_y < 0) continue;
                assert(p->target_x < MAP_WIDTH && p->target_y < MAP_HEIGHT);
                if (p->type == PUZZLE_LEVER || p->type == PUZZLE_TRIPLE_LEVER ||
                    p->type == PUZZLE_BARS || p->type == PUZZLE_BUTTON_COMBO ||
                    p->type == PUZZLE_HIDDEN_BUTTON) {
                    assert(gs.levels[level].cells[p->target_y][p->target_x].type ==
                           CELL_DOOR_LOCKED);
                    linked = true;
                }
            }
            assert(linked);
        }
    }
}

static bool captive_test_cell_reachable(const DungeonLevel *level,
                                        bool opened[MAP_HEIGHT][MAP_WIDTH],
                                        int x, int y) {
    return x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT &&
           level->cells[y][x].type != CELL_WALL &&
           (level->cells[y][x].type != CELL_DOOR_LOCKED || opened[y][x]);
}

static void captive_test_reachability(const DungeonLevel *level,
                                      bool opened[MAP_HEIGHT][MAP_WIDTH],
                                      int start_x, int start_y,
                                      bool reachable[MAP_HEIGHT][MAP_WIDTH]) {
    int queue[MAP_HEIGHT * MAP_WIDTH];
    int head = 0, tail = 0;
    memset(reachable, 0, sizeof(bool) * MAP_HEIGHT * MAP_WIDTH);
    if (!captive_test_cell_reachable(level, opened, start_x, start_y)) return;
    reachable[start_y][start_x] = true;
    queue[tail++] = start_y * MAP_WIDTH + start_x;
    while (head < tail) {
        int pos = queue[head++];
        int x = pos % MAP_WIDTH;
        int y = pos / MAP_WIDTH;
        static const int dx[] = {0, 1, 0, -1};
        static const int dy[] = {-1, 0, 1, 0};
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (!captive_test_cell_reachable(level, opened, nx, ny) ||
                reachable[ny][nx]) continue;
            reachable[ny][nx] = true;
            queue[tail++] = ny * MAP_WIDTH + nx;
        }
    }
}

static void test_generated_mission_progression_reaches_generators(void) {
    for (uint32_t seed = 0; seed < 256; seed++) {
        GameState gs;
        PuzzleList puzzles;
        game_state_init(&gs, GAME_CAPTIVE, 1);
        assert(game_state_new_mission_seeded(&gs, 1, seed));
        puzzle_init(&puzzles);
        for (int level = 0; level < gs.num_levels; level++)
            puzzle_generate(&puzzles, &gs.levels[level], level, seed);

        for (int level = 0; level < gs.num_levels; level++) {
            bool opened[MAP_HEIGHT][MAP_WIDTH] = {{false}};
            bool reachable[MAP_HEIGHT][MAP_WIDTH];
            int start_x = gs.party_x;
            int start_y = gs.party_y;
            if (level > 0) {
                start_x = start_y = -1;
                for (int y = 0; y < MAP_HEIGHT && start_x < 0; y++)
                    for (int x = 0; x < MAP_WIDTH; x++)
                        if (gs.levels[level].cells[y][x].type == CELL_STAIRS_UP) {
                            start_x = x;
                            start_y = y;
                            break;
                        }
                assert(start_x >= 0 && start_y >= 0);
            }

            bool changed;
            do {
                changed = false;
                captive_test_reachability(&gs.levels[level], opened,
                                           start_x, start_y, reachable);
                for (int i = 0; i < puzzles.num_puzzles; i++) {
                    const Puzzle *p = &puzzles.puzzles[i];
                    if (p->level != level || p->target_x < 0 ||
                        p->target_y < 0 || !reachable[p->y][p->x]) continue;
                    if (gs.levels[level].cells[p->target_y][p->target_x].type ==
                            CELL_DOOR_LOCKED &&
                        !opened[p->target_y][p->target_x]) {
                        opened[p->target_y][p->target_x] = true;
                        changed = true;
                    }
                }
            } while (changed);

            captive_test_reachability(&gs.levels[level], opened,
                                      start_x, start_y, reachable);
            for (int y = 0; y < MAP_HEIGHT; y++)
                for (int x = 0; x < MAP_WIDTH; x++)
                    if (gs.levels[level].cells[y][x].type == CELL_GENERATOR)
                        assert(reachable[y][x]);
            for (int i = 0; i < puzzles.num_puzzles; i++)
                if (puzzles.puzzles[i].level == level)
                    assert(reachable[puzzles.puzzles[i].y][puzzles.puzzles[i].x]);
        }
    }
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
    game_state_new_mission(&gs, 3);
    assert(gs.num_levels >= 2);
    int lvl = gs.num_levels - 1;
    puzzle_generate(&puzzles, &gs.levels[lvl], lvl, 0x12345678);
    for (int i = 0; i < puzzles.num_puzzles; i++) {
        const Puzzle *p = &puzzles.puzzles[i];
        if (p->type != PUZZLE_TELEPORTER_TRAP) continue;
        assert(p->target_x >= 0 && p->target_x < MAP_WIDTH);
        assert(p->target_y >= 0 && p->target_y < MAP_HEIGHT);
        assert(gs.levels[lvl].cells[p->target_y][p->target_x].type == CELL_FLOOR);
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

static void test_generated_electric_traps_face_walls(void) {
    bool found_electric = false;
    for (uint32_t seed = 1; seed <= 128 && !found_electric; seed++) {
        DungeonLevel level;
        PuzzleList puzzles;
        memset(&level, 0, sizeof(level));
        memset(&puzzles, 0, sizeof(puzzles));
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

        for (int x = 6; x < MAP_WIDTH - 2; x += 6)
            for (int y = 2; y < MAP_HEIGHT - 2; y++)
                level.cells[y][x].type = CELL_WALL;

        puzzle_generate(&puzzles, &level, 0, seed);
        for (int i = 0; i < puzzles.num_puzzles; i++) {
            const Puzzle *p = &puzzles.puzzles[i];
            if (p->type != PUZZLE_WALL_ELECTRIC) continue;
            assert(p->face >= DIR_NORTH && p->face <= DIR_WEST);
            static const int dx[] = {0, 1, 0, -1};
            static const int dy[] = {-1, 0, 1, 0};
            assert(level.cells[p->y + dy[p->face]][p->x + dx[p->face]].type == CELL_WALL);
            found_electric = true;
        }
    }
    assert(found_electric);
}

static void test_fallback_door_control_remains_visible(void) {
    DungeonLevel level;
    PuzzleList puzzles;
    memset(&level, 0, sizeof(level));
    memset(&puzzles, 0, sizeof(puzzles));

    /* A completely open map has no reachable floor cell next to a wall, so
     * puzzle_generate() must use its last-resort linked-door control. */
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            level.cells[y][x].type = CELL_FLOOR;
    level.cells[10][10].type = CELL_DOOR_LOCKED;

    puzzle_generate(&puzzles, &level, 0, 17);
    bool found = false;
    for (int i = 0; i < puzzles.num_puzzles; i++) {
        const Puzzle *p = &puzzles.puzzles[i];
        if (p->type != PUZZLE_LEVER || p->target_x != 10 ||
            p->target_y != 10) continue;
        assert(p->face == DIR_NORTH);
        assert(level.cells[p->y - 1][p->x].type == CELL_FLOOR);
        assert(level.cells[p->y][p->x].ornament[DIR_NORTH] == ORNAMENT_PANEL);
        found = true;
        break;
    }
    assert(found);
}

static void test_generated_buttons_face_walls(void) {
    static const int dx[] = {0, 1, 0, -1};
    static const int dy[] = {-1, 0, 1, 0};
    bool found_button = false;
    for (uint32_t seed = 1; seed <= 128; seed++) {
        DungeonLevel level;
        PuzzleList puzzles;
        memset(&level, 0, sizeof(level));
        memset(&puzzles, 0, sizeof(puzzles));
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
        for (int y = 3; y < MAP_HEIGHT - 3; y += 4) {
            level.cells[y][8].type = CELL_WALL;
            level.cells[y][10].type = CELL_DOOR_LOCKED;
        }

        puzzle_generate(&puzzles, &level, 0, seed);
        for (int i = 0; i < puzzles.num_puzzles; i++) {
            const Puzzle *p = &puzzles.puzzles[i];
            if (p->type != PUZZLE_BUTTON) continue;
            assert(p->face >= DIR_NORTH && p->face <= DIR_WEST);
            assert(level.cells[p->y + dy[p->face]][p->x + dx[p->face]].type == CELL_WALL);
            found_button = true;
        }
    }
    assert(found_button);
}

static void test_generated_puzzles_do_not_overlap(void) {
    for (uint32_t seed = 1; seed <= 64; seed++) {
        DungeonLevel level;
        PuzzleList puzzles;
        memset(&level, 0, sizeof(level));
        memset(&puzzles, 0, sizeof(puzzles));
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
        puzzle_generate(&puzzles, &level, 5, seed);
        for (int i = 0; i < puzzles.num_puzzles; i++)
            for (int j = i + 1; j < puzzles.num_puzzles; j++)
                assert(puzzles.puzzles[i].level != puzzles.puzzles[j].level ||
                       puzzles.puzzles[i].x != puzzles.puzzles[j].x ||
                       puzzles.puzzles[i].y != puzzles.puzzles[j].y);
    }
}

static void test_lethal_puzzle_hazards_enter_game_over(void) {
    GameState gs;
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.selected_droid = 0;
    gs.droids[0].hp = 1;
    for (int i = 1; i < 4; i++) gs.droids[i].hp = 0;

    puzzles.puzzles[0] = (Puzzle){
        .type = PUZZLE_FLOOR_TRAP, .x = 2, .y = 2,
        .level = gs.current_level, .face = DIR_NORTH, .state = 10
    };
    puzzles.num_puzzles = 1;
    assert(puzzle_interact(&puzzles, &gs, 2, 2, DIR_NORTH));
    assert(gs.droids[0].hp == 0);
    assert(gs.mode == STATE_GAMEOVER);

    gs.mode = STATE_GAME;
    gs.droids[0].hp = 1;
    puzzles.puzzles[0] = (Puzzle){
        .type = PUZZLE_FLOOR_TRAP, .x = 3, .y = 3,
        .level = gs.current_level, .state = 10
    };
    puzzle_check_step(&puzzles, &gs, 3, 3);
    assert(gs.droids[0].hp == 0);
    assert(gs.mode == STATE_GAMEOVER);

    gs.mode = STATE_GAME;
    for (int i = 0; i < 4; i++) gs.droids[i].hp = 1;
    puzzles.puzzles[0] = (Puzzle){
        .type = PUZZLE_WALL_ELECTRIC, .x = 4, .y = 4,
        .level = gs.current_level, .face = DIR_NORTH
    };
    assert(puzzle_interact(&puzzles, &gs, 4, 4, DIR_NORTH));
    assert(gs.mode == STATE_GAMEOVER);
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

static void test_combat_spawn_never_overlaps_active_creatures(void) {
    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    for (uint32_t seed = 0; seed < 256; seed++) {
        CreatureList creatures;
        combat_init(&creatures);
        combat_spawn_for_level(&creatures, &gs.levels[0], 0, seed);
        for (int i = 0; i < creatures.num_creatures; i++) {
            const Creature *a = &creatures.creatures[i];
            if (!a->active) continue;
            for (int j = i + 1; j < creatures.num_creatures; j++) {
                const Creature *b = &creatures.creatures[j];
                assert(!b->active || a->level != b->level ||
                       a->x != b->x || a->y != b->y);
            }
        }
    }
}

static void test_combat_spawn_density_advances_by_two_groups_per_level(void) {
    DungeonLevel level;
    memset(&level, 0, sizeof(level));
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            level.cells[y][x].type = CELL_FLOOR;

    int level_zero_total = 0;
    int level_one_total = 0;
    for (uint32_t seed = 0; seed < 128; seed++) {
        CreatureList low;
        CreatureList high;
        combat_init(&low);
        combat_init(&high);
        combat_spawn_for_level(&low, &level, 0, seed);
        combat_spawn_for_level(&high, &level, 1, seed);
        level_zero_total += low.num_creatures;
        level_one_total += high.num_creatures;
    }
    assert(level_one_total > level_zero_total);
}

static void test_combat_spawn_avoids_party_cell(void) {
    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;

    for (uint32_t seed = 0; seed < 512; seed++) {
        CreatureList creatures;
        combat_init(&creatures);
        combat_spawn_for_level_avoiding_party(&creatures, &gs.levels[0], 0,
                                              seed, &gs);
        assert(!combat_cell_occupied(&creatures, gs.current_level,
                                     gs.party_x, gs.party_y));
    }
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

static void test_combat_line_of_sight_respects_blocked_corners(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_SOUTH;
    gs.droids[0].weapons[0] = 18;
    gs.droids[0].weapon_damage = 0x0505;
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;
    gs.levels[0].cells[1][2].type = CELL_WALL;
    gs.levels[0].cells[2][1].type = CELL_WALL;

    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 30, .hp_max = 30,
        .x = 2, .y = 2, .level = 0, .active = true,
    };
    assert(!combat_droid_attack(&gs, &creatures, 0));
    assert(creatures.creatures[0].hp == 30);
    assert(gs.droids[0].energy == gs.droids[0].energy_max);
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
    assert(combat_cell_occupied(&creatures, 0, 2, 1));
    assert(!combat_cell_occupied(&creatures, 1, 2, 1));
    combat_tick(&creatures, &gs);
    assert(creatures.creatures[0].x == 2 && creatures.creatures[0].y == 1);
}

static void test_combat_creatures_move_cardinally(void) {
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
        .range = 0, .x = 3, .y = 3, .level = 0,
        .active = true, .alerted = true,
    };
    combat_tick(&creatures, &gs);
    assert(creatures.creatures[0].x == 2 && creatures.creatures[0].y == 3);
}

static void test_combat_creature_collision_ignores_other_levels(void) {
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

    creatures.num_creatures = 2;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 20, .hp_max = 20,
        .range = 0, .x = 3, .y = 3, .level = 0,
        .active = true, .alerted = true,
    };
    creatures.creatures[1] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 20, .hp_max = 20,
        .x = 2, .y = 3, .level = 1, .active = true,
    };
    combat_tick(&creatures, &gs);
    assert(creatures.creatures[0].x == 2 && creatures.creatures[0].y == 3);
}

static void test_combat_respawn_timer_runs_on_other_levels(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 0, .hp_max = 20,
        .x = 3, .y = 3, .level = 1, .active = false,
        .respawn_timer = 1,
    };

    combat_tick(&creatures, &gs);
    assert(creatures.creatures[0].active);
    assert(creatures.creatures[0].hp == 20);
    assert(creatures.creatures[0].respawn_timer == 0);
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
    gs.droids[0].xp = 900;
    gs.droids[0].skill_levels[XP_SKILL_COUNT - 1] = 1;
    gs.droids[0].weapons[0] = 13;
    gs.droids[0].weapon_damage = 0xFFFF;

    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 1, .hp_max = 32760, .speed = 30,
        .x = 2, .y = 1, .active = true,
    };
    assert(combat_droid_attack(&gs, &creatures, 0));
    assert(gs.droids[0].xp == 1296);
    assert(gs.droids[0].hp_max == 110);
}

static void test_combat_attack_events_are_per_attack(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    assert(game_state_new_mission(&gs, 1));
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_EAST;
    gs.droids[0].weapons[0] = 13;
    gs.droids[0].weapon_damage = 0x0505;
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 1, .hp_max = 1,
        .x = 2, .y = 1, .level = 0, .active = true,
    };
    assert(combat_droid_attack(&gs, &creatures, 0));
    assert(creatures.creature_killed);

    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 300, .hp_max = 300,
        .x = 2, .y = 1, .level = 0, .active = true,
    };
    assert(combat_droid_attack(&gs, &creatures, 0));
    assert(!creatures.creature_killed);
    assert(!creatures.level_up_occurred);
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

static void test_combat_respects_spray_weapon_range(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    assert(game_state_new_mission(&gs, 1));
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_EAST;
    gs.droids[0].weapons[0] = 33; /* spray weapon: range 4 */
    gs.droids[0].weapons[1] = 0;
    gs.droids[0].weapon_damage = 0x0505;

    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 30, .hp_max = 30,
        .x = 6, .y = 1, .level = 0, .active = true,
    };
    assert(!combat_droid_attack(&gs, &creatures, 0));
    assert(creatures.creatures[0].hp == 30);
    assert(gs.droids[0].energy == gs.droids[0].energy_max);
}

static void test_combat_melee_rejects_diagonal_target(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    assert(game_state_new_mission(&gs, 1));
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_EAST;
    gs.droids[0].weapons[0] = 13; /* melee */
    gs.droids[0].weapon_damage = 0x0505;

    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;

    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 30, .hp_max = 30,
        .x = 2, .y = 2, .level = 0, .active = true,
    };
    assert(!combat_droid_attack(&gs, &creatures, 0));
    assert(creatures.creatures[0].hp == 30);
    assert(gs.droids[0].energy == gs.droids[0].energy_max);
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
    gs.droids[0].weapons[0] = 0;
    gs.droids[0].weapons[1] = 0;
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

static void test_combat_tick_enters_game_over_on_last_droid(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;
    for (int i = 0; i < 4; ++i) gs.droids[i].hp = 1;
    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 20, .hp_max = 20,
        .damage_min = 10, .damage_max = 10, .range = 4,
        .speed = 0, .x = 2, .y = 1, .level = 0, .active = true,
        .alerted = true,
    };

    for (int i = 0; i < 4; ++i) {
        creatures.creatures[0].cooldown = 0;
        combat_tick(&creatures, &gs);
    }
    assert(gs.mode == STATE_GAMEOVER);
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

static void test_completed_captive_mission_enters_holomap(void) {
    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 9);
    assert(game_state_new_mission(&gs, 9));
    gs.generators_destroyed = gs.generators_total;
    assert(game_state_complete_mission(&gs));
    assert(gs.mode == STATE_HOLAMAP);
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

static void test_floor_arrival_rejects_active_creature(void) {
    GameState gs;
    CreatureList creatures = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission_seeded(&gs, 1, 17);

    int down_x = -1, down_y = -1;
    for (int y = 0; y < MAP_HEIGHT && down_x < 0; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (gs.levels[0].cells[y][x].type == CELL_STAIRS_DOWN) {
                down_x = x;
                down_y = y;
                break;
            }
        }
    }
    assert(down_x >= 0 && down_y >= 0);
    int arrival_x = -1, arrival_y = -1;
    for (int y = 0; y < MAP_HEIGHT && arrival_x < 0; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (gs.levels[1].cells[y][x].type == CELL_STAIRS_UP) {
                arrival_x = x;
                arrival_y = y;
                break;
            }
        }
    }
    assert(arrival_x >= 0 && arrival_y >= 0);
    gs.current_level = 0;
    gs.party_x = down_x;
    gs.party_y = down_y;
    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 10, .hp_max = 10,
        .x = arrival_x, .y = arrival_y, .level = 1, .active = true
    };
    assert(!combat_change_floor_if_clear(&gs, &creatures, 1));
    assert(gs.current_level == 0 && gs.party_x == down_x && gs.party_y == down_y);

    creatures.creatures[0].active = false;
    assert(combat_change_floor_if_clear(&gs, &creatures, 1));
    assert(gs.current_level == 1 && gs.party_x == arrival_x && gs.party_y == arrival_y);
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
    test_new_mission_rejects_null_state();
    test_generated_missions_link_puzzles_to_locked_doors();
    test_generated_mission_progression_reaches_generators();
    test_init_normalizes_invalid_mission();
    test_puzzle_rejects_invalid_level();
    test_button_combo_solution_is_reachable();
    test_teleporter_never_enters_blocked_cell();
    test_generated_teleporters_target_floor();
    test_generated_power_socket_has_no_stale_target();
    test_generated_electric_traps_face_walls();
    test_fallback_door_control_remains_visible();
    test_generated_buttons_face_walls();
    test_generated_puzzles_do_not_overlap();
    test_lethal_puzzle_hazards_enter_game_over();
    test_generated_triple_levers_use_all_eight_states();
    test_combat_spawn_never_overlaps_active_creatures();
    test_combat_spawn_density_advances_by_two_groups_per_level();
    test_first_mission_uses_architect_seed_zero();
    test_completed_captive_mission_enters_holomap();
    test_extreme_mission_seed_is_defined();
    test_combat_respects_closed_doors();
    test_combat_line_of_sight_respects_blocked_corners();
    test_combat_creatures_cannot_enter_party_tile();
    test_combat_creatures_move_cardinally();
    test_combat_creature_collision_ignores_other_levels();
    test_combat_respawn_timer_runs_on_other_levels();
    test_combat_gold_reward_saturates();
    test_combat_level_up_uses_pre_attack_xp();
    test_combat_attack_events_are_per_attack();
    test_combat_uses_ranged_hand_when_melee_is_first();
    test_combat_respects_spray_weapon_range();
    test_combat_melee_rejects_diagonal_target();
    test_combat_does_not_treat_non_weapon_as_ranged();
    test_combat_requires_equipped_weapon();
    test_combat_skips_destroyed_droids();
    test_combat_tick_enters_game_over_on_last_droid();
    test_combat_ignores_invalid_creature_health();
    test_combat_extreme_damage_does_not_wrap_hp();
    test_combat_does_not_heal_from_invalid_damage();
    test_combat_spawn_extreme_level_seed_is_defined();
    test_combat_spawn_normalizes_negative_creature_count();
    test_combat_spawn_avoids_party_cell();
    test_generator_counter_does_not_overflow();
    test_combat_rejects_invalid_position_state();
    test_campaign_progression();
    test_mission_completion_rejects_overshot_generator_count();
    test_floor_change_rejects_corrupt_level_count();
    test_generated_floor_connections();
    test_floor_arrival_rejects_active_creature();
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
