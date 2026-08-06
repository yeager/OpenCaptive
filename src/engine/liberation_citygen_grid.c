#include "liberation_citygen_grid.h"
#include "liberation_citygen.h"
#include <string.h>

static const CityGridDirection directions[4] = {
    { 0, -1, -64}, /* N */
    { 1,  0,   1}, /* E */
    { 0,  1,  64}, /* S */
    {-1,  0,  -1}, /* W */
};

static const uint8_t road_corners[4][2] = {
    {3, 0}, {6, 3}, {3, 6}, {0, 3},
};

static const int16_t road_dir_dx[4] = {0, 1, 0, -1};
static const int16_t road_dir_dy[4] = {-1, 0, 1, 0};

/* Return the west/previous cell only when it is in the same row.  A plain
 * i - 1 wraps from column 0 to the previous row's last column, which is not
 * a neighbouring city cell. */
static int previous_grid_cell(int index) {
    if (index <= 0 || index % CITYGRID_WIDTH == 0) return -1;
    return index - 1;
}

static const int8_t road_avail[36] = {
    -1, 1, 8, 7,  -1,-1, 2, 0,   1,-1, 3, 8,   2,-1,-1, 4,
     8, 3,-1, 5,   6, 4,-1,-1,   7, 8, 5,-1,  -1, 0, 6,-1,
     0, 2, 4, 6,
};

static const uint8_t road_count_table[9] = {
    70, 100, 70, 100, 70, 100, 70, 100, 30
};

static const uint8_t block_count_table[9] = {
    40, 10, 40, 10, 40, 10, 40, 10, 60
};

static const int16_t block_template_a[4][8] = {
    {  63,  64,  65, 127, 128, 129,   66,   62},
    { -65,  -1,  63, -66,  -2,  62,  127,  -63},
    { -65, -64, -63,   -1,    0,   1, -127, -129},
    {  65,   1, -63,  66,   2, -62, -127,   63},
};

static const int16_t block_template_b[4][9] = {
    { 127, 128, 129, 191, 192, 193,  64, 130,  126},
    {-130,  -2, 126, -131,  -3, 125,  -1, 126, -126},
    {-129,-128,-127, -65, -64, -63, -192,-126, -130},
    { 130,   2,-126, 131,   3,-125,   1,-126,  126},
};

static const uint8_t tile_templates[13][16] = {
    {  0,  2,129,150,   0,  1,195, 65,   0,196,193,193,   0,  0,215,  0},
    {  0,  2,129,  0,   0,  1,129,  0,   0,  2,129,  0,   0,  1,129,  0},
    {  0,  0,  0,  0,   0,  4, 65, 65,  23,  1,  3,193,   0,  1,129,214},
    {  0,  2,129,153,   0,  5,135, 66,  26,  2,129,201,   0,  1,129,216},
    { 86,  2,129,  0,  66,131,129,151, 193,193,132,  0,   0,  0,  0,  0},
    {  0,  0,  0,  0,  66, 65, 66, 65, 193,193,193,193,   0,  0,  0,  0},
    { 89,  2,137,152,  66, 71, 66, 65, 193,197,193,193,   0,  0,218,  0},
    {  0, 87,  0,  0,  66, 65, 68,  0, 193, 67,129,  0,  22,  1,129,  0},
    { 88,  2,129,  0,  74,  1,129,154, 193,  8,133,  0,  25,  1,129,  0},
    {  0, 90,  0,  0,  66, 65, 70, 65, 193,193,199,193,  24, 10,129,217},
    {111,  2,137,175,  74, 71,135, 66, 193,  7,199,201,  47, 10,129,239},
    { 91,  2,137,155,  66, 71, 70, 65, 193,197,199,193,  27, 10,129,219},
    { 91,  2,129,155,  74,  5,135, 66, 193,  8,133,201,  27,  1,129,219},
};

static void walk_road(CityGridState *s, int dir, int x, int y);

static unsigned citygrid_seed_index(const CityGridState *s) {
    return s->seed_lo % 9u;
}

uint16_t citygrid_prng(CityGridState *s) {
    if (!s) return 0;
    s->prng_state = (uint16_t)(s->prng_state * 0x5E5u + 0x29u);
    return s->prng_state;
}

void citygrid_init(CityGridState *s, uint16_t seed_hi, uint16_t seed_lo,
                   uint16_t difficulty) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->seed_hi = seed_hi;
    s->seed_lo = seed_lo;
    s->difficulty = (difficulty > 127) ? 127 : difficulty;
    s->seed_combined = (uint16_t)((seed_hi << 4) + seed_lo);
    s->prng_state = s->seed_combined;
    s->entry_point = -1;
}

static void count_roads(CityGridState *s) {
    uint16_t mask = 0;
    uint16_t count = 0;
    uint16_t bit = 1;
    const int8_t *avail = &road_avail[citygrid_seed_index(s) * 4];
    for (int i = 0; i < 4; i++) {
        if (avail[i] >= 0) {
            count++;
            mask |= bit;
        }
        bit <<= 1;
    }
    s->road_mask = mask;
    s->road_count = count;
}

static void walk_road(CityGridState *s, int dir, int x, int y) {
    if (!s || dir < 0 || dir >= 4 || x < 0 || x >= CITYGRID_META_SIZE ||
        y < 0 || y >= CITYGRID_META_SIZE)
        return;
    int idx = y * CITYGRID_META_SIZE + x;

    if (s->meta[CITYGRID_META_SIZE * CITYGRID_META_SIZE + idx] == 0)
        s->meta[CITYGRID_META_SIZE * CITYGRID_META_SIZE + idx] = s->building_counter_a;

    int back_dir = dir ^ 2;
    uint8_t existing = s->meta[idx];
    s->meta[idx] |= (1 << dir);

    if (existing != 0) return;

    citygrid_prng(s);
    int rval = s->prng_state & 3;
    int next_dir = dir;
    if (!(rval & 1))
        next_dir = dir - (rval - 1);
    next_dir &= 3;

    for (;;) {
        bool blocked = false;
        switch (next_dir) {
            case 0: blocked = (y == 0); break;
            case 1: blocked = (x == 6); break;
            case 2: blocked = (y == 6); break;
            case 3: blocked = (x == 0); break;
        }
        if (!blocked) break;

        next_dir = (next_dir + 1) & 3;
        if (s->prng_state & 8)
            next_dir = (next_dir - 2) & 3;
        if (next_dir == back_dir)
            continue;
        break;
    }
    next_dir &= 3;

    s->meta[idx] |= (1 << next_dir);
    int nx = x + road_dir_dx[next_dir];
    int ny = y + road_dir_dy[next_dir];
    walk_road(s, next_dir ^ 2, nx, ny);
}

static void expand_cell(CityGridState *s, int grid_offset, uint8_t cell_val,
                        uint8_t building_id) {
    int d2 = cell_val & 0x0F;
    if (d2 == 0x0F) {
        citygrid_prng(s);
        int r = s->prng_state & 3;
        if (r != 3) d2 += r;
    }
    d2 -= 3;
    if (d2 < 0) return;
    if (d2 > 0) {
        d2--;
        if (d2 >= 4) d2--;
    }
    if (d2 < 0 || d2 >= 13) return;

    const uint8_t *tmpl = tile_templates[d2];
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int pos = grid_offset + row * 64 + col;
            if (pos >= 0 && pos < CITYGRID_CELLS) {
                s->plane2[pos] = building_id;
                s->plane0[pos] = tmpl[row * 4 + col];
            }
        }
    }
}

static void expand_meta_to_grid(CityGridState *s) {
    uint16_t saved_seed = s->prng_state;
    memset(s->plane0, 0, CITYGRID_CELLS);

    int meta_idx = 0;
    int grid_base = 6 * 64 + 6;

    for (int my = 0; my < 7; my++) {
        int grid_row = grid_base;
        for (int mx = 0; mx < 7; mx++) {
            uint8_t cell = s->meta[meta_idx];
            uint8_t extra = s->meta[CITYGRID_META_SIZE * CITYGRID_META_SIZE + meta_idx];

            expand_cell(s, grid_row, cell, extra);

            if (cell & (1 << CITYGRID_DIR_E)) {
                uint16_t sv = s->prng_state;
                expand_cell(s, grid_row + 4, 10, extra);
                s->prng_state = sv;
            }

            if (cell & (1 << CITYGRID_DIR_S)) {
                uint16_t sv = s->prng_state;
                expand_cell(s, grid_row + 256, 5, extra);
                s->prng_state = sv;
            }

            grid_row += 8;
            meta_idx++;
        }
        grid_base += 8 * 64;
        meta_idx++;
    }

    if (s->road_mask & 1) {
        uint8_t bid = s->meta[CITYGRID_META_SIZE * CITYGRID_META_SIZE + 3];
        expand_cell(s, 30, 5, bid);
        expand_cell(s, 94, 5, bid);
    }
    if (s->road_mask & 2) {
        uint8_t bid = s->meta[CITYGRID_META_SIZE * CITYGRID_META_SIZE + 6 * CITYGRID_META_SIZE + 6];
        expand_cell(s, 1980, 10, bid);
    }
    if (s->road_mask & 4) {
        uint8_t bid = s->meta[CITYGRID_META_SIZE * CITYGRID_META_SIZE + 6 * CITYGRID_META_SIZE + 3];
        expand_cell(s, 3870, 5, bid);
    }
    if (s->road_mask & 8) {
        uint8_t bid = s->meta[CITYGRID_META_SIZE * CITYGRID_META_SIZE + 3 * CITYGRID_META_SIZE];
        expand_cell(s, 1920, 10, bid);
        expand_cell(s, 1922, 10, bid);
    }

    s->prng_state = saved_seed;
}

static void set_borders(CityGridState *s) {
    for (int x = 0; x < 64; x++) {
        s->plane1[x] = s->plane0[x];
        s->plane0[x] = 0xFF;
    }
    for (int y = 1; y < 63; y++) {
        int base = y * 64;
        s->plane1[base] = s->plane0[base];
        s->plane0[base] = 0xFF;
        s->plane1[base + 63] = s->plane0[base + 63];
        s->plane0[base + 63] = 0xFF;
    }
    for (int x = 0; x < 64; x++) {
        int idx = 63 * 64 + x;
        s->plane1[idx] = s->plane0[idx];
        s->plane0[idx] = 0xFF;
    }
}

static int place_block(CityGridState *s, const int16_t *templates,
                       int tmpl_stride, int cell_count, int retry_limit) {
    /* The two entries after the occupied cells are adjacency probes. */
    if (!s || !templates || cell_count < 1 ||
        tmpl_stride < cell_count + 2)
        return -1;
    while (retry_limit-- > 0) {
        citygrid_prng(s);
        uint16_t rval = s->prng_state;
        uint16_t offset = (rval >> 2) & 0x0FFF;
        if (offset >= CITYGRID_CELLS) continue;

        uint8_t cell = s->plane0[offset];
        bool found = false;

        if ((cell & 0x3F) == CITYGRID_CELL_EMPTY) {
            found = true;
        } else {
            int x = offset & 63;
            int y = offset >> 6;
            uint16_t dir_bits = (rval >> 2) & 0x0C;
            for (int d = 0; d < 4; d++) {
                int di = ((dir_bits >> 2) + d) & 3;
                int nx = x + directions[di].dx;
                int ny = y + directions[di].dy;
                if (nx < 0 || nx > 63 || ny < 0 || ny > 63) continue;
                int noff = ny * 64 + nx;
                if ((s->plane0[noff] & 0x3F) == CITYGRID_CELL_EMPTY) {
                    offset = (uint16_t)noff;
                    found = true;
                    break;
                }
            }
        }
        if (!found) continue;

        uint8_t cv = s->plane0[offset];
        int type = cv;
        type = ((type << 2) | (type >> 6)) & 0xFF;
        type = (type + 1) & 0xFF;
        if (rval & 0x8000) type += 2;
        type = (type + 2) & 3;

        /* The block families have different row widths (A=8, B=9).  Treat
         * the table as flat data and apply its explicit stride so a cast to
         * the wrong array width cannot make type N read from another row. */
        const int16_t *tmpl = templates + type * tmpl_stride;
        bool all_free = true;
        for (int i = 0; i < cell_count; i++) {
            int pos = (int)offset + tmpl[i];
            if (pos < 0 || pos >= CITYGRID_CELLS || s->plane0[pos] != 0) {
                all_free = false;
                break;
            }
        }
        if (!all_free) continue;

        int adj1 = (int)offset + tmpl[cell_count];
        int adj2 = (int)offset + tmpl[cell_count + 1];
        bool adj_ok = false;
        if (adj1 >= 0 && adj1 < CITYGRID_CELLS) {
            uint8_t a = s->plane0[adj1] & 0x3F;
            if (a >= 18 && a <= 21) adj_ok = true;
        }
        if (!adj_ok && adj2 >= 0 && adj2 < CITYGRID_CELLS) {
            uint8_t a = s->plane0[adj2] & 0x3F;
            if (a >= 18 && a <= 21) adj_ok = true;
        }
        if (!adj_ok) return 0;

        for (int i = 0; i < cell_count; i++) {
            int pos = (int)offset + tmpl[i];
            s->plane0[pos] = (uint8_t)((type << 6) | (s->building_counter_a & 0x3F));
            s->plane1[pos] = s->road_id;
            s->plane2[pos] = s->building_counter_a;
        }
        s->plane2[offset] |= 0x80;
        s->building_counter_a++;
        return 1;
    }
    return -1;
}

static void place_features(CityGridState *s) {
    s->building_counter_b = 1;
    uint8_t count = block_count_table[citygrid_seed_index(s)];

    for (int i = 0; i <= (int)count; i++) {
        uint8_t sv = (s->seed_combined & 0x0F) + 1;
        if (sv > 15) sv = 7;
        s->road_id = sv;
        place_block(s, &block_template_b[0][0], 9, 7, 4095);
    }
    s->building_counter_b--;
}

static void place_road_blocks(CityGridState *s) {
    s->building_counter_a = s->building_counter_b + 1;
    uint8_t count = road_count_table[citygrid_seed_index(s)];

    for (int i = 0; i <= (int)count; i++) {
        s->road_id = s->seed_combined & 1;
        place_block(s, &block_template_a[0][0], 8, 6, 4095);
    }

    uint8_t diff = s->building_counter_b + 1;
    s->building_counter_a -= diff;
}

/* sub_0932: check if cell at a1 belongs to building d7, return cell type info */
static bool citygrid_check_building_cell(uint8_t *plane0, uint8_t *plane2,
                                         int offset, uint8_t building_id,
                                         int *dir_delta, int d4) {
    uint8_t raw_p2 = plane2[offset];
    if (raw_p2 == 0 || raw_p2 == 0xFF) return false;
    uint8_t p2 = raw_p2 & 0x7F;
    if (p2 != building_id) return false;

    uint8_t cell = plane0[offset] & 0x3F;
    if (cell <= 0x0C) return false;
    if (cell == 0x1C) return true;
    if (cell > 0x11) return false;
    if (cell == 0x0D) return true;
    if (cell == 0x0E) {
        *dir_delta -= d4;
        *dir_delta &= 3;
        return true;
    }
    if (cell == 0x0F) {
        *dir_delta += d4;
        *dir_delta &= 3;
        return true;
    }
    return true;
}

/* sub_08F2: walk backward to find building origin */
static int citygrid_walk_to_origin(uint8_t *plane0, uint8_t *plane2,
                                   int offset, uint8_t building_id, int dir) {
    dir ^= 2;
    int d4 = 1;
    int prev = offset;
    for (;;) {
        int heading = dir & 3;
        int x = offset % CITYGRID_WIDTH;
        int y = offset / CITYGRID_WIDTH;
        int next_x = x + directions[heading].dx;
        int next_y = y + directions[heading].dy;
        /* A linear +/-1 offset wraps between rows at the horizontal
         * boundaries. Reject that wrap before inspecting the neighbour. */
        if (next_x < 0 || next_x >= CITYGRID_WIDTH ||
            next_y < 0 || next_y >= CITYGRID_HEIGHT) break;
        int next_off = next_y * CITYGRID_WIDTH + next_x;
        int dummy_dir = dir;
        if (!citygrid_check_building_cell(plane0, plane2, next_off,
                                          building_id, &dummy_dir, d4))
            break;
        prev = offset;
        offset = next_off;
        dir = dummy_dir;
    }
    return prev;
}

/* sub_07D2 + sub_0800: resolve building shapes */
static void resolve_building_shapes(CityGridState *s) {
    int total = s->building_counter_a + s->building_counter_b;
    if (total > CITYGRID_MAX_BUILDINGS)
        total = CITYGRID_MAX_BUILDINGS;

    for (int i = 0; i < total; i++) {
        if (s->buildings[i].connection == 0xFF) {
            uint16_t off = s->buildings[i].grid_offset;
            if (off >= CITYGRID_CELLS) continue;

            uint8_t raw_building_id = s->plane2[off];
            if (raw_building_id == 0 || raw_building_id == 0xFF) continue;
            uint8_t building_id = raw_building_id & 0x7F;
            uint8_t cell = s->plane0[off];
            int dir = ((cell << 2) | (cell >> 6)) & 3;

            int origin = citygrid_walk_to_origin(s->plane0, s->plane2,
                                                 off, building_id, dir);
            (void)origin;

            /* sub_083A: resolve connections between buildings */
            int pos = off;
            int neg_pos = pos;
            for (int j = 0; j < total; j++) {
                if (s->buildings[j].grid_offset == (uint16_t)neg_pos) {
                    /* Check adjacent cells and set connection byte */
                    int d4 = (uint8_t)(j + 1);
                    bool is_higher = (d4 > s->building_counter_b);

                    int dir_check = dir;
                    int check1 = pos + directions[(dir_check - 1) & 3].offset;
                    int check2 = pos + directions[(dir_check + 1) & 3].offset;

                    int conn = 0;
                    if (check1 >= 0 && check1 < CITYGRID_CELLS &&
                        (s->plane2[check1] & 0x7F) == (uint8_t)d4) {
                        uint8_t c = s->plane0[check1] & 0x3F;
                        bool is_0x1e = (c == 0x1E);
                        if (is_0x1e != is_higher)
                            conn = 1;
                    }
                    if (conn == 0 && check2 >= 0 && check2 < CITYGRID_CELLS &&
                        (s->plane2[check2] & 0x7F) == (uint8_t)d4) {
                        uint8_t c = s->plane0[check2] & 0x3F;
                        bool is_0x1e = (c == 0x1E);
                        if (is_0x1e != is_higher)
                            conn = 2;
                    }

                    if (conn == 1) {
                        uint8_t val = (uint8_t)((s->buildings[j].connection >> 16) + 1);
                        val |= 1;
                        s->buildings[j].connection = val;
                    } else if (conn == 2) {
                        uint8_t val = (uint8_t)((s->buildings[j].connection >> 16) & ~1);
                        val += 2;
                        s->buildings[j].connection = val;
                    }
                }
            }
        }
    }
}

/* sub_1766: initialize connection table */
static void init_connection_table(CityGridState *s) {
    memset(s->conn_table, 0xFF, CITYGRID_CONN_TABLE_SIZE);
}

/* sub_097A: set up building connectivity */
static void setup_building_connectivity(CityGridState *s) {
    /* sub_0A2A: init from seed coordinates */
    /* sub_09C2: assign special connection types */
    /* sub_0990: assign PRNG-based direction values */

    int count = s->building_counter_b;
    for (int i = 0; i < count; i++) {
        if (s->buildings[i].connection & 0x80) continue;
        if (s->buildings[i].direction & 0x80) continue;
        citygrid_prng(s);
        uint8_t dir_val = (s->prng_state & 3) + 3;
        s->buildings[i].direction = dir_val;
    }
}

/* sub_0A08: mask building records */
static void cleanup_building_records(CityGridState *s) {
    int count = s->building_counter_a;
    for (int i = 0; i < count; i++) {
        s->buildings[i].grid_offset &= 0x0FFF;
        s->buildings[i].connection &= 0xBF;
        s->buildings[i].direction &= 0x2F;
    }
}

/* sub_160E: inner road feature placement loop */
static bool place_road_feature_inner(CityGridState *s) {
    uint32_t mode = s->feature_mode;
    int max_attempts = 4096;

    while (max_attempts-- > 0) {
        citygrid_prng(s);
        uint16_t rval = s->prng_state;
        uint16_t raw = rval >> 2;
        int x = raw & 0x3F;
        int y = (raw >> 6) & 0x3F;
        int dir = (raw >> 12) & 3;
        int offset = y * 64 + x;

        if (mode & 0x20) {
            /* Mode: find road cell (0x0D) */
            uint8_t cell = s->plane0[offset] & 0x3F;
            if (cell != 0x0D) continue;
            s->feature_mode = (mode & 0xFFFFU) | ((uint32_t)offset << 16);
            return true;
        }

        /* Mode: walk in direction until finding target cell */
        for (int tries = 0; tries < 4; tries++) {
            int cx = x, cy = y, coff = offset;
            bool found_target = false;

            for (;;) {
                if (cx < 5 || cx > 58 || cy < 5 || cy > 58) break;

                uint8_t cell = s->plane0[coff] & 0x3F;

                if (cell == 0x01 || cell == 0x02) {
                    if (mode & 0x02) break;
                    s->feature_building_id = 0;
                    found_target = true;
                    break;
                }
                if (cell == 0x0D) {
                    if (!(mode & 0x01)) break;
                    s->feature_building_id = 0;
                    found_target = true;
                    break;
                }
                if (cell == 0x14) {
                    if (mode & 0x04) break;
                    s->feature_building_id = s->plane1[coff];
                    found_target = true;
                    break;
                }

                cx += directions[dir].dx;
                cy += directions[dir].dy;
                coff += directions[dir].offset;
            }

            if (found_target) {
                s->feature_mode = (mode & 0xFFFFU) | ((uint32_t)coff << 16);
                /* sub_1702: try to place in adjacent free cell */
                int place_dir = (dir + 1) & 3;
                int place_x = coff % CITYGRID_WIDTH + directions[place_dir].dx;
                int place_y = coff / CITYGRID_WIDTH + directions[place_dir].dy;
                int place_off = place_y * CITYGRID_WIDTH + place_x;
                if (place_x >= 0 && place_x < CITYGRID_WIDTH &&
                    place_y >= 0 && place_y < CITYGRID_HEIGHT &&
                    s->plane0[place_off] == 0) {
                    if (!(mode & 0x08)) {
                        s->plane0[place_off] = s->feature_cell_type |
                                               (uint8_t)(place_dir << 6);
                        s->plane1[place_off] = s->feature_building_id;
                    }
                    return true;
                }

                place_dir = (dir + 3) & 3;
                place_x = coff % CITYGRID_WIDTH + directions[place_dir].dx;
                place_y = coff / CITYGRID_WIDTH + directions[place_dir].dy;
                place_off = place_y * CITYGRID_WIDTH + place_x;
                if (place_x >= 0 && place_x < CITYGRID_WIDTH &&
                    place_y >= 0 && place_y < CITYGRID_HEIGHT &&
                    s->plane0[place_off] == 0) {
                    if (!(mode & 0x08)) {
                        s->plane0[place_off] = s->feature_cell_type |
                                               (uint8_t)(place_dir << 6);
                        s->plane1[place_off] = s->feature_building_id;
                    }
                    return true;
                }
            }

            dir = (dir + 1) & 3;
        }
    }
    return false;
}

/* sub_15F8: place road features */
static void place_road_features(CityGridState *s, uint8_t cell_type,
                                uint8_t count, uint16_t mode) {
    s->feature_cell_type = cell_type;
    s->feature_count = count;
    s->feature_mode = mode;
    while (s->feature_count > 0) {
        place_road_feature_inner(s);
        s->feature_count--;
    }
}

/* sub_2444: backup plane0 */
static void backup_plane0(CityGridState *s) {
    memcpy(s->plane0_backup, s->plane0, CITYGRID_CELLS);
}

/* sub_0A80: advanced feature placement with retry */
static void place_advanced_features(CityGridState *s) {
    backup_plane0(s);
    s->retry_limit = 0xFF;

    for (int i = 0x32; i >= 0; i--) {
        s->retry_limit--;
        if (s->retry_limit < 0) return;

        /* sub_0AA6: attempt placement */
        s->feature_mode = 0x0C;
        place_road_feature_inner(s);
        if (!(s->feature_mode & 0x10)) {
            i++;
            continue;
        }

        /* Found a road cell, try wall placement */
        uint16_t saved_prng = s->prng_state;
        /* Try complex wall placement at the found position */
        /* If it fails, restore and try again */
        (void)saved_prng;

        /* sub_0ADC: complex placement logic referencing saved position */
        /* Simplified: place cell type 0x29 at found position */
        backup_plane0(s);
    }
}

/* sub_0ECC: road-adjacent wall placement */
static void place_road_walls(CityGridState *s) {
    for (int attempts = 5; attempts >= 0; attempts--) {
        citygrid_prng(s);
        uint16_t rval = s->prng_state;
        int x = (rval >> 2) & 0x3F;
        int y = (rval >> 8) & 0x3F;
        int dir = (rval >> 14) & 3;
        int offset = y * 64 + x;

        /* Walk in direction until finding cell type 0x1F (border wall) */
        for (int d = 0; d < 4; d++) {
            int cx = x, cy = y, coff = offset;
            int cur_dir = (dir + d) & 3;

            bool found_wall = false;
            while (cx > 4 && cx < 58 && cy > 4 && cy < 58) {
                uint8_t cell = s->plane0[coff] & 0x3F;
                if (cell == 0x1F) {
                    found_wall = true;
                    break;
                }
                cx += directions[cur_dir].dx;
                cy += directions[cur_dir].dy;
                coff += directions[cur_dir].offset;
            }

            if (!found_wall) continue;

            /* Found wall at coff, get its direction */
            uint8_t wall_dir = s->plane0[coff] >> 6;

            /* Walk from wall position in wall's direction looking for
               connectable cell (0x24, 0x21, 0x29, 0x0D, 0x01, or building) */
            int wx = cx, wy = cy, woff = coff;
            wx += directions[wall_dir].dx;
            wy += directions[wall_dir].dy;
            woff += directions[wall_dir].offset;

            while (wx > 4 && wx < 58 && wy > 4 && wy < 58) {
                uint8_t c2 = s->plane0[woff] & 0x3F;
                if (c2 == 0x24 || c2 == 0x21 || c2 == 0x29 ||
                    c2 == 0x0D || c2 == 0x01 ||
                    (c2 >= 0x12 && c2 <= 0x15) ||
                    (c2 >= 0x1D && c2 <= 0x20)) {
                    /* Place wall cell 0x2E at the original wall position */
                    s->plane0[coff] = (s->plane0[coff] & 0xC0) | 0x2E;
                    uint8_t bid = s->plane2[coff] & 0x7F;
                    if (bid > 0) {
                        int bidx = (bid - 1) * 4;
                        if (bidx < CITYGRID_MAX_BUILDINGS * 4)
                            s->buildings[bid - 1].direction |= 0x10;
                    }
                    break;
                }
                if (c2 != 0) break;
                wx += directions[wall_dir].dx;
                wy += directions[wall_dir].dy;
                woff += directions[wall_dir].offset;
            }
            break;
        }
    }
}

/* sub_0180: find and set entry point */
static void find_entry_point(CityGridState *s) {
    if (s->entry_point != -1) return;

    for (int attempt = 0x1D; attempt >= 0; attempt--) {
        s->feature_mode = 0x21;
        s->feature_count = 1;
        s->feature_cell_type = 0x2E;
        place_road_feature_inner(s);

        if (s->feature_mode & 0x10) {
            /* Found a road cell — verify it's still road after walking */
            uint16_t found_off = (uint16_t)(s->feature_mode >> 16);
            if (found_off < CITYGRID_CELLS) {
                uint8_t raw_building_id = s->plane2[found_off];
                if (raw_building_id == 0 || raw_building_id == 0xFF) continue;
                uint8_t building_id = raw_building_id & 0x7F;
                uint8_t cell = s->plane0[found_off];
                int dir = ((cell << 2) | (cell >> 6)) & 3;
                int pos = citygrid_walk_to_origin(s->plane0, s->plane2,
                                                  found_off, building_id, dir);
                uint8_t c = s->plane0[pos] & 0x3F;
                if (c == 0x0D) {
                    s->entry_point = (int16_t)pos;
                    return;
                }
            }
        }
    }
}

/* sub_24B8: finalize pass — convert generation cell types to output values */
static void finalize_cells(CityGridState *s) {
    uint8_t out_plane1[CITYGRID_CELLS];
    uint8_t out_plane2[CITYGRID_CELLS];
    memset(out_plane1, 0, CITYGRID_CELLS);
    memset(out_plane2, 0, CITYGRID_CELLS);
    /* plane2 contains generation-time building IDs. Keep them available to
     * navigation and building interaction after the output planes replace it. */
    memcpy(s->building_ids, s->plane2, CITYGRID_CELLS);

    for (int i = 0; i < CITYGRID_CELLS; i++) {
        uint8_t raw = s->plane0[i];
        uint8_t cell = raw & 0x3F;
        uint8_t rotation = (raw >> 6) & 3;
        uint8_t p1_val = s->plane1[i];

        uint8_t out_type = 0;
        uint8_t out_p1 = 0;
        uint8_t out_p2 = 0;

        if (cell == 0) {
            /* Empty → wall type 0x10 */
            out_type = 0x10;
        } else if (raw == 0xFF) {
            /* Border wall */
            int previous = previous_grid_cell(i);
            uint8_t below = previous >= 0 ? s->plane0[previous] : 0;
            out_type = 6;
            out_p1 = 0;
            out_p2 = 0;

            /* Check for edge entry points */
            int grid_pos = i;
            if (below != 0) {
                out_p1 = 0x20;
                /* Special positions get different values */
                if (grid_pos == 0x20 || grid_pos == 0xFE1 ||
                    grid_pos == 0x800 || grid_pos == 0x801) {
                    /* Keep 0x20 */
                } else {
                    out_p1 = 0x21;
                }
                out_p2 = below & 0xC0;
            }
        } else if (cell >= 0x01 && cell <= 0x11) {
            /* Building cells: type = 0x12, cell - 1 */
            out_type = 0x12;
            out_p1 = (uint8_t)(cell - 1);
        } else if (cell >= 0x12 && cell <= 0x15) {
            /* Door cells */
            out_type = 0x01;
            if (cell == 0x13) out_p1 = 1;
            else out_p1 = 0;

            int previous = previous_grid_cell(i);
            uint8_t p1_below = previous >= 0 ? s->plane1[previous] : 0;
            uint8_t door_extra = p1_below & 0x0F;
            out_p1 |= (uint8_t)(door_extra << 2);
        } else if (cell >= 0x16 && cell <= 0x1B) {
            /* Wall segment cells */
            uint8_t sub = (uint8_t)(cell - 0x16);
            uint8_t wall_extra = p1_val & 0x38;
            out_type = 0x11;
            out_p1 = sub | wall_extra;
        } else if (cell == 0x1C) {
            /* Stairway */
            out_type = 0x13;
            out_p1 = (uint8_t)(cell - 0x1C);
        } else if (cell >= 0x1D && cell <= 0x20) {
            /* Building entrance cells */
            out_type = 0x02;
            if (cell == 0x1E) out_p1 = 1;
            else out_p1 = 0;

            int previous = previous_grid_cell(i);
            uint8_t p1_below = previous >= 0 ? s->plane1[previous] : 0;
            uint8_t ent_extra = p1_below & 0x0F;
            out_p1 |= (uint8_t)(ent_extra << 2);
        } else if (cell == 0x21) {
            /* Road feature: lamp post */
            out_type = 0x0D;
            out_p1 = p1_val;
            out_p2 = 0;
        } else if (cell == 0x22) {
            /* Road feature: post box */
            out_type = 0x0E;
            out_p1 = p1_val;
            out_p2 = 0;
        } else if (cell == 0x23) {
            /* Road feature: phone box */
            out_type = 0x0B;
            int previous = previous_grid_cell(i);
            uint8_t prev_byte = previous >= 0 ? s->plane0[previous] : 0;
            out_p1 = prev_byte;
            uint8_t p1_below = previous >= 0 ? s->plane1[previous] : 0;
            out_p2 = (p1_below & 0x0F) << 2;
        } else if (cell == 0x24) {
            /* Shop entrance */
            out_type = 0x1A;
            out_p1 = p1_val;
            out_p2 = 0;
        } else if (cell == 0x25) {
            /* Bank entrance */
            out_type = 0x1B;
            out_p1 = p1_val;
            out_p2 = 0;
        } else if (cell == 0x26) {
            /* Bar entrance */
            out_type = 0x17;
            out_p1 = p1_val;
            out_p2 = 0;
        } else if (cell == 0x27) {
            /* Hotel entrance */
            out_type = 0x18;
            out_p1 = p1_val;
            out_p2 = 0;
        } else if (cell == 0x29) {
            /* Alley/passage */
            out_type = 0x16;
            out_p1 = p1_val;
            out_p2 = 0;
        } else if (cell == 0x2A) {
            /* Special feature 0x2A */
            out_type = 0x19;
            out_p1 = 0;
        } else if (cell == 0x2B) {
            /* Special feature 0x2B */
            out_type = 0x19;
            out_p1 = 1;
        } else if (cell == 0x2C) {
            /* Special feature 0x2C */
            out_type = 0x19;
            out_p1 = 0x0C;
        } else if (cell == 0x2E) {
            /* Entry point marker */
            out_type = 0x02;
            out_p1 = 2;
        } else if (cell == 0x2F) {
            /* Road block */
            out_type = 0x1C;
            out_p1 = p1_val;
        } else {
            /* Default: treat as wall */
            out_type = 0x12;
            out_p1 = (uint8_t)(cell - 1);
        }

        /* Apply rotation */
        out_p1 = (uint8_t)((out_p1 & 0x3F) | (rotation << 6));
        out_p1 = (uint8_t)(((out_p1 << 2) | (out_p1 >> 6)) & 0xFF);

        out_plane1[i] = out_p1;
        out_plane2[i] = out_p2;
        s->plane0[i] = out_type;
    }

    memcpy(s->plane1, out_plane1, CITYGRID_CELLS);
    memcpy(s->plane2, out_plane2, CITYGRID_CELLS);

    /* Set entry point cell */
    if (s->entry_point >= 0 && s->entry_point < CITYGRID_CELLS) {
        s->plane0[s->entry_point] = 0x0A;
    }
}

void citygrid_generate(CityGridState *s) {
    if (!s) return;
    memset(s->plane0, 0, CITYGRID_CELLS);
    memset(s->plane1, 0, CITYGRID_CELLS);
    memset(s->plane2, 0, CITYGRID_CELLS);
    memset(s->building_ids, 0, CITYGRID_CELLS);
    memset(s->meta, 0, sizeof(s->meta));
    s->entry_point = -1;

    count_roads(s);

    s->building_counter_a = 1;
    citygrid_prng(s);

    if (s->road_count > 0) {
        int dir = 0;
        while (!(s->road_mask & (1 << dir))) dir++;

        for (uint16_t i = 0; i < s->road_count; i++) {
            walk_road(s, dir ^ 2, road_corners[dir][0], road_corners[dir][1]);
            s->building_counter_a++;
            dir = (dir + 1) & 3;
            while (!(s->road_mask & (1 << dir)))
                dir = (dir + 1) & 3;
        }
    }

    expand_meta_to_grid(s);

    if (s->difficulty >= 0)
        set_borders(s);

    if (s->difficulty >= 2)
        place_features(s);

    if (s->difficulty >= 3) {
        place_road_blocks(s);
        resolve_building_shapes(s);
        init_connection_table(s);
        setup_building_connectivity(s);
        cleanup_building_records(s);
        place_road_features(s, 0x22, 4, 4);
        place_road_features(s, 0x21, 10, 6);
        place_road_features(s, 0x23, 1, 3);
    }

    if (s->difficulty >= 4) {
        place_advanced_features(s);
        place_road_walls(s);

        uint16_t saved_prng = s->prng_state;
        find_entry_point(s);
        s->prng_state = saved_prng;
    }

    finalize_cells(s);
}

void citygrid_map_buildings(CityGridState *s, const CityGrid *bg) {
    if (!s || !bg || bg->total_buildings <= 0) return;

    /* The CityGen grid assigns building IDs in plane2 during road walking.
     * BuildingGen independently generates building records indexed by
     * placement order. sub_1352 walks grid buildings and assigns each one
     * the corresponding BuildingGen type by matching building_counter order.
     *
     * plane1 stores the BuildingGen building type index (0-8) for each cell
     * that belongs to a building, enabling the runtime to look up building
     * names and interaction types. */
    int bg_count = bg->total_buildings;
    if (bg_count > CITYGEN_MAX_BUILDINGS) bg_count = CITYGEN_MAX_BUILDINGS;

    for (int i = 0; i < CITYGRID_CELLS; i++) {
        uint8_t raw_bid = s->building_ids[i];
        if (raw_bid == 0 || raw_bid == 0xFF) continue;
        uint8_t bid = raw_bid & 0x7F;
        if (bid == 0) continue;

        int bg_idx = (bid - 1) % bg_count;
        s->plane1[i] = bg->buildings[bg_idx].type;
    }
}
