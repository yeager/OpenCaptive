#ifndef LIBERATION_CITYGEN_GRID_H
#define LIBERATION_CITYGEN_GRID_H

#include <stdint.h>
#include <stdbool.h>

#define CITYGRID_WIDTH  64
#define CITYGRID_HEIGHT 64
#define CITYGRID_CELLS  (CITYGRID_WIDTH * CITYGRID_HEIGHT)
#define CITYGRID_PLANES 3

#define CITYGRID_META_SIZE 8

#define CITYGRID_CELL_EMPTY  0x0D
#define CITYGRID_CELL_ROAD   0x0D
#define CITYGRID_CELL_WALL   0x00

#define CITYGRID_DIR_N 0
#define CITYGRID_DIR_E 1
#define CITYGRID_DIR_S 2
#define CITYGRID_DIR_W 3

#define CITYGRID_MAX_BUILDINGS 256
#define CITYGRID_CONN_TABLE_SIZE 51

typedef struct {
    int8_t  dx;
    int8_t  dy;
    int16_t offset;
} CityGridDirection;

typedef struct {
    uint16_t grid_offset;
    uint8_t  connection;
    uint8_t  direction;
} CityGridBuilding;

typedef struct {
    uint8_t plane0[CITYGRID_CELLS];
    uint8_t plane1[CITYGRID_CELLS];
    uint8_t plane2[CITYGRID_CELLS];

    uint8_t plane0_backup[CITYGRID_CELLS];

    uint8_t meta[CITYGRID_META_SIZE * CITYGRID_META_SIZE * 2];

    CityGridBuilding buildings[CITYGRID_MAX_BUILDINGS];
    uint8_t conn_table[CITYGRID_CONN_TABLE_SIZE];

    uint16_t prng_state;
    uint16_t seed_hi;
    uint16_t seed_lo;
    uint16_t seed_combined;

    uint16_t difficulty;
    uint16_t road_mask;
    uint16_t road_count;
    uint8_t  building_counter_a;
    uint8_t  building_counter_b;
    uint8_t  road_id;
    int16_t  retry_limit;
    int16_t  entry_point;

    uint8_t  feature_cell_type;
    uint8_t  feature_count;
    uint16_t feature_mode;
    uint8_t  feature_building_id;
} CityGridState;

uint16_t citygrid_prng(CityGridState *s);

void citygrid_init(CityGridState *s, uint16_t seed_hi, uint16_t seed_lo,
                   uint16_t difficulty);
void citygrid_generate(CityGridState *s);

#include "liberation_citygen.h"
void citygrid_map_buildings(CityGridState *s, const CityGrid *buildings);

#endif
