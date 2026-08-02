#include "liberation_citygen_grid.h"
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

/* Forward declarations */
static void walk_road(CityGridState *s, int dir, int x, int y);

uint16_t citygrid_prng(CityGridState *s) {
    s->prng_state = (uint16_t)(s->prng_state * 0x5E5u + 0x29u);
    return s->prng_state;
}

void citygrid_init(CityGridState *s, uint16_t seed_hi, uint16_t seed_lo,
                   uint16_t difficulty) {
    memset(s, 0, sizeof(*s));
    s->seed_hi = seed_hi;
    s->seed_lo = seed_lo;
    s->difficulty = (difficulty > 127) ? 127 : difficulty;
    s->seed_combined = (uint16_t)((seed_hi << 4) + seed_lo);
    s->prng_state = s->seed_combined;
}

static void count_roads(CityGridState *s) {
    uint16_t mask = 0;
    uint16_t count = 0;
    uint16_t bit = 1;
    const int8_t *avail = &road_avail[s->seed_lo * 4];
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

static int place_block(CityGridState *s, const int16_t templates[][8],
                       int tmpl_stride, int cell_count, int retry_limit) {
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

        const int16_t *tmpl = templates[type];
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
    uint8_t count = block_count_table[s->seed_lo];

    for (int i = 0; i <= (int)count; i++) {
        uint8_t sv = (s->seed_combined & 0x0F) + 1;
        if (sv > 15) sv = 7;
        s->road_id = sv;
        place_block(s, (const int16_t(*)[8])block_template_b, 9, 7, 4095);
    }
    s->building_counter_b--;
}

static void place_road_blocks(CityGridState *s) {
    s->building_counter_a = s->building_counter_b + 1;
    uint8_t count = road_count_table[s->seed_lo];

    for (int i = 0; i <= (int)count; i++) {
        s->road_id = s->seed_combined & 1;
        place_block(s, (const int16_t(*)[8])block_template_a, 8, 6, 4095);
    }

    uint8_t diff = s->building_counter_b + 1;
    s->building_counter_a -= diff;
}

void citygrid_generate(CityGridState *s) {
    memset(s->plane0, 0, CITYGRID_CELLS);
    memset(s->plane1, 0, CITYGRID_CELLS);
    memset(s->plane2, 0, CITYGRID_CELLS);
    memset(s->meta, 0, sizeof(s->meta));

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

    if (s->difficulty >= 3)
        place_road_blocks(s);
}
