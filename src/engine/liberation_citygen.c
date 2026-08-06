#include "liberation_citygen.h"
#include "i18n.h"
#include "liberation_data.h"
#include <string.h>
#include <stdio.h>

uint16_t citygen_prng(uint16_t *state) {
    if (!state) return 0;
    *state = (uint16_t)(*state * 0x5E5u + 0x29u);
    return *state;
}

/* Rotate left 16-bit */
static uint16_t rol16(uint16_t val, int n) {
    n &= 15;
    if (n == 0) return val;
    return (uint16_t)((val << n) | (val >> (16 - n)));
}

/* Rotate right 16-bit */
static uint16_t ror16(uint16_t val, int n) {
    n &= 15;
    if (n == 0) return val;
    return (uint16_t)((val >> n) | (val << (16 - n)));
}

void citygen_init(CityGrid *grid, uint16_t seed, uint16_t level) {
    if (!grid) return;
    memset(grid, 0, sizeof(*grid));
    grid->seed = seed;
    grid->level = level;
    grid->prng_state = seed;
}

/*
 * Grid parameter computation from BuildingGen 0x548-0x68A.
 *
 * density        = level * 5 + 15 + (PRNG_roll & 7); cap 100
 * num_columns    = (level >> 4) + 6 + (seed_roll & 3) - 2; cap 15
 * num_roads      = 2 + (seed_ror & 3) - (level >> 3); min 1; if level==0 then 5
 * num_cross_roads = (seed_ror & 1) + (level >> 3) + 1; cap 5
 *
 * Then derived:
 *   road_buildings = num_cross_roads/2 (or num_cross_roads if /2==0)
 *   side_buildings = min(num_roads * 2, 5)
 *   buildings_per_segment = num_columns/2 (or num_columns if /2==0)
 *   total = side + road + segment; adjusted to fit
 */
void citygen_compute_params(CityGrid *grid) {
    if (!grid) return;
    uint16_t seed = grid->seed;
    uint16_t level = grid->level;

    /* density: 0x560-0x58C */
    uint16_t d1 = seed & 0x0F;
    uint16_t r = citygen_prng(&grid->prng_state);
    uint16_t d2 = rol16(r, d1) & 7;
    uint32_t density = (uint32_t)level * 5U + 15U + d2;
    if (density > 100U) density = 100U;
    grid->density = (uint16_t)density;

    /* num_columns: 0x590-0x5B2 */
    d1 += 8;
    uint16_t cols = ror16(seed, d1) & 3;
    int16_t col_val = (int16_t)cols - 2;
    int16_t nc = (int16_t)((level >> 4) + 6) + col_val;
    if (nc > 15) nc >>= 1;
    while (nc > 15) nc >>= 1;
    grid->num_columns = (uint16_t)nc;

    /* num_roads: 0x5B6-0x5E2 */
    d2 = seed & 7;
    int16_t nr = (int16_t)(2 + (ror16(seed, d2) & 3)) - (int16_t)(level >> 3);
    if (nr <= 0) nr = 1;
    if (level == 0) nr = 5;
    grid->num_roads = (uint16_t)nr;

    /* num_cross_roads: 0x5E6-0x60C */
    d2 = seed & 3;
    uint16_t ncr = (ror16(ror16(seed, 8), d2) & 1) + (level >> 3) + 1;
    if (ncr > 5) ncr = 5;
    grid->num_cross_roads = ncr;

    /* derived: 0x610-0x654 */
    uint16_t rb = ncr >> 1;
    if (rb == 0) rb = ncr;
    grid->road_buildings = rb;

    uint16_t sb = (uint16_t)nr * 2;
    if (sb > 5) sb = 5;
    grid->side_buildings = sb;

    uint16_t bps = grid->num_columns >> 1;
    if (bps == 0) bps = grid->num_columns;
    grid->buildings_per_segment = bps;

    /* total: 0x658-0x68A */
    uint16_t total = density >> 1;
    if (total < rb + 1) total = rb + 1;

    if (total < (uint16_t)(sb + rb) ||
        (bps > 0 && (total - sb - rb) / bps == 0)) {
        total = sb + rb + bps + 1;
    }
    grid->total_buildings = total;
}

/*
 * Building placement from BuildingGen 0x68C-0x900.
 *
 * Creates a grid of buildings connected by roads. The grid has
 * cross_roads columns and roads rows. Each building is a 36-byte
 * record. Buildings are connected via forward (0xAAAA) and
 * backward (0xBBBB) links.
 */
void citygen_place_buildings(CityGrid *grid) {
    if (!grid) return;
    uint16_t total = grid->total_buildings;
    if (total > CITYGEN_MAX_BUILDINGS) total = CITYGEN_MAX_BUILDINGS;
    grid->total_buildings = total;

    /* Phase 1: Create road spine (0x6A6-0x6FE)
     * Place buildings along main road with diagonal connections */
    int idx = 0;
    int roads = grid->num_roads;

    /* Road spine: each road is a chain of buildings */
    for (int road = 0; road < roads && idx < total; road++) {
        int road_start = idx;
        for (int col = 0; col < grid->num_columns && idx < total; col++) {
            grid->buildings[idx].flags = 0;
            idx++;
        }

        /* Connect buildings along this road */
        for (int col = 0; col + 1 < (idx - road_start); col++) {
            int a = road_start + col;
            int b = road_start + col + 1;
            uint8_t ca = grid->buildings[a].flags & 3;
            if (ca < 3) {
                grid->buildings[a].connections[ca].marker = CONN_FORWARD;
                grid->buildings[a].connections[ca].target_index = (uint16_t)b;
                grid->buildings[a].flags = (grid->buildings[a].flags & 0xFC) | (ca + 1);
            }
            uint8_t cb = grid->buildings[b].flags & 3;
            if (cb < 3) {
                grid->buildings[b].connections[cb].marker = CONN_BACKWARD;
                grid->buildings[b].connections[cb].target_index = (uint16_t)a;
                grid->buildings[b].flags = (grid->buildings[b].flags & 0xFC) | (cb + 1);
            }
        }
    }

    /* Phase 2: Cross-road connections (0x706-0x978)
     * Connect buildings between adjacent roads */
    for (int col = 0; col < grid->num_columns && col < total; col++) {
        for (int road = 0; road + 1 < roads; road++) {
            int a_idx = road * grid->num_columns + col;
            int b_idx = (road + 1) * grid->num_columns + col;
            if (a_idx >= total || b_idx >= total) continue;

            uint8_t ca = grid->buildings[a_idx].flags & 3;
            uint8_t cb = grid->buildings[b_idx].flags & 3;
            if (ca < 3 && cb < 3) {
                grid->buildings[a_idx].connections[ca].marker = CONN_FORWARD;
                grid->buildings[a_idx].connections[ca].target_index = (uint16_t)b_idx;
                grid->buildings[a_idx].flags = (grid->buildings[a_idx].flags & 0xFC) | (ca + 1);

                grid->buildings[b_idx].connections[cb].marker = CONN_BACKWARD;
                grid->buildings[b_idx].connections[cb].target_index = (uint16_t)a_idx;
                grid->buildings[b_idx].flags = (grid->buildings[b_idx].flags & 0xFC) | (cb + 1);
            }
        }
    }

    /* Phase 3: Random extra connections (0x992-0xA72)
     * Add random connections to fill remaining slots */
    if (grid->num_roads == 0 || total == 0) return;
    for (int attempts = 4095; attempts > 0; attempts--) {
        uint16_t r = citygen_prng(&grid->prng_state);
        int target = (ror16(r, 2) & 0xFFFF) % grid->num_roads + grid->num_cross_roads;
        if (target >= (int)total) continue;

        uint8_t cc = grid->buildings[target].flags & 3;
        if (cc >= 3) continue;

        /* Find a random unconnected building to link to */
        int src = (citygen_prng(&grid->prng_state) & 0xFFFF) % total;
        if (src == target) continue;

        uint8_t sc = grid->buildings[src].flags & 3;
        if (sc >= 3) continue;

        grid->buildings[target].connections[cc].marker = CONN_FORWARD;
        grid->buildings[target].connections[cc].target_index = (uint16_t)src;
        grid->buildings[target].flags = (grid->buildings[target].flags & 0xFC) | (cc + 1);

        grid->buildings[src].connections[sc].marker = CONN_BACKWARD;
        grid->buildings[src].connections[sc].target_index = (uint16_t)target;
        grid->buildings[src].flags = (grid->buildings[src].flags & 0xFC) | (sc + 1);
    }
}

/*
 * Building type assignment from BuildingGen 0x1A60-0x1AE2.
 *
 * Each building gets a type (PRNG % 9), a sequential ID,
 * and a name seed.
 */
void citygen_assign_types(CityGrid *grid) {
    if (!grid) return;
    if (grid->total_buildings > CITYGEN_MAX_BUILDINGS)
        grid->total_buildings = CITYGEN_MAX_BUILDINGS;
    /* Seed scramble: ror 4 before assignment (0x1A80) */
    grid->prng_state = ror16(grid->prng_state, 4);

    uint16_t seq_id = 0;

    for (int i = 0; i < (int)grid->total_buildings; i++) {
        /* Type = PRNG % 9 (0x1AA4) */
        uint16_t r = citygen_prng(&grid->prng_state);
        uint8_t raw = (uint8_t)(r & 0xFF);
        grid->buildings[i].type = raw % 9;

        /* Sequential ID */
        grid->buildings[i].id = (uint8_t)(seq_id & 0xFF);
        seq_id++;

        /* Name seed from PRNG */
        citygen_prng(&grid->prng_state);
        grid->buildings[i].name_seed = (uint8_t)(grid->prng_state & 0xFF);

    }
}

/*
 * City name generation from BuildingGen 0x2E1C-0x2E8E.
 *
 * Picks words from $-delimited newspaper/TV name tables,
 * combined with Greek letter suffixes.
 */
void citygen_generate_name(CityGrid *grid) {
    if (!grid) return;
    /* Newspaper suffixes for city naming */
    static const char *const greek[] = {
        "Alpha", "Beta", "Gamma", "Delta", "Epsilon",
        "Zeta", "Eta", "Theta", "Kappa",
    };

    /* City name = syllable pair from PRNG */
    uint16_t r1 = citygen_prng(&grid->prng_state);
    int s1 = (r1 >> 4) % LIBERATION_CITY_SYLLABLE_COUNT;
    uint16_t r2 = citygen_prng(&grid->prng_state);
    int s2 = r2 % LIBERATION_CITY_SYLLABLE_COUNT;

    /* Capitalize first syllable */
    char buf[64];
    snprintf(buf, sizeof(buf), "%s%s", liberation_city_syllables[s1],
             liberation_city_syllables[s2]);
    if (buf[0] >= 'a' && buf[0] <= 'z') buf[0] -= 32;

    /* Append Greek letter based on level */
    int greek_idx = grid->level % 9;
    snprintf(grid->city_name, sizeof(grid->city_name), "%s %s",
             buf, greek[greek_idx]);
}

/*
 * Generate building names.
 * Type determines the name table used:
 *   0: shop   → liberation_shop_types
 *   1: bar    → liberation_bar_types
 *   2: business → liberation_business_types
 *   3: industrial → liberation_industrial_types
 *   4: residence → "Private Residence"
 *   5: library  → "Library"
 *   6: police   → "Police Station"
 *   7: records  → "City Records Office"
 *   8: special  → first/last name combo
 */
static void citygen_name_building(CityGrid *grid, int idx) {
    if (!grid || idx < 0 || idx >= CITYGEN_MAX_BUILDINGS) return;
    CityBuilding *b = &grid->buildings[idx];
    uint16_t state = b->name_seed | ((uint16_t)b->id << 8);

    switch (b->type) {
    case BUILDING_SHOP: {
        int si = citygen_prng(&state) % LIBERATION_SHOP_TYPE_COUNT;
        int ni = citygen_prng(&state) % LIBERATION_LAST_NAME_COUNT;
        snprintf(b->name, sizeof(b->name), "%s's %s",
                 liberation_last_names[ni], liberation_shop_types[si]);
        break;
    }
    case BUILDING_BAR: {
        int bi = citygen_prng(&state) % LIBERATION_BAR_TYPE_COUNT;
        snprintf(b->name, sizeof(b->name), "%s", liberation_bar_types[bi]);
        break;
    }
    case BUILDING_BUSINESS: {
        int bi = citygen_prng(&state) % LIBERATION_BUSINESS_TYPE_COUNT;
        int ni = citygen_prng(&state) % LIBERATION_LAST_NAME_COUNT;
        snprintf(b->name, sizeof(b->name), "%s %s",
                 liberation_last_names[ni], liberation_business_types[bi]);
        break;
    }
    case BUILDING_INDUSTRIAL: {
        int ii = citygen_prng(&state) % LIBERATION_INDUSTRIAL_TYPE_COUNT;
        snprintf(b->name, sizeof(b->name), "%s", liberation_industrial_types[ii]);
        break;
    }
    case BUILDING_RESIDENCE:
        snprintf(b->name, sizeof(b->name), "%s", _("Private Residence"));
        break;
    case BUILDING_LIBRARY:
        snprintf(b->name, sizeof(b->name), "%s", _("Library"));
        break;
    case BUILDING_POLICE:
        snprintf(b->name, sizeof(b->name), "%s", _("Police Station"));
        break;
    case BUILDING_RECORDS:
        snprintf(b->name, sizeof(b->name), "%s", _("City Records Office"));
        break;
    case BUILDING_SPECIAL: {
        int fi = citygen_prng(&state) % LIBERATION_FIRST_NAME_COUNT;
        int li = citygen_prng(&state) % LIBERATION_LAST_NAME_COUNT;
        snprintf(b->name, sizeof(b->name), "%s %s",
                 liberation_first_names[fi], liberation_last_names[li]);
        break;
    }
    }
}

void citygen_generate(CityGrid *grid, uint16_t seed, uint16_t level) {
    if (!grid) return;
    citygen_init(grid, seed, level);
    citygen_compute_params(grid);
    citygen_place_buildings(grid);
    citygen_assign_types(grid);
    citygen_generate_name(grid);

    for (int i = 0; i < (int)grid->total_buildings; i++) {
        citygen_name_building(grid, i);
    }
}
