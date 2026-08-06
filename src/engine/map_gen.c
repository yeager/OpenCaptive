#include "map_gen.h"
#include "mapgen_ca.h"
#include <string.h>

/* Exact 16-bit Architect PRNG recovered at offsets 0x44a0-0x44b8 in the
 * hash-verified Amiga MapGen HUNK: state = state * 0x5e5 + 0x29; then
 * ROR.W #4 and EORI.W #0x800. Map construction below remains incomplete. */
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

void mapgen_original_prng_sequence(uint16_t seed, uint16_t *out, size_t count) {
    if (!out && count) return;
    uint16_t state = seed;
    for (size_t i = 0; i < count; ++i) {
        state = (uint16_t)(state * 0x5e5u + 0x29u);
        out[i] = (uint16_t)(ror16(state, 4) ^ 0x0800u);
    }
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
    if (!level) return;
    if (level_num < 0) level_num = 0;
    if (level_num >= MAX_LEVELS) level_num = MAX_LEVELS - 1;
    memset(level, 0, sizeof(*level));
    level->level = level_num;
    level->seed = seed;

    /* Use original CA map generation from CAPPO.EXE.
     * Map type is derived from seed bits, matching the original dispatch. */
    CAMap ca;
    ca_init(&ca);
    uint16_t ca_state = (uint16_t)(seed + level_num * 7919);
    ca_generate_pattern(&ca, &ca_state);
    CAMapType type = (CAMapType)((seed + level_num) & 3);
    ca_apply_rules(&ca, type);
    ca_to_dungeon_level(&ca, level, level_num);
    /* ca_to_dungeon_level() clears the destination before filling it, so
     * restore the source seed that belongs to the generated level. */
    level->seed = seed;

    prng_seed(seed + level_num * 7919);

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
                    bool locked = (prng_next() % 3 == 0);
                    level->cells[y][x].type = locked ? CELL_DOOR_LOCKED : CELL_DOOR;
                    if (locked) {
                        for (int ky = y - 3; ky <= y + 3; ky++) {
                            for (int kx = x - 3; kx <= x + 3; kx++) {
                                if (kx > 0 && kx < MAP_WIDTH - 1 &&
                                    ky > 0 && ky < MAP_HEIGHT - 1 &&
                                    level->cells[ky][kx].type == CELL_FLOOR &&
                                    level->cells[ky][kx].item_id == 0) {
                                    level->cells[ky][kx].item_id = 57;
                                    goto key_placed;
                                }
                            }
                        }
                        key_placed:;
                    }
                }
            }
        }
    }

    /* Generator placement from CAPPO.EXE at 0x1C3C:
     * count = (PRNG & 7) + 1, position = PRNG & 0x7F (cap 0x6F),
     * width = (PRNG & 7) + 1, placed as rectangular patches */
    int gen_count = (prng_next() & 7) + 1;
    for (int g = 0; g < gen_count; g++) {
        int gx = prng_next() & 0x7F;
        if (gx > 0x6F) gx &= 0x3F;
        int gw = (prng_next() & 7) + 1;
        if (gx + gw > MAP_WIDTH - 1) gw = MAP_WIDTH - 1 - gx;
        for (int dx2 = 0; dx2 < gw; dx2++) {
            int x = gx + dx2;
            int y = 1 + (prng_next() % (MAP_HEIGHT - 2));
            if (in_bounds(x, y) && level->cells[y][x].type == CELL_FLOOR) {
                level->cells[y][x].type = CELL_GENERATOR;
            }
        }
    }

    int pit_count = (prng_next() & 3) + 1;
    for (int p = 0; p < pit_count; p++) {
        int px = 1 + (prng_next() % (MAP_WIDTH - 2));
        int py = 1 + (prng_next() % (MAP_HEIGHT - 2));
        if (in_bounds(px, py) && level->cells[py][px].type == CELL_FLOOR)
            level->cells[py][px].type = CELL_PIT;
    }

    int plate_count = (prng_next() & 3);
    for (int p = 0; p < plate_count; p++) {
        int px = 1 + (prng_next() % (MAP_WIDTH - 2));
        int py = 1 + (prng_next() % (MAP_HEIGHT - 2));
        if (in_bounds(px, py) && level->cells[py][px].type == CELL_FLOOR)
            level->cells[py][px].type = CELL_PRESSURE_PLATE;
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
    /* Keep the special row-0 entrance cell immutable.  carve() deliberately
     * rejects the outer border, so allowing it here would consume PRNG steps
     * without changing the map and could make Architect's walk needlessly
     * expensive (and produce a false carved-cell count). */
    if (x < 1 || x >= MAP_WIDTH - 1 || y < 1 || y >= MAP_HEIGHT - 1)
        return false;
    if (section_floor[architect_section_at(x, y)] != floor) return false;
    if (level->cells[y][x].type != CELL_WALL) return false;
    return architect_open_neighbors(level, x, y) == 1;
}

static void architect_add_frontier(const DungeonLevel *level,
                                   const int section_floor[16], int floor,
                                   int x, int y,
                                   bool queued[MAP_HEIGHT][MAP_WIDTH],
                                   int frontier[MAP_WIDTH * MAP_HEIGHT],
                                   int *frontier_count) {
    if (!frontier_count || *frontier_count >= MAP_WIDTH * MAP_HEIGHT ||
        x < 1 || x >= MAP_WIDTH - 1 || y < 1 || y >= MAP_HEIGHT - 1 ||
        queued[y][x] ||
        !architect_can_carve(level, section_floor, floor, x, y))
        return;
    queued[y][x] = true;
    frontier[(*frontier_count)++] = y * MAP_WIDTH + x;
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

static int architect_floor_cell_count(const DungeonLevel *level,
                                      const int section_floor[16], int floor) {
    int count = 0;
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (section_floor[architect_section_at(x, y)] == floor &&
                level->cells[y][x].type != CELL_WALL)
                count++;
        }
    }
    return count;
}

/* A short Architect walk can terminate in a sealed-looking pocket when a
 * section is small.  Grow the existing connected component until it has a
 * useful minimum footprint, while refusing to cross the section assigned to
 * another logical floor. */
static void architect_ensure_minimum_floor(DungeonLevel *level,
                                            const int section_floor[16],
                                            int floor, int minimum) {
    int count = architect_floor_cell_count(level, section_floor, floor);
    while (count < minimum) {
        bool carved = false;
        for (int y = 0; y < MAP_HEIGHT && !carved; y++) {
            for (int x = 0; x < MAP_WIDTH && !carved; x++) {
                if (section_floor[architect_section_at(x, y)] != floor ||
                    level->cells[y][x].type != CELL_WALL ||
                    x < 1 || x >= MAP_WIDTH - 1 ||
                    y < 1 || y >= MAP_HEIGHT - 1)
                    continue;
                int open_neighbors = 0;
                for (int d = 0; d < 4; d++) {
                    int nx = x + dx[d], ny = y + dy[d];
                    if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT &&
                        section_floor[architect_section_at(nx, ny)] == floor &&
                        level->cells[ny][nx].type != CELL_WALL)
                        open_neighbors++;
                }
                if (open_neighbors == 0) continue;
                carve(level, x, y);
                count++;
                carved = true;
            }
        }
        if (!carved) break;
    }
}

static void architect_carve_floor(DungeonLevel *level, const int section_floor[16],
                                  int floor, int start_x, int start_y) {
    level->cells[start_y][start_x].type = CELL_FLOOR;
    int x = start_x, y = start_y;
    int carved = 1;
    int frontier[MAP_WIDTH * MAP_HEIGHT];
    int frontier_count = 0;
    bool queued[MAP_HEIGHT][MAP_WIDTH] = {{false}};

    for (int d = 0; d < 4; d++)
        architect_add_frontier(level, section_floor, floor,
                               x + dx[d], y + dy[d], queued,
                               frontier, &frontier_count);

    /* Maintain the candidate frontier incrementally.  The previous
     * implementation rescanned the complete map after every blocked random
     * walk step, which made broad seed validation effectively quadratic. */
    while (frontier_count > 0 && carved < 720) {
        int index = (int)(prng_next() % (uint32_t)frontier_count);
        int pos = frontier[index];
        frontier[index] = frontier[--frontier_count];
        int nx = pos % MAP_WIDTH, ny = pos / MAP_WIDTH;
        queued[ny][nx] = false;
        if (!architect_can_carve(level, section_floor, floor, nx, ny)) continue;

        carve(level, nx, ny);
        x = nx; y = ny;
        carved++;
        for (int d = 0; d < 4; d++)
            architect_add_frontier(level, section_floor, floor,
                                   x + dx[d], y + dy[d], queued,
                                   frontier, &frontier_count);
        if ((prng_next() % 31) == 0) {
            architect_try_room(level, section_floor, floor, x, y);
            /* Rooms can add several new boundary cells.  The maximum room is
             * 6x5, so a small deterministic neighbourhood covers its rim. */
            for (int ry = y - 4; ry <= y + 4; ry++)
                for (int rx = x - 4; rx <= x + 4; rx++)
                    architect_add_frontier(level, section_floor, floor,
                                           rx, ry, queued,
                                           frontier, &frontier_count);
        }
    }
    architect_ensure_minimum_floor(level, section_floor, floor, 21);
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
    if (!out_num_levels) return;
    *out_num_levels = 0;
    if (!levels) return;
    int section_floor[16];
    int sections[16], section_count = 0;
    prng_seed(seed);
    for (int s = 0; s < 16; s++) {
        section_floor[s] = -1;
        if (architect_allowed_section(seed, s)) sections[section_count++] = s;
    }
    /* A malformed/future mask must still produce initialized geometry for the
     * mission loader; the original layout always has at least one section. */
    if (section_count == 0) sections[section_count++] = 0;

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
    /* Keep malformed/future section masks from turning root selection into a
     * modulo-zero operation. */
    int root_section = root_count > 0 ? root_sections[prng_next() % root_count] : 0;
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
        levels[f].level = f;
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

void map_generate_exterior(DungeonLevel *exterior, int entrance_x, uint32_t seed) {
    if (!exterior) return;
    memset(exterior, 0, sizeof(*exterior));
    if (entrance_x < 1) entrance_x = 1;
    if (entrance_x >= MAP_WIDTH - 1) entrance_x = MAP_WIDTH - 2;
    exterior->level = -1;
    exterior->seed = seed;

    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            exterior->cells[y][x].type = CELL_WALL;

    /* Carve an open landing pad area centered on the base entrance.
     * The original uses a roughly 20-wide, 10-tall zone with the
     * base entrance at the top edge (y=1) and the landing spot near
     * the bottom. */
    int pad_x0 = entrance_x - 10;
    int pad_x1 = entrance_x + 10;
    if (pad_x0 < 1) pad_x0 = 1;
    if (pad_x1 > MAP_WIDTH - 2) pad_x1 = MAP_WIDTH - 2;
    int pad_y0 = 1;
    int pad_y1 = 12;
    if (pad_y1 > MAP_HEIGHT - 2) pad_y1 = MAP_HEIGHT - 2;

    for (int y = pad_y0; y <= pad_y1; y++)
        for (int x = pad_x0; x <= pad_x1; x++)
            exterior->cells[y][x].type = CELL_FLOOR;

    /* Base entrance: a door at the top center connecting to floor 0 */
    exterior->cells[pad_y0][entrance_x].type = CELL_STAIRS_DOWN;

    /* Scatter some terrain obstacles using the mission PRNG */
    uint16_t state = (uint16_t)(seed ^ 0xA3B7);
    for (int i = 0; i < 8; i++) {
        state = (uint16_t)(state * 0x5E5u + 0x29u);
        int ox = pad_x0 + 2 + (state % (pad_x1 - pad_x0 - 3));
        state = (uint16_t)(state * 0x5E5u + 0x29u);
        int oy = pad_y0 + 3 + (state % (pad_y1 - pad_y0 - 4));
        if (oy > pad_y0 + 1 && exterior->cells[oy][ox].type == CELL_FLOOR)
            exterior->cells[oy][ox].type = CELL_WALL;
    }

    /* Texture: exterior uses texture set 0 */
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++) {
            for (int d = 0; d < 4; d++)
                exterior->cells[y][x].wall_tex[d] = 0;
            exterior->cells[y][x].floor_tex = 0;
            exterior->cells[y][x].ceil_tex = 0;
        }
}
