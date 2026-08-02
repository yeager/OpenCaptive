#ifndef LIBERATION_CITYGEN_H
#define LIBERATION_CITYGEN_H

#include <stdint.h>
#include <stdbool.h>

#define CITYGEN_MAX_BUILDINGS   100
#define CITYGEN_RECORD_SIZE     36
#define CITYGEN_MAX_CONNECTIONS 3
#define CITYGEN_BUILDING_TYPES  9

/* Building type categories (byte 0 of record, PRNG % 9) */
typedef enum {
    BUILDING_SHOP      = 0,
    BUILDING_BAR       = 1,
    BUILDING_BUSINESS  = 2,
    BUILDING_INDUSTRIAL = 3,
    BUILDING_RESIDENCE = 4,
    BUILDING_LIBRARY   = 5,
    BUILDING_POLICE    = 6,
    BUILDING_RECORDS   = 7,
    BUILDING_SPECIAL   = 8,
} BuildingType;

/* Connection direction markers from the original */
#define CONN_FORWARD  0xAAAA
#define CONN_BACKWARD 0xBBBB

typedef struct {
    uint16_t marker;
    uint16_t target_index;
} CityConnection;

typedef struct {
    uint8_t type;
    uint8_t id;
    uint8_t name_seed;
    uint8_t flags;
    uint16_t link;
    CityConnection connections[CITYGEN_MAX_CONNECTIONS];
    char name[32];
} CityBuilding;

typedef struct {
    uint16_t prng_state;
    uint16_t seed;
    uint16_t level;
    uint16_t density;
    uint16_t num_columns;
    uint16_t num_roads;
    uint16_t num_cross_roads;
    uint16_t road_buildings;
    uint16_t side_buildings;
    uint16_t buildings_per_segment;
    uint16_t total_buildings;
    CityBuilding buildings[CITYGEN_MAX_BUILDINGS];
    char city_name[64];
    uint16_t features;
} CityGrid;

/* PRNG: state = state * 0x5E5 + 0x29 (identical to Captive MapGen) */
uint16_t citygen_prng(uint16_t *state);

void citygen_init(CityGrid *grid, uint16_t seed, uint16_t level);
void citygen_compute_params(CityGrid *grid);
void citygen_place_buildings(CityGrid *grid);
void citygen_connect_roads(CityGrid *grid);
void citygen_assign_types(CityGrid *grid);
void citygen_generate_name(CityGrid *grid);
void citygen_generate(CityGrid *grid, uint16_t seed, uint16_t level);

#endif
