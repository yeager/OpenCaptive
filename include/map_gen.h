#ifndef MAP_GEN_H
#define MAP_GEN_H

#include "game_state.h"
#include <stddef.h>

// Generate a dungeon level from a seed
// Algorithm: random walk carving, 30 steps, seed = ((mission-1)*11) + base
void map_generate(DungeonLevel *level, uint32_t seed, int level_num);

/* Generate one Captive base.  Architect stores all floors in one flattened
 * 64x32 map; this API presents its isolated floor regions through the
 * existing level array while keeping the original section model (16x8 cells).
 */
void map_generate_base(DungeonLevel levels[MAX_LEVELS], int *out_num_levels,
                       uint32_t seed);

/* Exact 16-bit Architect PRNG recovered from the verified Amiga MapGen HUNK.
 * Exposed as a sequence helper so ports can validate phase work before map
 * output fixtures are available. */
void mapgen_original_prng_sequence(uint16_t seed, uint16_t *out, size_t count);

/* Generate the exterior landing zone level prepended to a base.
 * The exterior is a flat open area with the base entrance at one edge.
 * entrance_x is the x coordinate of the base entrance on floor 0 row 0. */
void map_generate_exterior(DungeonLevel *exterior, int entrance_x, uint32_t seed);

#endif
