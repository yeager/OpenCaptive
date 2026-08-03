#include "puzzle.h"
#include "captive_data.h"
#include <string.h>

static uint32_t puzzle_seed;

void puzzle_init(PuzzleList *pl) {
    memset(pl, 0, sizeof(*pl));
}

void puzzle_generate(PuzzleList *pl, DungeonLevel *lvl, int level_num, uint32_t seed) {
    puzzle_seed = seed + level_num * 4217;

    static const int dx[] = {0, 1, 0, -1};
    static const int dy[] = {-1, 0, 1, 0};

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

                // Check that the opposite direction from this floor cell has a wall
                int opp = (d + 2) % 4;
                int wx = nx + dx[opp];
                int wy = ny + dy[opp];
                if (wx < 0 || wx >= MAP_WIDTH || wy < 0 || wy >= MAP_HEIGHT) continue;

                Puzzle *p = &pl->puzzles[pl->num_puzzles++];
                p->type = PUZZLE_BUTTON;
                p->x = nx;
                p->y = ny;
                p->level = level_num;
                p->face = opp;
                p->target_x = x;
                p->target_y = y;
                p->state = 0;
                p->solved = false;

                // Set ornament on wall face
                lvl->cells[ny][nx].ornament[opp] = ORNAMENT_BUTTON;
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

            // Check adjacent wall
            for (int d = 0; d < 4; d++) {
                int wx = rx + dx[d];
                int wy = ry + dy[d];
                if (lvl->cells[wy][wx].type == CELL_WALL) {
                    Puzzle *p = &pl->puzzles[pl->num_puzzles++];
                    p->type = (captive_prng(&puzzle_seed) % 3 == 0) ? PUZZLE_TRIPLE_LEVER : PUZZLE_LEVER;
                    p->x = rx;
                    p->y = ry;
                    p->level = level_num;
                    p->face = d;
                    p->state = 0;
                    p->solution = (uint8_t)(1 + captive_prng(&puzzle_seed) % 7);
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
                    break;
                }
            }
            break;
        }
    }

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
                for (int d = 0; d < 4; d++) {
                    if (lvl->cells[ry + dy[d]][rx + dx[d]].type == CELL_WALL) {
                        Puzzle *p = &pl->puzzles[pl->num_puzzles++];
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
                        break;
                    }
                }
                break;
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
                for (int d = 0; d < 4; d++) {
                    if (lvl->cells[ry + dy[d]][rx + dx[d]].type == CELL_WALL) {
                        Puzzle *p = &pl->puzzles[pl->num_puzzles++];
                        p->type = PUZZLE_BUTTON_COMBO;
                        p->x = rx; p->y = ry;
                        p->level = level_num; p->face = d;
                        p->state = 0;
                        p->solution = (uint8_t)(captive_prng(&puzzle_seed) & 0xFF);
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
                        break;
                    }
                }
                break;
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
                for (int d = 0; d < 4; d++) {
                    if (lvl->cells[ry + dy[d]][rx + dx[d]].type == CELL_WALL) {
                        Puzzle *p = &pl->puzzles[pl->num_puzzles++];
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
                        break;
                    }
                }
                break;
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
                Puzzle *p = &pl->puzzles[pl->num_puzzles++];
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
                Puzzle *p = &pl->puzzles[pl->num_puzzles++];
                p->type = PUZZLE_TELEPORTER_TRAP;
                p->x = rx; p->y = ry;
                p->level = level_num;
                p->face = 0;
                p->state = 0;
                p->solved = false;
                int tx = 2 + captive_prng(&puzzle_seed) % (MAP_WIDTH - 4);
                int ty = 2 + captive_prng(&puzzle_seed) % (MAP_HEIGHT - 4);
                p->target_x = tx; p->target_y = ty;
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

            for (int d = 0; d < 4; d++) {
                if (lvl->cells[ry + dy[d]][rx + dx[d]].type == CELL_WALL) {
                    Puzzle *p = &pl->puzzles[pl->num_puzzles++];
                    p->type = PUZZLE_POWER_SOCKET;
                    p->x = rx;
                    p->y = ry;
                    p->level = level_num;
                    p->face = d;
                    p->state = 9; // 9 charges
                    p->solved = false;
                    lvl->cells[ry][rx].ornament[d] = ORNAMENT_PIPE;
                    break;
                }
            }
            break;
        }
    }

    int elec_count = 1 + captive_prng(&puzzle_seed) % 3;
    for (int e = 0; e < elec_count && pl->num_puzzles < MAX_PUZZLES; e++) {
        int attempts = 50;
        while (attempts-- > 0) {
            int rx = 2 + captive_prng(&puzzle_seed) % (MAP_WIDTH - 4);
            int ry = 2 + captive_prng(&puzzle_seed) % (MAP_HEIGHT - 4);
            if (lvl->cells[ry][rx].type != CELL_FLOOR) continue;
            int d = captive_prng(&puzzle_seed) % 4;
            Puzzle *p = &pl->puzzles[pl->num_puzzles++];
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
    for (int i = 0; i < pl->num_puzzles; i++) {
        Puzzle *p = &pl->puzzles[i];
        if (p->x != x || p->y != y || p->face != face ||
            p->level != gs->current_level) continue;

        DungeonLevel *lvl = &gs->levels[gs->current_level];

        switch (p->type) {
            case PUZZLE_BUTTON:
                p->state = !p->state;
                // Toggle linked door
                if (p->target_x >= 0 && p->target_y >= 0) {
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
                    if (p->target_x >= 0 && p->target_y >= 0) {
                        lvl->cells[p->target_y][p->target_x].type = CELL_DOOR;
                    }
                }
                return true;

            case PUZZLE_TRIPLE_LEVER:
                p->state = (p->state + 1) % 8;
                if (p->state == p->solution) {
                    p->solved = true;
                    if (p->target_x >= 0 && p->target_y >= 0) {
                        lvl->cells[p->target_y][p->target_x].type = CELL_DOOR;
                    }
                }
                return true;

            case PUZZLE_BARS:
                p->state = (p->state + 1) % 16;
                if (p->state == p->solution) {
                    p->solved = true;
                    if (p->target_x >= 0 && p->target_y >= 0)
                        lvl->cells[p->target_y][p->target_x].type = CELL_DOOR;
                }
                return true;

            case PUZZLE_BUTTON_COMBO:
                p->state ^= (1 << (face % 8));
                if (p->state == p->solution) {
                    p->solved = true;
                    if (p->target_x >= 0 && p->target_y >= 0)
                        lvl->cells[p->target_y][p->target_x].type = CELL_DOOR;
                }
                return true;

            case PUZZLE_HIDDEN_BUTTON:
                if (!p->solved) {
                    p->solved = true;
                    if (p->target_x >= 0 && p->target_y >= 0) {
                        MapCell *cell = &lvl->cells[p->target_y][p->target_x];
                        if (cell->type == CELL_DOOR_LOCKED)
                            cell->type = CELL_DOOR;
                    }
                }
                return true;

            case PUZZLE_FLOOR_TRAP: {
                Droid *d = &gs->droids[gs->selected_droid];
                d->hp -= p->state;
                if (d->hp < 0) d->hp = 0;
                return true;
            }

            case PUZZLE_TELEPORTER_TRAP:
                if (p->target_x >= 0 && p->target_y >= 0) {
                    gs->party_x = p->target_x;
                    gs->party_y = p->target_y;
                }
                return true;

            case PUZZLE_LASER_CODE:
                return false;

            case PUZZLE_POWER_SOCKET:
                if (p->state > 0) {
                    p->state--;
                    // Recharge selected droid (420 energy per charge)
                    Droid *d = &gs->droids[gs->selected_droid];
                    d->energy += 42;
                    if (d->energy > d->energy_max)
                        d->energy = d->energy_max;
                }
                return true;

            case PUZZLE_WALL_ELECTRIC: {
                int elec_dmg = 8 + gs->current_level * 3;
                for (int di = 0; di < 4; di++) {
                    if (gs->droids[di].hp > 0) {
                        gs->droids[di].hp -= (int16_t)elec_dmg;
                        if (gs->droids[di].hp < 0) gs->droids[di].hp = 0;
                    }
                }
                return true;
            }

            default:
                break;
        }
    }
    return false;
}

void puzzle_check_step(PuzzleList *pl, GameState *gs, int x, int y) {
    for (int i = 0; i < pl->num_puzzles; i++) {
        Puzzle *p = &pl->puzzles[i];
        if (p->x != x || p->y != y || p->level != gs->current_level) continue;

        switch (p->type) {
            case PUZZLE_FLOOR_TRAP: {
                Droid *d = &gs->droids[gs->selected_droid];
                d->hp -= p->state;
                if (d->hp < 0) d->hp = 0;
                break;
            }
            case PUZZLE_TELEPORTER_TRAP:
                if (p->target_x >= 0 && p->target_y >= 0) {
                    gs->party_x = p->target_x;
                    gs->party_y = p->target_y;
                }
                break;
            default:
                break;
        }
    }
}
