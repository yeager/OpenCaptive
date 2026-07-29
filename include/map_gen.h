#ifndef MAP_GEN_H
#define MAP_GEN_H

#include "game_state.h"

// Generate a dungeon level from a seed
// Algorithm: random walk carving, 30 steps, seed = ((mission-1)*11) + base
void map_generate(DungeonLevel *level, uint32_t seed, int level_num);

/* Generate one Captive base.  Architect stores all floors in one flattened
 * 64x32 map; this API presents its isolated floor regions through the
 * existing level array while keeping the original section model (16x8 cells).
 */
void map_generate_base(DungeonLevel levels[MAX_LEVELS], int *out_num_levels,
                       uint32_t seed);

#endif
