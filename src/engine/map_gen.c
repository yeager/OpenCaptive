#include "map_gen.h"
#include <string.h>

// Captive uses a simple LCG PRNG
// Reference: captive.atari.org DevScapes documentation
static uint32_t prng_state;

static uint32_t prng_next(void) {
    prng_state = prng_state * 1103515245 + 12345;
    return (prng_state >> 16) & 0x7FFF;
}

static void prng_seed(uint32_t seed) {
    prng_state = seed;
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
