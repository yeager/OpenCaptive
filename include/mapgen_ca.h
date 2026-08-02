#ifndef MAPGEN_CA_H
#define MAPGEN_CA_H

#include <stdint.h>
#include <stdbool.h>
#include "game_state.h"

#define CA_WIDTH  10
#define CA_HEIGHT 56
#define CA_CELL_SIZE 5

typedef enum {
    CA_TYPE_MAZE  = 0,
    CA_TYPE_ROOMS = 1,
    CA_TYPE_OPEN  = 2,
    CA_TYPE_MIXED = 3,
} CAMapType;

typedef struct {
    uint8_t cells[CA_HEIGHT][CA_WIDTH][CA_CELL_SIZE];
} CAMap;

void ca_init(CAMap *map);
void ca_generate_pattern(CAMap *map, uint16_t *prng_state);
void ca_apply_rules(CAMap *map, CAMapType type);
bool ca_cell_is_wall(const CAMap *map, int x, int y);
uint8_t ca_cell_wall_flags(const CAMap *map, int x, int y);
void ca_to_dungeon_level(const CAMap *map, DungeonLevel *level, int level_num);

#endif
