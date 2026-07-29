#ifndef MAP_GEN_H
#define MAP_GEN_H

#include "game_state.h"

// Generate a dungeon level from a seed
// Algorithm: random walk carving, 30 steps, seed = ((mission-1)*11) + base
void map_generate(DungeonLevel *level, uint32_t seed, int level_num);

#endif
