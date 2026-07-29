#include "map_gen.h"
#include <string.h>

/* Deterministic implementation PRNG.  Its output keeps current saves and
 * tests stable, but it has not been validated against original Architect
 * output and must not be described as recovered MapGen code. */
static uint16_t prng_state;

static uint16_t ror16(uint16_t value, unsigned count) {
    return (uint16_t)((value >> count) | (value << (16 - count)));
}

static uint16_t prng_next(void) {
    prng_state = (uint16_t)(prng_state * 1509u + 41u);
    return (uint16_t)(ror16(prng_state, 4) ^ 0x0800u);
}

static void prng_seed(uint32_t seed) {
    prng_state = (uint16_t)seed;
}

static void carve(DungeonLevel *level, int x, int y) {
    if (x >= 1 && x < MAP_WIDTH - 1 && y >= 1 && y < MAP_HEIGHT - 1) {
        level->cells[y][x].type = CELL_FLOOR;
    }
}

static bool in_bounds(int x, int y) {
    return x >= 1 && x < MAP_WIDTH - 1 && y >= 1 && y < MAP_HEIGHT - 1;
}

// Direction offsets: N, E, S, W
static const int dx[] = { 0, 1, 0, -1 };
static const int dy[] = { -1, 0, 1, 0 };

static void place_room(DungeonLevel *level, int cx, int cy, int w, int h) {
    for (int ry = cy - h/2; ry <= cy + h/2; ry++) {
        for (int rx = cx - w/2; rx <= cx + w/2; rx++) {
            carve(level, rx, ry);
        }
    }
}

static void place_corridor(DungeonLevel *level, int x1, int y1, int x2, int y2) {
    int x = x1, y = y1;
    while (x != x2) {
        carve(level, x, y);
        x += (x2 > x) ? 1 : -1;
    }
    while (y != y2) {
        carve(level, x, y);
        y += (y2 > y) ? 1 : -1;
    }
    carve(level, x2, y2);
}

void map_generate(DungeonLevel *level, uint32_t seed, int level_num) {
    memset(level, 0, sizeof(*level));
    level->level = level_num;
    level->seed = seed;

    prng_seed(seed + level_num * 7919);

    // Start with all walls
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            level->cells[y][x].type = CELL_WALL;

    // Place rooms (Captive typically has 8-20 rooms per level)
    int num_rooms = 8 + (prng_next() % 13);
    int room_cx[32], room_cy[32];
    int placed = 0;

    for (int i = 0; i < num_rooms && placed < 32; i++) {
        int cx = 4 + (prng_next() % (MAP_WIDTH - 8));
        int cy = 4 + (prng_next() % (MAP_HEIGHT - 8));
        int rw = 2 + (prng_next() % 5);
        int rh = 2 + (prng_next() % 4);

        place_room(level, cx, cy, rw, rh);
        room_cx[placed] = cx;
        room_cy[placed] = cy;
        placed++;
    }

    // Connect rooms with corridors
    for (int i = 1; i < placed; i++) {
        place_corridor(level, room_cx[i-1], room_cy[i-1],
                       room_cx[i], room_cy[i]);
    }

    // Place entrance stairs (in first room)
    if (placed > 0) {
        level->cells[room_cy[0]][room_cx[0]].type =
            (level_num == 0) ? CELL_FLOOR : CELL_STAIRS_UP;
    }

    // Place exit stairs (in last room)
    if (placed > 1) {
        level->cells[room_cy[placed-1]][room_cx[placed-1]].type = CELL_STAIRS_DOWN;
    }

    // Place doors at corridor-room junctions
    for (int y = 2; y < MAP_HEIGHT - 2; y++) {
        for (int x = 2; x < MAP_WIDTH - 2; x++) {
            if (level->cells[y][x].type != CELL_FLOOR) continue;

            // Check for corridor pinch points (wall on two opposite sides, open on other two)
            bool ns_wall = (level->cells[y-1][x].type == CELL_WALL &&
                           level->cells[y+1][x].type == CELL_WALL);
            bool ew_open = (level->cells[y][x-1].type == CELL_FLOOR &&
                           level->cells[y][x+1].type == CELL_FLOOR);
            bool ew_wall = (level->cells[y][x-1].type == CELL_WALL &&
                           level->cells[y][x+1].type == CELL_WALL);
            bool ns_open = (level->cells[y-1][x].type == CELL_FLOOR &&
                           level->cells[y+1][x].type == CELL_FLOOR);

            if ((ns_wall && ew_open) || (ew_wall && ns_open)) {
                if (prng_next() % 4 == 0) {
                    level->cells[y][x].type = (prng_next() % 3 == 0)
                        ? CELL_DOOR_LOCKED : CELL_DOOR;
                }
            }
        }
    }

    // Place generators (mission objectives)
    int gens_to_place = 1 + (level_num / 3);
    for (int g = 0; g < gens_to_place && g < placed; g++) {
        int ri = placed - 1 - g;
        if (ri <= 0) ri = 1;
        int gx = room_cx[ri] + (prng_next() % 3) - 1;
        int gy = room_cy[ri] + (prng_next() % 3) - 1;
        if (in_bounds(gx, gy) && level->cells[gy][gx].type == CELL_FLOOR) {
            level->cells[gy][gx].type = CELL_GENERATOR;
        }
    }

    // Place shops (one per level, usually)
    if (placed > 2) {
        int si = 1 + (prng_next() % (placed - 2));
        level->cells[room_cy[si]][room_cx[si]].type = CELL_SHOP;
    }

    // Assign wall textures based on level
    uint8_t base_wall = (level_num % 4);
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            for (int d = 0; d < 4; d++) {
                level->cells[y][x].wall_tex[d] = base_wall;
            }
            level->cells[y][x].floor_tex = base_wall;
            level->cells[y][x].ceil_tex = base_wall;
        }
    }

    // Scatter wall ornaments
    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (level->cells[y][x].type == CELL_WALL) continue;
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d];
                int ny = y + dy[d];
                if (in_bounds(nx, ny) && level->cells[ny][nx].type == CELL_WALL) {
                    if (prng_next() % 8 == 0) {
                        level->cells[y][x].ornament[d] =
                            (OrnamentType)(1 + (prng_next() % 7));
                    }
                }
            }
        }
    }
}

/* --- Architect-style flattened base generation --------------------------------
 *
 * Captive does not make one independent maze per floor.  It partitions the
 * 64x32 allocation into sixteen 16x8 regions, assigns each region to a floor,
 * and then carves paths while treating boundaries between different floors as
 * unconditional walls.  The public guide documents this representation; the
 * routines below preserve it within OpenCaptive's existing level-array API.
 */

#define ARCH_SECTIONS_X 4
#define ARCH_SECTIONS_Y 4
#define ARCH_SECTION_W 16
#define ARCH_SECTION_H 8

static int architect_section_at(int x, int y) {
    return (y / ARCH_SECTION_H) * ARCH_SECTIONS_X + (x / ARCH_SECTION_W);
}

static bool architect_allowed_section(uint32_t seed, int section) {
    /* The first bases progressively unlock the 4x4 section grid. */
    if (seed == 0) return section == 1 || section == 5 || section == 9;
    if (seed == 1) return section < 4 || section == 5 || section == 6;
    if (seed == 2) return section < 8 || section == 9 || section == 10;
    if (seed == 3) return section < 12;
    if (seed == 4) return section < 12 || section == 13 || section == 14;
    return true;
}

static int architect_open_neighbors(const DungeonLevel *level, int x, int y) {
    int count = 0;
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT &&
            level->cells[ny][nx].type != CELL_WALL) count++;
    }
    return count;
}

static bool architect_can_carve(const DungeonLevel *level, const int section_floor[16],
                                int floor, int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return false;
    if (section_floor[architect_section_at(x, y)] != floor) return false;
    if (level->cells[y][x].type != CELL_WALL) return false;
    return architect_open_neighbors(level, x, y) == 1;
}

static bool architect_find_frontier(const DungeonLevel *level, const int section_floor[16],
                                    int floor, int *x, int *y) {
    int start = (int)(prng_next() % (MAP_WIDTH * MAP_HEIGHT));
    for (int attempt = 0; attempt < MAP_WIDTH * MAP_HEIGHT; attempt++) {
        int pos = (start + attempt) % (MAP_WIDTH * MAP_HEIGHT);
        int px = pos % MAP_WIDTH, py = pos / MAP_WIDTH;
        if (section_floor[architect_section_at(px, py)] != floor ||
            level->cells[py][px].type == CELL_WALL) continue;
        for (int d = 0; d < 4; d++) {
            if (architect_can_carve(level, section_floor, floor,
                                    px + dx[d], py + dy[d])) {
                *x = px;
                *y = py;
                return true;
            }
        }
    }
    return false;
}

static void architect_try_room(DungeonLevel *level, const int section_floor[16],
                               int floor, int cx, int cy) {
    int width = 2 + (int)(prng_next() % 5);
    int height = 2 + (int)(prng_next() % 4);
    int x0 = cx - width / 2, y0 = cy - height / 2;
    int x1 = x0 + width - 1, y1 = y0 + height - 1;
    if (x0 < 0 || y0 < 0 || x1 >= MAP_WIDTH || y1 >= MAP_HEIGHT) return;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (section_floor[architect_section_at(x, y)] != floor) return;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (level->cells[y][x].type == CELL_WALL) carve(level, x, y);
}

static void architect_carve_floor(DungeonLevel *level, const int section_floor[16],
                                  int floor, int start_x, int start_y) {
    level->cells[start_y][start_x].type = CELL_FLOOR;
    int x = start_x, y = start_y;
    int carved = 1;
    for (int move = 0; move < 12289 && carved < 720; move++) {
        int direction = (int)(prng_next() % 4);
        int nx = x + dx[direction], ny = y + dy[direction];
        if (architect_can_carve(level, section_floor, floor, nx, ny)) {
            carve(level, nx, ny);
            x = nx; y = ny;
            carved++;
            if ((prng_next() % 31) == 0)
                architect_try_room(level, section_floor, floor, x, y);
            continue;
        }
        if (!architect_find_frontier(level, section_floor, floor, &x, &y)) break;
    }
}

static bool architect_pick_cell(const DungeonLevel *level, int *x, int *y) {
    int start = (int)(prng_next() % (MAP_WIDTH * MAP_HEIGHT));
    for (int n = 0; n < MAP_WIDTH * MAP_HEIGHT; n++) {
        int pos = (start + n) % (MAP_WIDTH * MAP_HEIGHT);
        int px = pos % MAP_WIDTH, py = pos / MAP_WIDTH;
        /* Keep row zero clear for the base entrance on the root floor. */
        if (py != 0 && level->cells[py][px].type == CELL_FLOOR) {
            *x = px; *y = py;
            return true;
        }
    }
    return false;
}

static void architect_finish_level(DungeonLevel *level, int level_num) {
    level->level = level_num;
    uint8_t base_wall = (uint8_t)(level_num % 4);
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            for (int d = 0; d < 4; d++) level->cells[y][x].wall_tex[d] = base_wall;
            level->cells[y][x].floor_tex = base_wall;
            level->cells[y][x].ceil_tex = base_wall;
        }
    }
    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (level->cells[y][x].type == CELL_WALL) continue;
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d], ny = y + dy[d];
                if (level->cells[ny][nx].type == CELL_WALL && prng_next() % 8 == 0)
                    level->cells[y][x].ornament[d] =
                        (OrnamentType)(1 + prng_next() % 7);
            }
        }
    }
}

/* Architect places doors after carving paths, at one-cell choke points.  Keep
 * that placement separate from the digger: a door is an interactive barrier,
 * not a different kind of path. */
static void architect_place_doors(DungeonLevel *level) {
    int candidates[MAP_WIDTH * MAP_HEIGHT];
    int count = 0;

    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (level->cells[y][x].type != CELL_FLOOR) continue;
            bool ns_walls = level->cells[y - 1][x].type == CELL_WALL &&
                            level->cells[y + 1][x].type == CELL_WALL;
            bool ew_open = level->cells[y][x - 1].type == CELL_FLOOR &&
                           level->cells[y][x + 1].type == CELL_FLOOR;
            bool ew_walls = level->cells[y][x - 1].type == CELL_WALL &&
                            level->cells[y][x + 1].type == CELL_WALL;
            bool ns_open = level->cells[y - 1][x].type == CELL_FLOOR &&
                           level->cells[y + 1][x].type == CELL_FLOOR;
            if ((ns_walls && ew_open) || (ew_walls && ns_open))
                candidates[count++] = y * MAP_WIDTH + x;
        }
    }

    /* A base without a door makes the door and wall-control gameplay paths
     * unreachable.  Guarantee one where topology permits, then add up to the
     * documented maximum of eight with Architect's PRNG. */
    int placed = 0;
    while (count > 0 && placed < 8) {
        int index = (int)(prng_next() % (uint16_t)count);
        int pos = candidates[index];
        candidates[index] = candidates[--count];
        if (placed == 0 || (prng_next() & 3u) == 0) {
            level->cells[pos / MAP_WIDTH][pos % MAP_WIDTH].type = CELL_DOOR;
            placed++;
        }
    }
}

void map_generate_base(DungeonLevel levels[MAX_LEVELS], int *out_num_levels,
                       uint32_t seed) {
    int section_floor[16];
    int sections[16], section_count = 0;
    prng_seed(seed);
    for (int s = 0; s < 16; s++) {
        section_floor[s] = -1;
        if (architect_allowed_section(seed, s)) sections[section_count++] = s;
    }

    int floors = (seed == 0) ? 1 : 2 + (int)(prng_next() % 4);
    if (floors > section_count) floors = section_count;
    if (floors > MAX_LEVELS) floors = MAX_LEVELS;

    /* Assign at least one physical 16x8 region to each floor, then assign the
     * remaining regions adjacent to an already assigned region where possible. */
    /* The root is always on the first map row.  Reserve a permitted top-row
     * section for floor 0 before assigning the remaining logical floors. */
    int root_sections[ARCH_SECTIONS_X], root_count = 0;
    for (int s = 0; s < ARCH_SECTIONS_X; s++)
        if (architect_allowed_section(seed, s)) root_sections[root_count++] = s;
    int root_section = root_sections[prng_next() % root_count];
    section_floor[root_section] = 0;

    for (int i = 1; i < floors; i++) {
        int at = (int)(prng_next() % section_count);
        while (section_floor[sections[at]] != -1)
            at = (at + 1) % section_count;
        section_floor[sections[at]] = i;
    }
    /* Grow outward from the assigned seeds.  Each physical section therefore
     * belongs to one contiguous logical floor, matching Architect's walk from
     * a section to an adjacent section during layout creation. */
    int assigned = floors;
    while (assigned < section_count) {
        int frontier_sections[16], frontier_count = 0;
        for (int i = 0; i < section_count; i++) {
            int s = sections[i];
            if (section_floor[s] != -1) continue;
            int sx = s % 4, sy = s / 4;
            for (int d = 0; d < 4; d++) {
                int nx = sx + dx[d], ny = sy + dy[d];
                if (nx >= 0 && nx < 4 && ny >= 0 && ny < 4 &&
                    section_floor[ny * 4 + nx] >= 0) {
                    frontier_sections[frontier_count++] = s;
                    break;
                }
            }
        }
        /* All documented early-map masks are connected.  Keep a deterministic
         * fallback for a malformed future mask rather than leaving a section
         * unassigned. */
        int s = frontier_count ? frontier_sections[prng_next() % frontier_count] : -1;
        if (s < 0) {
            for (int i = 0; i < section_count; i++)
                if (section_floor[sections[i]] == -1) { s = sections[i]; break; }
        }
        int candidates[4], count = 0;
        int sx = s % 4, sy = s / 4;
        for (int d = 0; d < 4; d++) {
            int nx = sx + dx[d], ny = sy + dy[d];
            if (nx >= 0 && nx < 4 && ny >= 0 && ny < 4) {
                int adjacent = ny * 4 + nx;
                if (section_floor[adjacent] >= 0) candidates[count++] = section_floor[adjacent];
            }
        }
        section_floor[s] = count ? candidates[prng_next() % count] : 0;
        assigned++;
    }

    for (int f = 0; f < floors; f++) {
        memset(&levels[f], 0, sizeof(levels[f]));
        levels[f].seed = seed;
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                levels[f].cells[y][x].type = CELL_WALL;

        int start_section = -1;
        for (int s = 0; s < 16; s++)
            if (section_floor[s] == f) { start_section = s; break; }
        int sx = (start_section % 4) * ARCH_SECTION_W + ARCH_SECTION_W / 2;
        int sy = (start_section / 4) * ARCH_SECTION_H + ARCH_SECTION_H / 2;
        if (f == 0) {
            /* The first base is hard-coded by Architect to the documented
             * column 30, not the centre of its sparse top-row section. */
            sx = seed == 0U ? 30 :
                (root_section % 4) * ARCH_SECTION_W + ARCH_SECTION_W / 2;
            sy = 0;
        }
        architect_carve_floor(&levels[f], section_floor, f, sx, sy);
        architect_finish_level(&levels[f], f);
        architect_place_doors(&levels[f]);
    }

    /* Connect adjacent logical floors using paired cells.  The current game
     * state already handles these as stairs, while Architect uses offsets into
     * its flat allocation for the same transition. */
    for (int f = 0; f + 1 < floors; f++) {
        int down_x, down_y, up_x, up_y;
        if (architect_pick_cell(&levels[f], &down_x, &down_y) &&
            architect_pick_cell(&levels[f + 1], &up_x, &up_y)) {
            levels[f].cells[down_y][down_x].type = CELL_STAIRS_DOWN;
            levels[f + 1].cells[up_y][up_x].type = CELL_STAIRS_UP;
        }
    }

    for (int f = 0; f < floors; f++) {
        int x, y;
        if (architect_pick_cell(&levels[f], &x, &y))
            levels[f].cells[y][x].type = CELL_GENERATOR;
        if (architect_pick_cell(&levels[f], &x, &y))
            levels[f].cells[y][x].type = CELL_SHOP;
    }
    *out_num_levels = floors;
}
