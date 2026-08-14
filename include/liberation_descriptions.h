#ifndef LIBERATION_DESCRIPTIONS_H
#define LIBERATION_DESCRIPTIONS_H

#include "liberation_citygen.h"
#include "liberation_text_table.h"
#include <stdbool.h>
#include <stddef.h>

/* Authentic Liberation building/location descriptions (the DTE table).  Set
 * once from the loaded game data; callers then ask for the real description of
 * a building category instead of showing invented prose. */
void liberation_descriptions_set(const LibTextTable *table);

/* Expand the authentic room description for a building category into out.
 * Returns false (leaving out untouched) when no table is set or the category
 * has no authentic DTE section, so callers keep their existing text rather than
 * showing the wrong building's description. */
bool liberation_description_for_building(BuildingType type, uint32_t seed,
                                         char *out, size_t out_size);

#endif
