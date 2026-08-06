#include "mapgen_ca.h"
#include <string.h>
#include "game_state.h"

static uint16_t ca_prng(uint16_t *state) {
    *state = (uint16_t)(*state * 0x5E5u + 0x29u);
    return *state;
}

void ca_init(CAMap *map) {
    if (!map) return;
    memset(map, 0, sizeof(*map));
}

void ca_generate_pattern(CAMap *map, uint16_t *prng_state) {
    if (!map || !prng_state) return;
    for (int y = 0; y < CA_HEIGHT; y++) {
        for (int x = 0; x < CA_WIDTH; x++) {
            uint16_t r = ca_prng(prng_state);
            map->cells[y][x][0] = (uint8_t)(r & 0xFF);
            map->cells[y][x][1] = (uint8_t)((r >> 8) & 0xFF);
            r = ca_prng(prng_state);
            map->cells[y][x][2] = (uint8_t)(r & 0xFF);
            map->cells[y][x][3] = (uint8_t)((r >> 8) & 0xFF);
            map->cells[y][x][4] = (uint8_t)(r & 0x1F);
        }
    }
}

/* Type 0: Maze — wall connectivity using bits 0x101/0x808 → 0x10 */
static void ca_apply_maze(CAMap *map) {
    for (int y = 0; y < CA_HEIGHT; y++) {
        for (int x = 0; x < CA_WIDTH; x++) {
            uint8_t *c = map->cells[y][x];
            uint16_t w01 = c[0] | (c[1] << 8);
            uint16_t ax = w01 & 0x0101;
            uint16_t bx = w01 & 0x0808;
            uint8_t al = (uint8_t)(ax & 0xFF) | (uint8_t)(bx & 0xFF);
            uint8_t ah = (uint8_t)((ax >> 8) & 0xFF) | (uint8_t)((bx >> 8) & 0xFF);
            if (al) al |= 0x10;
            if (ah) ah |= 0x10;

            uint8_t dl = c[0] & 0x20;
            uint8_t bl2 = (w01 >> 8) & 0x40;
            if (dl | bl2) ah |= 0x80;
            c[0] = al | (c[0] & 0x20) | dl;
            c[1] = ah;

            uint8_t d2 = c[1] & 0x20;
            uint8_t b2 = c[2] & 0x04;
            if (d2 | b2) c[2] = (c[2] & 0xF0) | 0x08;
            else c[2] &= 0xF0;

            uint16_t w34 = c[3] | (c[4] << 8);
            ax = w34 & 0x0101;
            bx = w34 & 0x0808;
            al = (uint8_t)(ax & 0xFF) | (uint8_t)(bx & 0xFF);
            ah = (uint8_t)((ax >> 8) & 0xFF) | (uint8_t)((bx >> 8) & 0xFF);
            if (al) al |= 0x10;
            if (ah) ah |= 0x10;
            c[4] = (c[4] & 0xE0) | (ah & 0x1F);

            dl = c[2] & 0x40;
            uint16_t bx2 = (c[2] | (c[3] << 8)) & 0x4000;
            if (dl | bx2) c[3] |= 0x80;
        }
    }
}

/* Type 1: Rooms — using bits 0x202/0x404 → 0x10 */
static void ca_apply_rooms(CAMap *map) {
    for (int y = 0; y < CA_HEIGHT; y++) {
        for (int x = 0; x < CA_WIDTH; x++) {
            uint8_t *c = map->cells[y][x];
            uint16_t w01 = c[0] | (c[1] << 8);
            uint16_t ax = w01 & 0x0202;
            uint16_t bx = w01 & 0x0404;
            uint8_t al = (uint8_t)(ax & 0xFF) | (uint8_t)(bx & 0xFF);
            uint8_t ah = (uint8_t)((ax >> 8) & 0xFF) | (uint8_t)((bx >> 8) & 0xFF);
            if (al) al |= 0x10;
            if (ah) ah |= 0x10;

            uint8_t dl = c[0] & 0x80;
            uint8_t bl2 = c[0] & 0x40;
            if (dl | bl2) ah |= 0x80;
            c[0] = al | dl;
            c[1] = ah;

            dl = c[2] & 0x02;
            uint8_t b2 = c[2] & 0x01;
            if (dl | b2) c[2] = (c[2] & 0xF0) | 0x08;
            else c[2] &= 0xF0;

            uint16_t w34 = c[3] | (c[4] << 8);
            ax = w34 & 0x0202;
            bx = w34 & 0x0404;
            al = (uint8_t)(ax & 0xFF) | (uint8_t)(bx & 0xFF);
            ah = (uint8_t)((ax >> 8) & 0xFF) | (uint8_t)((bx >> 8) & 0xFF);
            if (al) al |= 0x10;
            if (ah) ah |= 0x10;
            c[4] = (c[4] & 0xE0) | (ah & 0x1F);

            uint16_t d34 = c[2] | (c[3] << 8);
            dl = (d34 >> 8) & 0x20;
            uint8_t b3 = c[2] & 0x80;
            if (dl | b3) c[3] |= 0x80;
        }
    }
}

/* Type 2: Open — using bits 0x101/0x404 → 0x18 (wider openings) */
static void ca_apply_open(CAMap *map) {
    for (int y = 0; y < CA_HEIGHT; y++) {
        for (int x = 0; x < CA_WIDTH; x++) {
            uint8_t *c = map->cells[y][x];
            uint16_t w01 = c[0] | (c[1] << 8);
            uint16_t bx = w01 & 0x0101;
            uint16_t ax = w01 & 0x0404;
            uint8_t al = (uint8_t)(ax & 0xFF) | (uint8_t)(bx & 0xFF);
            uint8_t ah = (uint8_t)((ax >> 8) & 0xFF) | (uint8_t)((bx >> 8) & 0xFF);
            if (al) al |= 0x18;
            if (ah) ah |= 0x18;

            uint8_t bl2 = c[0] & 0x20;
            uint8_t dl = c[0] & 0x80;
            if (dl | bl2) ah |= 0xC0;
            c[0] = al | dl;

            uint16_t w12 = c[1] | (c[2] << 8);
            bl2 = c[1] & 0x20;
            uint16_t d2 = w12 & 0x0200;
            if (d2 | bl2) c[2] = (c[2] & 0xF0) | 0x0C;
            else c[2] &= 0xF0;
            c[1] = ah | (uint8_t)(d2 >> 8);

            uint16_t w34 = c[3] | (c[4] << 8);
            bx = w34 & 0x0101;
            ax = w34 & 0x0404;
            al = (uint8_t)(ax & 0xFF) | (uint8_t)(bx & 0xFF);
            ah = (uint8_t)((ax >> 8) & 0xFF) | (uint8_t)((bx >> 8) & 0xFF);
            if (al) al |= 0x18;
            if (ah) ah |= 0x18;
            c[4] = (c[4] & 0xE0) | (ah & 0x1F);

            uint16_t d23 = c[2] | (c[3] << 8);
            uint8_t b3 = (d23 >> 8) & 0x20;
            dl = c[2] & 0x40;
            if (dl | b3) c[3] |= 0xC0;
        }
    }
}

/* Type 3: Mixed — like open but with extra shr+or for denser walls */
static void ca_apply_mixed(CAMap *map) {
    for (int y = 0; y < CA_HEIGHT; y++) {
        for (int x = 0; x < CA_WIDTH; x++) {
            uint8_t *c = map->cells[y][x];
            uint16_t w01 = c[0] | (c[1] << 8);
            uint16_t bx = w01 & 0x0101;
            uint16_t ax = w01 & 0x0404;
            uint16_t cx2 = ax >> 1;
            ax |= cx2;
            uint8_t al = (uint8_t)(ax & 0xFF) | (uint8_t)(bx & 0xFF);
            uint8_t ah = (uint8_t)((ax >> 8) & 0xFF) | (uint8_t)((bx >> 8) & 0xFF);
            if (al) al |= 0x18;
            if (ah) ah |= 0x18;

            uint8_t bl2 = c[0] & 0x20;
            uint8_t dl = c[0] & 0x80;
            uint8_t cl2 = dl >> 1;
            dl |= cl2;
            dl |= bl2;
            if (dl) ah |= 0xC0;
            c[0] = al | dl;

            uint16_t w12 = c[1] | (c[2] << 8);
            bl2 = c[1] & 0x20;
            uint16_t dx2 = w12 & 0x0200;
            cx2 = dx2 >> 1;
            dx2 |= cx2;
            dx2 |= bl2;
            if (dx2) c[2] = (c[2] & 0xF0) | 0x0C;
            else c[2] &= 0xF0;
            c[1] = ah | (uint8_t)((dx2 >> 8) & 0xFF);

            uint16_t w34 = c[3] | (c[4] << 8);
            bx = w34 & 0x0101;
            ax = w34 & 0x0404;
            cx2 = ax >> 1;
            ax |= cx2;
            al = (uint8_t)(ax & 0xFF) | (uint8_t)(bx & 0xFF);
            ah = (uint8_t)((ax >> 8) & 0xFF) | (uint8_t)((bx >> 8) & 0xFF);
            if (al) al |= 0x18;
            if (ah) ah |= 0x18;
            c[4] = (c[4] & 0xE0) | (ah & 0x1F);

            uint16_t d23 = c[2] | (c[3] << 8);
            bl2 = c[2] & 0x40;
            dx2 = d23 & 0x2000;
            if (dx2) bl2 |= 0x80;
            if (bl2 | dx2) c[3] |= 0xC0;
        }
    }
}

void ca_apply_rules(CAMap *map, CAMapType type) {
    if (!map) return;
    switch (type) {
        case CA_TYPE_MAZE:  ca_apply_maze(map); break;
        case CA_TYPE_ROOMS: ca_apply_rooms(map); break;
        case CA_TYPE_OPEN:  ca_apply_open(map); break;
        case CA_TYPE_MIXED: ca_apply_mixed(map); break;
    }
}

bool ca_cell_is_wall(const CAMap *map, int x, int y) {
    if (!map || x < 0 || x >= CA_WIDTH || y < 0 || y >= CA_HEIGHT) return true;
    const uint8_t *c = map->cells[y][x];
    /* A cell is wall if any of its 5 wall segment bits are set */
    return ((c[0] & 0x10) | (c[1] & 0x10) | (c[2] & 0x08) |
            (c[3] & 0x80) | (c[4] & 0x10)) != 0;
}

uint8_t ca_cell_wall_flags(const CAMap *map, int x, int y) {
    if (!map || x < 0 || x >= CA_WIDTH || y < 0 || y >= CA_HEIGHT) return 0xFF;
    const uint8_t *c = map->cells[y][x];
    uint8_t flags = 0;
    if (c[0] & 0x10) flags |= 0x01;
    if (c[1] & 0x10) flags |= 0x02;
    if (c[2] & 0x08) flags |= 0x04;
    if (c[3] & 0x80) flags |= 0x08;
    if (c[4] & 0x10) flags |= 0x10;
    return flags;
}

void ca_to_dungeon_level(const CAMap *map, DungeonLevel *level, int level_num) {
    if (!map || !level) return;
    if (level_num < 0) level_num = 0;
    if (level_num >= MAX_LEVELS) level_num = MAX_LEVELS - 1;
    memset(level, 0, sizeof(*level));
    level->level = level_num;

    /* The CA grid is 10×56 cells. The DungeonLevel is 64×32 cells.
     * Scale: each CA cell maps to ~6×1 dungeon cells horizontally,
     * and the 56 CA rows map to 32 dungeon rows (skip every other + trim). */
    for (int dy = 0; dy < MAP_HEIGHT; dy++) {
        int ca_y = (dy * CA_HEIGHT) / MAP_HEIGHT;
        if (ca_y >= CA_HEIGHT) ca_y = CA_HEIGHT - 1;
        for (int dx2 = 0; dx2 < MAP_WIDTH; dx2++) {
            int ca_x = (dx2 * CA_WIDTH) / MAP_WIDTH;
            if (ca_x >= CA_WIDTH) ca_x = CA_WIDTH - 1;

            MapCell *mc = &level->cells[dy][dx2];
            uint8_t seg = ca_cell_wall_flags(map, ca_x, ca_y);
            mc->ca_segments = seg;

            const uint8_t *c = map->cells[ca_y][ca_x];
            uint8_t thickness = 0;
            if (c[0] & 0x18) thickness = c[0] & 0x18;
            else if (c[3] & 0xC0) thickness = c[3] & 0xC0;
            mc->ca_thickness = thickness;

            if (seg != 0) {
                mc->type = CELL_WALL;
            } else {
                mc->type = CELL_FLOOR;
            }

            uint8_t base_tex = (uint8_t)(level_num % 4);
            for (int d = 0; d < 4; d++)
                mc->wall_tex[d] = base_tex;
            mc->floor_tex = base_tex;
            mc->ceil_tex = base_tex;
        }
    }

    /* Border walls */
    for (int x = 0; x < MAP_WIDTH; x++) {
        level->cells[0][x].type = CELL_WALL;
        level->cells[MAP_HEIGHT-1][x].type = CELL_WALL;
    }
    for (int y = 0; y < MAP_HEIGHT; y++) {
        level->cells[y][0].type = CELL_WALL;
        level->cells[y][MAP_WIDTH-1].type = CELL_WALL;
    }
}
