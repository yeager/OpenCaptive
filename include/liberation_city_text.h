#ifndef LIBERATION_CITY_TEXT_H
#define LIBERATION_CITY_TEXT_H

#include "liberation_cte.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Authentic Liberation in-city interaction script (the CTE table).  Set once
 * from the loaded game data; callers then surface real dialogue instead of
 * invented text.  A NULL table (no game data, e.g. a unit test) makes the
 * queries below return false so callers keep their existing fallback. */
void liberation_city_text_set(const CteTable *table);

/* Expand one of the game's 16 authentic clue quotes — the literary clues the
 * city's informants deliver (CTE sections 300+n / 316+n, the two halves) —
 * chosen deterministically by seed, into out.  Returns false (out left
 * untouched) when no table is set or the sections are absent, so callers keep
 * their own text rather than showing nothing. */
bool liberation_city_text_clue(uint32_t seed, char *out, size_t out_size);

#endif
