#ifndef LIBERATION_PLOTGEN_H
#define LIBERATION_PLOTGEN_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define PLOTGEN_MAX_BUILDINGS 128
#define PLOTGEN_MAX_ROOMS_PER_BUILDING 4
#define PLOTGEN_BUILDING_RECORD_SIZE 36
#define PLOTGEN_NAME_SIZE 64
#define PLOTGEN_TEXT_SIZE 8192

typedef enum {
    PLOTGEN_BTYPE_SHOP       = 0,
    PLOTGEN_BTYPE_BAR        = 1,
    PLOTGEN_BTYPE_BUSINESS   = 2,
    PLOTGEN_BTYPE_INDUSTRIAL = 3,
    PLOTGEN_BTYPE_RESIDENCE  = 4,
    PLOTGEN_BTYPE_LIBRARY    = 5,
    PLOTGEN_BTYPE_POLICE     = 6,
    PLOTGEN_BTYPE_RECORDS    = 7,
    PLOTGEN_BTYPE_SPECIAL    = 8,
    PLOTGEN_BTYPE_COUNT      = 9,
} PlotgenBuildingType;

typedef struct {
    uint16_t link_back;
    uint16_t marker;
    uint16_t link_fwd;
} PlotgenRoom;

typedef struct {
    uint8_t  type;
    uint8_t  building_id;
    uint8_t  flags;
    uint8_t  room_count;
    PlotgenRoom rooms[PLOTGEN_MAX_ROOMS_PER_BUILDING];
} PlotgenBuilding;

typedef struct {
    uint16_t seed;
    uint16_t prng_state;

    uint16_t grid_density;
    uint16_t num_columns;
    uint16_t num_roads;
    uint16_t num_cross_roads;
    uint16_t num_total;
    uint16_t side_buildings;
    uint16_t road_buildings;
    uint16_t building_columns;

    uint16_t num_buildings;
    PlotgenBuilding buildings[PLOTGEN_MAX_BUILDINGS];

    uint8_t  type_counts[PLOTGEN_BTYPE_COUNT];

    char city_name[PLOTGEN_NAME_SIZE];
    char victim_name[PLOTGEN_NAME_SIZE];
    char victim_title[PLOTGEN_NAME_SIZE];
    char news_source[PLOTGEN_NAME_SIZE];

    uint16_t victim_gender;
} PlotgenState;

void plotgen_init(PlotgenState *ps, uint16_t seed);
uint16_t plotgen_prng(PlotgenState *ps);
void plotgen_compute_params(PlotgenState *ps);
void plotgen_generate_buildings(PlotgenState *ps);
void plotgen_generate_names(PlotgenState *ps);

#endif
