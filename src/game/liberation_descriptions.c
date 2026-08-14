#include "liberation_descriptions.h"

static const LibTextTable *g_descriptions;

void liberation_descriptions_set(const LibTextTable *table) {
    g_descriptions = table;
}

/* Map the game's simplified building category to the original Liberation DTE
 * section id.  The game collapses the 20 authentic categories into 9 types, so
 * only the unambiguous ones map to a real "location description" section; the
 * rest (residence, library, police, special) are game concepts with no DTE
 * section and return -1 so the caller keeps its own text.  Section ids come
 * from the DTE table headers: 1=General Stores, 8=Bars, 11=Merchants,
 * 13=Metals/Engineering, 0=City Records Office. */
static int dte_section_for_building(BuildingType type) {
    switch (type) {
        case BUILDING_SHOP:       return 1;
        case BUILDING_BAR:        return 8;
        case BUILDING_BUSINESS:   return 11;
        case BUILDING_INDUSTRIAL: return 13;
        case BUILDING_RECORDS:    return 0;
        default:                  return -1;
    }
}

bool liberation_description_for_building(BuildingType type, uint32_t seed,
                                         char *out, size_t out_size) {
    if (!g_descriptions || !out || out_size == 0) return false;
    int section_id = dte_section_for_building(type);
    if (section_id < 0) return false;
    const LibTextSection *sec =
        lib_text_table_find(g_descriptions, (unsigned)section_id);
    if (!sec) return false;
    LibTextState state;
    lib_text_state_init(&state, seed);
    /* case 0 = the normal, occupant-present description. */
    return lib_text_expand(g_descriptions, sec, &state, 0, out, out_size);
}
