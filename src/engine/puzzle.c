#include "puzzle.h"
#include "captive_data.h"
#include "i18n.h"
#include <limits.h>
#include <string.h>
#include <stdio.h>

static uint32_t puzzle_seed;
static const int dx[] = {0, 1, 0, -1};
static const int dy[] = {-1, 0, 1, 0};

static bool all_droids_dead(const GameState *gs) {
    if (!gs) return false;
    for (int i = 0; i < (int)(sizeof(gs->droids) / sizeof(gs->droids[0])); i++)
        if (gs->droids[i].hp > 0) return false;
    return true;
}

static void apply_puzzle_game_over(GameState *gs) {
    if (all_droids_dead(gs)) gs->mode = STATE_GAMEOVER;
}

static bool valid_teleport_destination(const GameState *gs, int x, int y) {
    if (!gs || gs->current_level < 0 || gs->current_level >= gs->num_levels ||
        gs->current_level >= MAX_LEVELS || x < 0 || x >= MAP_WIDTH ||
        y < 0 || y >= MAP_HEIGHT) return false;
    CellType cell = gs->levels[gs->current_level].cells[y][x].type;
    return cell != CELL_WALL && cell != CELL_DOOR && cell != CELL_DOOR_LOCKED;
}

void puzzle_init(PuzzleList *pl) {
    if (!pl) return;
    memset(pl, 0, sizeof(*pl));
}

static Puzzle *puzzle_append(PuzzleList *pl) {
    if (!pl || pl->num_puzzles < 0 || pl->num_puzzles >= MAX_PUZZLES)
        return NULL;
    Puzzle *p = &pl->puzzles[pl->num_puzzles++];
    memset(p, 0, sizeof(*p));
    /* Puzzles without a linked door use the paired sentinel.  Initializing
     * it here prevents stale coordinates when a list is reused. */
    p->target_x = -1;
    p->target_y = -1;
    return p;
}

static bool puzzle_cell_occupied(const PuzzleList *pl, int level,
                                 int x, int y) {
    if (!pl || pl->num_puzzles < 0 || pl->num_puzzles > MAX_PUZZLES)
        return true;
    for (int i = 0; i < pl->num_puzzles; i++) {
        const Puzzle *p = &pl->puzzles[i];
        if (p->level == level && p->x == x && p->y == y)
            return true;
    }
    return false;
}

static void puzzle_build_reachable(const DungeonLevel *lvl, int level_num,
                                   bool reachable[MAP_HEIGHT][MAP_WIDTH]) {
    if (!lvl) return;
    memset(reachable, 0, sizeof(bool) * MAP_HEIGHT * MAP_WIDTH);
    int start_x = -1, start_y = -1;
    for (int y = 0; y < MAP_HEIGHT && start_x < 0; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (level_num > 0 && lvl->cells[y][x].type != CELL_STAIRS_UP)
                continue;
            if (level_num == 0 && lvl->cells[y][x].type == CELL_WALL)
                continue;
            start_x = x;
            start_y = y;
            break;
        }
    }
    if (start_x < 0) {
        for (int y = 0; y < MAP_HEIGHT && start_x < 0; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                if (lvl->cells[y][x].type != CELL_WALL) {
                    start_x = x;
                    start_y = y;
                    break;
                }
    }
    if (start_x < 0 || lvl->cells[start_y][start_x].type == CELL_DOOR_LOCKED)
        return;

    int queue[MAP_HEIGHT * MAP_WIDTH];
    int head = 0, tail = 0;
    reachable[start_y][start_x] = true;
    queue[tail++] = start_y * MAP_WIDTH + start_x;
    while (head < tail) {
        int pos = queue[head++];
        int x = pos % MAP_WIDTH, y = pos / MAP_WIDTH;
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT ||
                reachable[ny][nx] || lvl->cells[ny][nx].type == CELL_WALL ||
                lvl->cells[ny][nx].type == CELL_DOOR_LOCKED)
                continue;
            reachable[ny][nx] = true;
            queue[tail++] = ny * MAP_WIDTH + nx;
        }
    }
}

static bool puzzle_add_reachable_door_control(PuzzleList *pl,
                                              DungeonLevel *lvl,
                                              int level_num,
                                              bool reachable[MAP_HEIGHT][MAP_WIDTH]) {
    if (!pl || !lvl || pl->num_puzzles >= MAX_PUZZLES) return false;
    int target_x = -1, target_y = -1;
    for (int y = 1; y < MAP_HEIGHT - 1 && target_x < 0; y++)
        for (int x = 1; x < MAP_WIDTH - 1; x++)
            if (lvl->cells[y][x].type == CELL_DOOR_LOCKED) {
                target_x = x;
                target_y = y;
                break;
            }
    if (target_x < 0) return false;

    for (int i = 0; i < pl->num_puzzles; i++) {
        const Puzzle *p = &pl->puzzles[i];
        if (p->target_x == target_x && p->target_y == target_y &&
            p->level == level_num && p->x >= 0 && p->x < MAP_WIDTH &&
            p->y >= 0 && p->y < MAP_HEIGHT && reachable[p->y][p->x])
            return true;
    }

    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (!reachable[y][x] || lvl->cells[y][x].type != CELL_FLOOR ||
                puzzle_cell_occupied(pl, level_num, x, y)) continue;
            for (int d = 0; d < 4; d++) {
                int wx = x + dx[d], wy = y + dy[d];
                if (lvl->cells[wy][wx].type != CELL_WALL) continue;
                Puzzle *p = puzzle_append(pl);
                if (!p) return false;
                p->type = PUZZLE_LEVER;
                p->x = x;
                p->y = y;
                p->level = level_num;
                p->face = d;
                p->state = 0;
                p->solution = (uint8_t)(captive_prng(&puzzle_seed) & 1U);
                p->target_x = target_x;
                p->target_y = target_y;
                p->solved = false;
                lvl->cells[y][x].ornament[d] = ORNAMENT_PANEL;
                return true;
            }
        }
    }
    /* Tiny Architect sections may have no reachable floor cell directly
     * against a wall.  The interaction state is still valid on an open cell;
     * use it as a last-resort control so the linked door cannot deadlock the
     * mission. */
    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (!reachable[y][x] ||
                (lvl->cells[y][x].type != CELL_FLOOR &&
                 lvl->cells[y][x].type != CELL_STAIRS_UP) ||
                puzzle_cell_occupied(pl, level_num, x, y)) continue;
            Puzzle *p = puzzle_append(pl);
            if (!p) return false;
            p->type = PUZZLE_LEVER;
            p->x = x;
            p->y = y;
            p->level = level_num;
            p->face = DIR_NORTH;
            p->state = 0;
            p->solution = (uint8_t)(captive_prng(&puzzle_seed) & 1U);
            p->target_x = target_x;
            p->target_y = target_y;
            p->solved = false;
            /* Keep the last-resort control visible as well as interactive.
             * This branch is used for tiny/flat layouts without a reachable
             * floor cell next to a wall. The viewport has a floor-panel
             * fallback for this otherwise wall-less surface. */
            lvl->cells[y][x].ornament[DIR_NORTH] = ORNAMENT_PANEL;
            return true;
        }
    }
    return false;
}

void puzzle_generate(PuzzleList *pl, DungeonLevel *lvl, int level_num, uint32_t seed) {
    if (!pl || !lvl || pl->num_puzzles < 0 || pl->num_puzzles > MAX_PUZZLES ||
        level_num < 0 || level_num >= MAX_LEVELS) return;
    puzzle_seed = seed + level_num * 4217;

    bool reachable[MAP_HEIGHT][MAP_WIDTH];
    puzzle_build_reachable(lvl, level_num, reachable);

    // Place buttons near doors
    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (lvl->cells[y][x].type != CELL_DOOR &&
                lvl->cells[y][x].type != CELL_DOOR_LOCKED) continue;
            if (pl->num_puzzles >= MAX_PUZZLES) return;
            if (captive_prng(&puzzle_seed) % 3 != 0) continue;

            // Find a nearby wall face to place a button
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d];
                int ny = y + dy[d];
                if (nx < 1 || nx >= MAP_WIDTH-1 || ny < 1 || ny >= MAP_HEIGHT-1) continue;
                if (lvl->cells[ny][nx].type != CELL_FLOOR) continue;
                if (puzzle_cell_occupied(pl, level_num, nx, ny)) continue;

                /* d points from the door to the button cell.  Continuing in
                 * that direction from the button must reach its backing wall;
                 * the old code checked the door itself and oriented the
                 * ornament toward the doorway. */
                int wx = nx + dx[d];
                int wy = ny + dy[d];
                if (wx < 0 || wx >= MAP_WIDTH || wy < 0 || wy >= MAP_HEIGHT) continue;
                if (lvl->cells[wy][wx].type != CELL_WALL) continue;

                Puzzle *p = puzzle_append(pl);
                if (!p) return;
                p->type = PUZZLE_BUTTON;
                p->x = nx;
                p->y = ny;
                p->level = level_num;
                p->face = d;
                p->target_x = x;
                p->target_y = y;
                p->state = 0;
                p->solved = false;

                // Set ornament on wall face
                lvl->cells[ny][nx].ornament[d] = ORNAMENT_BUTTON;
                break;
            }
        }
    }

    // Place levers in random rooms
    int num_levers = 2 + level_num / 3;
    if (num_levers > 8) num_levers = 8;
    for (int i = 0; i < num_levers; i++) {
        if (pl->num_puzzles >= MAX_PUZZLES) break;
        int attempts = 50;
        while (attempts-- > 0) {
            int rx = 2 + captive_prng(&puzzle_seed) % (MAP_WIDTH - 4);
            int ry = 2 + captive_prng(&puzzle_seed) % (MAP_HEIGHT - 4);
            if (lvl->cells[ry][rx].type != CELL_FLOOR) continue;
            if (!reachable[ry][rx]) continue;
            if (puzzle_cell_occupied(pl, level_num, rx, ry)) continue;

            // Check adjacent wall
            bool placed = false;
            for (int d = 0; d < 4; d++) {
                int wx = rx + dx[d];
                int wy = ry + dy[d];
                if (lvl->cells[wy][wx].type == CELL_WALL) {
                    Puzzle *p = puzzle_append(pl);
                    if (!p) break;
                    p->type = (captive_prng(&puzzle_seed) % 3 == 0) ?
                        PUZZLE_TRIPLE_LEVER : PUZZLE_LEVER;
                    p->x = rx;
                    p->y = ry;
                    p->level = level_num;
                    p->face = d;
                    p->state = 0;
                    /* Binary levers use bit 0; keep the stored solution in
                     * the same 0/1 domain used by interaction and hints. */
                    p->solution = p->type == PUZZLE_TRIPLE_LEVER
                        ? (uint8_t)(captive_prng(&puzzle_seed) & 7U)
                        : (uint8_t)(captive_prng(&puzzle_seed) & 1U);
                    p->solved = false;

                    // Find a locked door to link to
                    p->target_x = -1;
                    p->target_y = -1;
                    for (int sy = 1; sy < MAP_HEIGHT - 1; sy++) {
                        for (int sx = 1; sx < MAP_WIDTH - 1; sx++) {
                            if (lvl->cells[sy][sx].type == CELL_DOOR_LOCKED) {
                                p->target_x = sx;
                                p->target_y = sy;
                                goto lever_placed;
                            }
                        }
                    }
                    lever_placed:
                    lvl->cells[ry][rx].ornament[d] = ORNAMENT_PANEL;
                    placed = true;
                    break;
                }
            }
            if (placed) break;
        }
    }

    /* A locked door can split a generated floor.  Keep at least one lever on
     * the entrance side so the player can open the first linked door instead
     * of finding every generated control behind it. */
    puzzle_add_reachable_door_control(pl, lvl, level_num, reachable);

    // Place bars puzzles (number-matching, from level 3+)
    if (level_num >= 3) {
        int num_bars = 1 + level_num / 5;
        if (num_bars > 4) num_bars = 4;
        for (int i = 0; i < num_bars; i++) {
            if (pl->num_puzzles >= MAX_PUZZLES) break;
            int attempts = 60;
            while (attempts-- > 0) {
                int rx = 2 + captive_prng(&puzzle_seed) % (MAP_WIDTH - 4);
                int ry = 2 + captive_prng(&puzzle_seed) % (MAP_HEIGHT - 4);
                if (lvl->cells[ry][rx].type != CELL_FLOOR) continue;
                if (puzzle_cell_occupied(pl, level_num, rx, ry)) continue;
                bool placed = false;
                for (int d = 0; d < 4; d++) {
                    if (lvl->cells[ry + dy[d]][rx + dx[d]].type == CELL_WALL) {
                        Puzzle *p = puzzle_append(pl);
                        if (!p) break;
                        p->type = PUZZLE_BARS;
                        p->x = rx; p->y = ry;
                        p->level = level_num; p->face = d;
                        p->state = 0;
                        p->solution = (uint8_t)(1 + captive_prng(&puzzle_seed) % 15);
                        p->solved = false;
                        p->target_x = -1; p->target_y = -1;
                        for (int sy = 1; sy < MAP_HEIGHT - 1; sy++)
                            for (int sx = 1; sx < MAP_WIDTH - 1; sx++)
                                if (lvl->cells[sy][sx].type == CELL_DOOR_LOCKED) {
                                    p->target_x = sx; p->target_y = sy;
                                    goto bars_placed;
                                }
                        bars_placed:
                        lvl->cells[ry][rx].ornament[d] = ORNAMENT_SCREEN;
                        placed = true;
                        break;
                    }
                }
                if (placed) break;
            }
        }
    }

    // Place button combos (8 blue buttons, from level 4+)
    if (level_num >= 4) {
        int num_combos = 1 + level_num / 6;
        if (num_combos > 3) num_combos = 3;
        for (int i = 0; i < num_combos; i++) {
            if (pl->num_puzzles >= MAX_PUZZLES) break;
            int attempts = 60;
            while (attempts-- > 0) {
                int rx = 2 + captive_prng(&puzzle_seed) % (MAP_WIDTH - 4);
                int ry = 2 + captive_prng(&puzzle_seed) % (MAP_HEIGHT - 4);
                if (lvl->cells[ry][rx].type != CELL_FLOOR) continue;
                if (puzzle_cell_occupied(pl, level_num, rx, ry)) continue;
                bool placed = false;
                for (int d = 0; d < 4; d++) {
                    if (lvl->cells[ry + dy[d]][rx + dx[d]].type == CELL_WALL) {
                        Puzzle *p = puzzle_append(pl);
                        if (!p) break;
                        p->type = PUZZLE_BUTTON_COMBO;
                        p->x = rx; p->y = ry;
                        p->level = level_num; p->face = d;
                        p->state = 0;
                        /* This prototype exposes one panel face, so only the
                         * bit selected by that face can be toggled.  A full
                         * 8-bit solution would make almost every generated
                         * combination permanently unsolvable. */
                        p->solution = (uint8_t)((captive_prng(&puzzle_seed) & 1U)
                                                 << (d % 8));
                        p->solved = false;
                        p->target_x = -1; p->target_y = -1;
                        for (int sy = 1; sy < MAP_HEIGHT - 1; sy++)
                            for (int sx = 1; sx < MAP_WIDTH - 1; sx++)
                                if (lvl->cells[sy][sx].type == CELL_DOOR_LOCKED) {
                                    p->target_x = sx; p->target_y = sy;
                                    goto combo_placed;
                                }
                        combo_placed:
                        lvl->cells[ry][rx].ornament[d] = ORNAMENT_PANEL;
                        placed = true;
                        break;
                    }
                }
                if (placed) break;
            }
        }
    }

    // Place hidden buttons in grates (from level 2+)
    if (level_num >= 2) {
        int num_hidden = 1 + level_num / 4;
        if (num_hidden > 4) num_hidden = 4;
        for (int i = 0; i < num_hidden; i++) {
            if (pl->num_puzzles >= MAX_PUZZLES) break;
            int attempts = 60;
            while (attempts-- > 0) {
                int rx = 2 + captive_prng(&puzzle_seed) % (MAP_WIDTH - 4);
                int ry = 2 + captive_prng(&puzzle_seed) % (MAP_HEIGHT - 4);
                if (lvl->cells[ry][rx].type != CELL_FLOOR) continue;
                if (puzzle_cell_occupied(pl, level_num, rx, ry)) continue;
                bool placed = false;
                for (int d = 0; d < 4; d++) {
                    if (lvl->cells[ry + dy[d]][rx + dx[d]].type == CELL_WALL) {
                        Puzzle *p = puzzle_append(pl);
                        if (!p) break;
                        p->type = PUZZLE_HIDDEN_BUTTON;
                        p->x = rx; p->y = ry;
                        p->level = level_num; p->face = d;
                        p->state = 0;
                        p->solved = false;
                        p->target_x = -1; p->target_y = -1;
                        for (int sy = 1; sy < MAP_HEIGHT - 1; sy++)
                            for (int sx = 1; sx < MAP_WIDTH - 1; sx++)
                                if (lvl->cells[sy][sx].type == CELL_DOOR_LOCKED) {
                                    p->target_x = sx; p->target_y = sy;
                                    goto hidden_placed;
                                }
                        hidden_placed:
                        lvl->cells[ry][rx].ornament[d] = ORNAMENT_VENT;
                        placed = true;
                        break;
                    }
                }
                if (placed) break;
            }
        }
    }

    // Place floor traps (from level 3+)
    if (level_num >= 3) {
        int num_traps = 2 + level_num / 3;
        if (num_traps > 10) num_traps = 10;
        for (int i = 0; i < num_traps; i++) {
            if (pl->num_puzzles >= MAX_PUZZLES) break;
            int attempts = 40;
            while (attempts-- > 0) {
                int rx = 2 + captive_prng(&puzzle_seed) % (MAP_WIDTH - 4);
                int ry = 2 + captive_prng(&puzzle_seed) % (MAP_HEIGHT - 4);
                if (lvl->cells[ry][rx].type != CELL_FLOOR) continue;
                if (puzzle_cell_occupied(pl, level_num, rx, ry)) continue;
                Puzzle *p = puzzle_append(pl);
                if (!p) break;
                p->type = PUZZLE_FLOOR_TRAP;
                p->x = rx; p->y = ry;
                p->level = level_num;
                p->face = 0;
                p->state = (uint8_t)(10 + captive_prng(&puzzle_seed) % 30);
                p->solved = false;
                p->target_x = -1; p->target_y = -1;
                break;
            }
        }
    }

    // Place teleporter traps (from level 5+)
    if (level_num >= 5) {
        int num_tele = 1 + level_num / 5;
        if (num_tele > 4) num_tele = 4;
        for (int i = 0; i < num_tele; i++) {
            if (pl->num_puzzles >= MAX_PUZZLES) break;
            int attempts = 40;
            while (attempts-- > 0) {
                int rx = 2 + captive_prng(&puzzle_seed) % (MAP_WIDTH - 4);
                int ry = 2 + captive_prng(&puzzle_seed) % (MAP_HEIGHT - 4);
                if (lvl->cells[ry][rx].type != CELL_FLOOR) continue;
                if (puzzle_cell_occupied(pl, level_num, rx, ry)) continue;
                Puzzle *p = puzzle_append(pl);
                if (!p) break;
                p->type = PUZZLE_TELEPORTER_TRAP;
                p->x = rx; p->y = ry;
                p->level = level_num;
                p->face = 0;
                p->state = 0;
                p->solved = false;
                p->target_x = -1;
                p->target_y = -1;
                for (int target_attempts = 0; target_attempts < 60;
                     target_attempts++) {
                    int tx = 2 + captive_prng(&puzzle_seed) % (MAP_WIDTH - 4);
                    int ty = 2 + captive_prng(&puzzle_seed) % (MAP_HEIGHT - 4);
                    if (lvl->cells[ty][tx].type == CELL_FLOOR) {
                        p->target_x = tx;
                        p->target_y = ty;
                        break;
                    }
                }
                break;
            }
        }
    }

    // Place power sockets (one per level from level 2+)
    if (level_num >= 2 && pl->num_puzzles < MAX_PUZZLES) {
        int attempts = 100;
        while (attempts-- > 0) {
            int rx = 2 + captive_prng(&puzzle_seed) % (MAP_WIDTH - 4);
            int ry = 2 + captive_prng(&puzzle_seed) % (MAP_HEIGHT - 4);
            if (lvl->cells[ry][rx].type != CELL_FLOOR) continue;
            if (puzzle_cell_occupied(pl, level_num, rx, ry)) continue;

            for (int d = 0; d < 4; d++) {
                if (lvl->cells[ry + dy[d]][rx + dx[d]].type == CELL_WALL) {
                    Puzzle *p = puzzle_append(pl);
                    if (!p) break;
                    p->type = PUZZLE_POWER_SOCKET;
                    p->x = rx;
                    p->y = ry;
                    p->level = level_num;
                    p->face = d;
                    p->state = 9; // 9 charges
                    p->solved = false;
                    lvl->cells[ry][rx].ornament[d] = ORNAMENT_PIPE;
                    attempts = 0;
                    break;
                }
            }
        }
    }

    int elec_count = 1 + captive_prng(&puzzle_seed) % 3;
    for (int e = 0; e < elec_count && pl->num_puzzles < MAX_PUZZLES; e++) {
            int attempts = 50;
            while (attempts-- > 0) {
                int rx = 2 + captive_prng(&puzzle_seed) % (MAP_WIDTH - 4);
                int ry = 2 + captive_prng(&puzzle_seed) % (MAP_HEIGHT - 4);
                if (lvl->cells[ry][rx].type != CELL_FLOOR) continue;
                if (puzzle_cell_occupied(pl, level_num, rx, ry)) continue;
                int d = captive_prng(&puzzle_seed) % 4;
                if (lvl->cells[ry + dy[d]][rx + dx[d]].type != CELL_WALL)
                    continue;
                Puzzle *p = puzzle_append(pl);
                if (!p) break;
                p->type = PUZZLE_WALL_ELECTRIC;
                p->x = rx; p->y = ry;
                p->level = level_num;
                p->face = d;
                p->state = 0;
                p->solved = false;
                p->target_x = -1; p->target_y = -1;
                lvl->cells[ry][rx].ornament[d] = ORNAMENT_PANEL;
                break;
            }
        }
}

bool puzzle_interact(PuzzleList *pl, GameState *gs, int x, int y, int face) {
    if (!pl || !gs || pl->num_puzzles < 0 || pl->num_puzzles > MAX_PUZZLES ||
        gs->current_level < 0 || gs->current_level >= gs->num_levels ||
        gs->current_level >= MAX_LEVELS || x < 0 || x >= MAP_WIDTH ||
        y < 0 || y >= MAP_HEIGHT || face < 0 || face > DIR_WEST) {
        return false;
    }
    for (int i = 0; i < pl->num_puzzles; i++) {
        Puzzle *p = &pl->puzzles[i];
        if (p->x != x || p->y != y || p->face != face ||
            p->level != gs->current_level) continue;

        DungeonLevel *lvl = &gs->levels[gs->current_level];

        switch (p->type) {
            case PUZZLE_BUTTON:
                p->state = !p->state;
                // Toggle linked door
                if (p->target_x >= 0 && p->target_x < MAP_WIDTH &&
                    p->target_y >= 0 && p->target_y < MAP_HEIGHT) {
                    MapCell *cell = &lvl->cells[p->target_y][p->target_x];
                    if (cell->type == CELL_DOOR_LOCKED)
                        cell->type = CELL_DOOR;
                    else if (cell->type == CELL_DOOR)
                        cell->type = CELL_FLOOR;
                }
                p->solved = true;
                return true;

            case PUZZLE_LEVER:
                p->state = !p->state;
                if (p->state == (p->solution & 1)) {
                    p->solved = true;
                    if (p->target_x >= 0 && p->target_x < MAP_WIDTH &&
                        p->target_y >= 0 && p->target_y < MAP_HEIGHT) {
                        lvl->cells[p->target_y][p->target_x].type = CELL_DOOR;
                    }
                }
                return true;

            case PUZZLE_TRIPLE_LEVER:
                p->state = (p->state + 1) % 8;
                if (p->state == p->solution) {
                    p->solved = true;
                    if (p->target_x >= 0 && p->target_x < MAP_WIDTH &&
                        p->target_y >= 0 && p->target_y < MAP_HEIGHT) {
                        lvl->cells[p->target_y][p->target_x].type = CELL_DOOR;
                    }
                }
                return true;

            case PUZZLE_BARS:
                p->state = (p->state + 1) % 16;
                if (p->state == p->solution) {
                    p->solved = true;
                    if (p->target_x >= 0 && p->target_x < MAP_WIDTH &&
                        p->target_y >= 0 && p->target_y < MAP_HEIGHT)
                        lvl->cells[p->target_y][p->target_x].type = CELL_DOOR;
                }
                return true;

            case PUZZLE_BUTTON_COMBO:
                p->state ^= (1 << (face % 8));
                if (p->state == p->solution) {
                    p->solved = true;
                    if (p->target_x >= 0 && p->target_x < MAP_WIDTH &&
                        p->target_y >= 0 && p->target_y < MAP_HEIGHT)
                        lvl->cells[p->target_y][p->target_x].type = CELL_DOOR;
                }
                return true;

            case PUZZLE_HIDDEN_BUTTON:
                if (!p->solved) {
                    p->solved = true;
                    if (p->target_x >= 0 && p->target_x < MAP_WIDTH &&
                        p->target_y >= 0 && p->target_y < MAP_HEIGHT) {
                        MapCell *cell = &lvl->cells[p->target_y][p->target_x];
                        if (cell->type == CELL_DOOR_LOCKED)
                            cell->type = CELL_DOOR;
                    }
                }
                return true;

            case PUZZLE_FLOOR_TRAP: {
                if (gs->selected_droid < 0 ||
                    gs->selected_droid >= (int)(sizeof(gs->droids) / sizeof(gs->droids[0])))
                    return false;
                Droid *d = &gs->droids[gs->selected_droid];
                if (p->state >= d->hp)
                    d->hp = 0;
                else
                    d->hp = (int16_t)(d->hp - p->state);
                apply_puzzle_game_over(gs);
                return true;
            }

            case PUZZLE_TELEPORTER_TRAP:
                    if (valid_teleport_destination(gs, p->target_x,
                                                   p->target_y)) {
                    gs->party_x = p->target_x;
                    gs->party_y = p->target_y;
                }
                return true;

            case PUZZLE_LASER_CODE:
                return false;

            case PUZZLE_POWER_SOCKET:
                if (p->state > 0) {
                    if (gs->selected_droid < 0 ||
                        gs->selected_droid >= (int)(sizeof(gs->droids) / sizeof(gs->droids[0])))
                        return false;
                    p->state--;
                    // Recharge selected droid (420 energy per charge)
                    Droid *d = &gs->droids[gs->selected_droid];
                    int charged = (int)d->energy + 420;
                    if (charged > d->energy_max) charged = d->energy_max;
                    if (charged > INT16_MAX) charged = INT16_MAX;
                    d->energy = (int16_t)charged;
                }
                return true;

            case PUZZLE_WALL_ELECTRIC: {
                int elec_dmg = 8 + gs->current_level * 3;
                for (int di = 0; di < 4; di++) {
                    if (gs->droids[di].hp > 0) {
                        int remaining_hp = (int)gs->droids[di].hp - elec_dmg;
                        if (remaining_hp < 0) remaining_hp = 0;
                        gs->droids[di].hp = (int16_t)remaining_hp;
                    }
                }
                apply_puzzle_game_over(gs);
                return true;
            }

            default:
                break;
        }
    }
    return false;
}

void puzzle_check_step(PuzzleList *pl, GameState *gs, int x, int y) {
    if (!pl || !gs || pl->num_puzzles < 0 || pl->num_puzzles > MAX_PUZZLES ||
        gs->current_level < 0 || gs->current_level >= gs->num_levels ||
        gs->current_level >= MAX_LEVELS || x < 0 || x >= MAP_WIDTH ||
        y < 0 || y >= MAP_HEIGHT) return;
    for (int i = 0; i < pl->num_puzzles; i++) {
        Puzzle *p = &pl->puzzles[i];
        if (p->x != x || p->y != y || p->level != gs->current_level) continue;

        switch (p->type) {
            case PUZZLE_FLOOR_TRAP: {
                if (gs->selected_droid < 0 ||
                    gs->selected_droid >= (int)(sizeof(gs->droids) / sizeof(gs->droids[0])))
                    break;
                Droid *d = &gs->droids[gs->selected_droid];
                if (p->state >= d->hp)
                    d->hp = 0;
                else
                    d->hp = (int16_t)(d->hp - p->state);
                apply_puzzle_game_over(gs);
                break;
            }
            case PUZZLE_TELEPORTER_TRAP:
                if (valid_teleport_destination(gs, p->target_x,
                                               p->target_y)) {
                    gs->party_x = p->target_x;
                    gs->party_y = p->target_y;
                }
                break;
            default:
                break;
        }
    }
}

bool puzzle_get_clipboard_hint(const PuzzleList *pl, const GameState *gs,
                               int x, int y, int face, char *buf, int buf_size) {
    if (!pl || !gs || !buf || buf_size <= 0 ||
        pl->num_puzzles < 0 || pl->num_puzzles > MAX_PUZZLES ||
        gs->current_level < 0 || gs->current_level >= gs->num_levels ||
        gs->current_level >= MAX_LEVELS || x < 0 || x >= MAP_WIDTH ||
        y < 0 || y >= MAP_HEIGHT || face < 0 || face > DIR_WEST) return false;
    for (int i = 0; i < pl->num_puzzles; i++) {
        const Puzzle *p = &pl->puzzles[i];
        if (p->x != x || p->y != y || p->face != face ||
            p->level != gs->current_level || p->solved) continue;
        switch (p->type) {
            case PUZZLE_LEVER:
                snprintf(buf, buf_size, _("Solution: %s"),
                         (p->solution & 1U) ? _("ON") : _("OFF"));
                return true;
            case PUZZLE_TRIPLE_LEVER:
                snprintf(buf, buf_size, _("Solution: %d%d%d"),
                         (p->solution >> 2) & 1, (p->solution >> 1) & 1, p->solution & 1);
                return true;
            case PUZZLE_BARS:
                snprintf(buf, buf_size, _("Match: %d"), p->solution);
                return true;
            case PUZZLE_BUTTON_COMBO:
                snprintf(buf, buf_size, _("Code: %02X"), p->solution);
                return true;
            case PUZZLE_LASER_CODE:
                snprintf(buf, buf_size, _("Password: %d"), p->solution);
                return true;
            default: break;
        }
    }
    return false;
}
