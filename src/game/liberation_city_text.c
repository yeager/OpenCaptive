#include "liberation_city_text.h"
#include <stdio.h>

static const CteTable *g_city_text;

void liberation_city_text_set(const CteTable *table) {
    g_city_text = table;
}

/* The 16 authentic clue quotes are each stored as two halves: the opening line
 * in section 300+n and its completion in section 316+n (verified against the
 * real CITY_TEXT).  Each half already carries its own "..." join, so the two
 * expansions are simply concatenated. */
bool liberation_city_text_clue(uint32_t seed, char *out, size_t out_size) {
    if (!g_city_text || !out || out_size < 4) return false;
    unsigned n = seed % 16u;
    const CteSection *a = cte_section_find(g_city_text, (uint16_t)(300u + n));
    const CteSection *b = cte_section_find(g_city_text, (uint16_t)(316u + n));
    if (!a || !b) return false;

    char first[256] = "", second[256] = "";
    CteState state;
    cte_state_init(&state, 1);
    if (!cte_expand(g_city_text, a, &state, first, sizeof first)) return false;
    cte_state_init(&state, 1);
    if (!cte_expand(g_city_text, b, &state, second, sizeof second)) return false;

    snprintf(out, out_size, "%s%s", first, second);
    return out[0] != '\0';
}
