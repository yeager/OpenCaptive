#include "liberation_plotgen.h"
#include "liberation_data.h"
#include <string.h>
#include <stdio.h>

uint16_t plotgen_prng(PlotgenState *ps) {
    if (!ps) return 0;
    ps->prng_state = (uint16_t)(ps->prng_state * 0x5E5 + 0x29);
    return ps->prng_state;
}

static uint16_t prng_range(PlotgenState *ps, uint16_t n) {
    if (n == 0) return 0;
    uint16_t r = plotgen_prng(ps);
    return (uint16_t)((r >> 8) % n);
}

static uint16_t ror16(uint16_t value, unsigned count) {
    count &= 15U;
    if (count == 0U) return value;
    return (uint16_t)((value >> count) | (value << (16U - count)));
}

void plotgen_init(PlotgenState *ps, uint16_t seed) {
    if (!ps) return;
    memset(ps, 0, sizeof(*ps));
    ps->seed = seed;
    ps->prng_state = seed;
}

/*
 * PlotGen $0548-$068A: compute city parameters from seed.
 * Transcribed from 68000 disassembly of the original Amiga binary.
 */
void plotgen_compute_params(PlotgenState *ps) {
    if (!ps) return;
    uint16_t seed_lo = ps->seed & 0x0F;
    plotgen_prng(ps);
    uint16_t d2 = ps->prng_state;

    /* grid_density = seed*5 + 15 + rotation_bits, clamped to 100 */
    uint16_t rot = d2;
    /* ROL.W seed_lo, d2 */
    for (int i = 0; i < (seed_lo & 0x1F); i++)
        rot = (uint16_t)((rot << 1) | (rot >> 15));
    rot &= 0x07;

    uint32_t density = (uint32_t)ps->seed * 5U + 15U + rot;
    if (density > 100U) density = 100U;
    ps->grid_density = (uint16_t)density;

    /* num_columns = seed>>4 + 6 + rotation_bits, halved while >15 */
    uint16_t d0 = ps->prng_state;
    uint16_t shift_amt = (uint16_t)(seed_lo + 8);
    for (int i = 0; i < (shift_amt & 0x1F); i++)
        d0 = (uint16_t)((d0 << 1) | (d0 >> 15));
    d0 &= 0x03;
    /* Keep the -2..+1 adjustment signed.  Subtracting from uint16_t and
     * converting the wrapped value back to int16_t is implementation-defined
     * on non-two's-complement targets. */
    int16_t column_adjust = (int16_t)d0 - 2;
    d2 = (uint16_t)((int)(ps->seed >> 4) + 6 + column_adjust);
    while (d2 > 15) d2 >>= 1;
    ps->num_columns = d2;

    /* num_roads: seed-derived + rotation, min 1 */
    d2 = ps->prng_state & 0x07;
    d0 = ps->prng_state;
    d0 = ror16(d0, d2);
    d0 &= 0x03;
    d0 += 2;
    uint16_t d1 = ps->seed >> 3;
    int roads = (int)d0 - (int)d1;
    if (roads <= 0) roads = 1;
    if (ps->seed == 0) roads = 5;
    ps->num_roads = (uint16_t)roads;

    /* num_cross_roads: seed-derived, 1..5 */
    d2 = ps->prng_state & 0x03;
    d0 = ps->prng_state;
    d0 = (uint16_t)((d0 >> 8) | (d0 << 8));
    d0 = ror16(d0, d2);
    d0 &= 0x01;
    d1 = ps->seed >> 3;
    d0 += d1;
    d0++;
    if (d0 > 5) d0 = 5;
    ps->num_cross_roads = d0;

    /* road_buildings = cross_roads/2 (min=cross_roads if /2==0) */
    d0 = ps->num_cross_roads >> 1;
    if (d0 == 0) d0 = ps->num_cross_roads;
    ps->road_buildings = d0;

    /* side_buildings = min(roads*2, 5) */
    d0 = ps->num_roads * 2;
    if (d0 > 5) d0 = 5;
    ps->side_buildings = d0;

    /* num_total: ensure room for all categories */
    d0 = ps->road_buildings;
    d1 = ps->grid_density >> 1;
    if (d1 <= d0) d1 = d0 + 1;
    ps->num_total = d1;

    /* building_columns */
    d0 = ps->num_columns >> 1;
    if (d0 == 0) d0 = ps->num_columns;
    ps->building_columns = d0;
}

/*
 * PlotGen $068C-$086E: generate building records.
 * Each building has type, id, flags, and up to 4 room connections.
 */
void plotgen_generate_buildings(PlotgenState *ps) {
    if (!ps) return;
    plotgen_compute_params(ps);

    uint16_t total = ps->num_total;
    if (total > PLOTGEN_MAX_BUILDINGS) total = PLOTGEN_MAX_BUILDINGS;
    ps->num_buildings = total;

    memset(ps->buildings, 0, sizeof(ps->buildings));
    memset(ps->type_counts, 0, sizeof(ps->type_counts));

    /* Assign building types from seed */
    for (uint16_t i = 0; i < total; i++) {
        uint16_t type_idx = (uint16_t)((ps->prng_state & 0xFF) % PLOTGEN_BTYPE_COUNT);
        plotgen_prng(ps);
        ps->buildings[i].type = (uint8_t)type_idx;
        ps->buildings[i].building_id = (uint8_t)i;
        ps->type_counts[type_idx]++;

        /* Room count: 1-4 based on PRNG */
        uint16_t rc = (uint16_t)(1 + prng_range(ps, 4));
        if (rc > PLOTGEN_MAX_ROOMS_PER_BUILDING)
            rc = PLOTGEN_MAX_ROOMS_PER_BUILDING;
        ps->buildings[i].room_count = (uint8_t)rc;

        for (uint8_t r = 0; r < rc; r++) {
            ps->buildings[i].rooms[r].marker = (r == 0) ? 0xAAAA : 0xBBBB;
            ps->buildings[i].rooms[r].link_back = (r > 0) ? (uint16_t)(r - 1) : 0xFFFF;
            ps->buildings[i].rooms[r].link_fwd = (r + 1 < rc) ? (uint16_t)(r + 1) : 0xFFFF;
        }
    }
}

static void pick_from_table(PlotgenState *ps, const char *const *table,
                            int count, char *out, size_t out_size) {
    int idx = prng_range(ps, (uint16_t)count);
    snprintf(out, out_size, "%s", table[idx]);
}

/*
 * PlotGen $0C5C-$0CE6: generate city name, victim name, victim title,
 * and news source name.
 */
void plotgen_generate_names(PlotgenState *ps) {
    if (!ps) return;
    /* City name: two syllables + optional Greek letter suffix */
    pick_from_table(ps, liberation_city_syllables,
                    LIBERATION_CITY_SYLLABLE_COUNT, ps->city_name, PLOTGEN_NAME_SIZE);

    char syl2[32];
    pick_from_table(ps, liberation_city_syllables,
                    LIBERATION_CITY_SYLLABLE_COUNT, syl2, sizeof(syl2));

    size_t len = strlen(ps->city_name);
    snprintf(ps->city_name + len, PLOTGEN_NAME_SIZE - len, "%s", syl2);

    /* Victim name: first + last */
    pick_from_table(ps, liberation_first_names,
                    LIBERATION_FIRST_NAME_COUNT, ps->victim_name, PLOTGEN_NAME_SIZE);

    char last[32];
    pick_from_table(ps, liberation_last_names,
                    LIBERATION_LAST_NAME_COUNT, last, sizeof(last));

    len = strlen(ps->victim_name);
    snprintf(ps->victim_name + len, PLOTGEN_NAME_SIZE - len, " %s", last);

    /* Gender: bit from PRNG (0=male, 1=female) */
    plotgen_prng(ps);
    ps->victim_gender = (ps->prng_state >> 4) & 1;

    /* Victim title */
    pick_from_table(ps, liberation_npc_titles,
                    LIBERATION_NPC_TITLE_COUNT, ps->victim_title, PLOTGEN_NAME_SIZE);

    /* News source: newspaper or TV channel name */
    static const char *const newspapers[] = {
        "Gazette", "Advertiser", "News", "Guardian", "Times",
        "Herald", "Mercury", "Mirror", "Express", "Messenger",
        "Beobachter", "Shimbun", "Post", "Daily News",
        "Communique", "Star",
    };
    static const char *const tv_channels[] = {
        "Channel 2", "KVLM", "VidNews", "NewsNet", "Cable News",
        "Terebi no Shimbun", "NewsDesk", "Fernsehnachrichten",
        "actualites",
    };

    plotgen_prng(ps);
    if (ps->prng_state & 0x100) {
        pick_from_table(ps, tv_channels, 9, ps->news_source, PLOTGEN_NAME_SIZE);
    } else {
        char city_part[32];
        snprintf(city_part, sizeof(city_part), "%s", ps->city_name);
        char paper[32];
        pick_from_table(ps, newspapers, 16, paper, sizeof(paper));
        snprintf(ps->news_source, PLOTGEN_NAME_SIZE, "%s %s", city_part, paper);
    }
}
