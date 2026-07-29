#ifndef LIBERATION_H
#define LIBERATION_H

#include "game_state.h"
#include <stdbool.h>
#include <stdint.h>

// Liberation: Captive 2 city layout
// The city is a grid of buildings with streets between them

#define LIB_CITY_WIDTH  32
#define LIB_CITY_HEIGHT 32
#define LIB_BUILDING_FLOORS 4
#define LIB_FLOOR_WIDTH  16
#define LIB_FLOOR_HEIGHT 16

typedef enum {
    LIB_CELL_VOID = 0,
    LIB_CELL_STREET,
    LIB_CELL_SIDEWALK,
    LIB_CELL_WALL,
    LIB_CELL_FLOOR,
    LIB_CELL_DOOR,
    LIB_CELL_ELEVATOR,
    LIB_CELL_STAIRS,
    LIB_CELL_TERMINAL,
    LIB_CELL_NPC,
} LibCellType;

typedef enum {
    LIB_BUILDING_NONE = 0,
    LIB_BUILDING_RESIDENTIAL,
    LIB_BUILDING_COMMERCIAL,
    LIB_BUILDING_INDUSTRIAL,
    LIB_BUILDING_GOVERNMENT,
    LIB_BUILDING_PRISON,
    LIB_BUILDING_HOSPITAL,
    LIB_BUILDING_POLICE,
    LIB_BUILDING_SHOP,
} LibBuildingType;

typedef struct {
    LibCellType cells[LIB_FLOOR_HEIGHT][LIB_FLOOR_WIDTH];
} LibFloor;

typedef struct {
    LibBuildingType type;
    int city_x, city_y;
    int width, height;
    int num_floors;
    LibFloor floors[LIB_BUILDING_FLOORS];
    uint32_t seed;
} LibBuilding;

#define LIB_MAX_BUILDINGS 128

typedef struct {
    LibBuildingType grid[LIB_CITY_HEIGHT][LIB_CITY_WIDTH];
    LibBuilding buildings[LIB_MAX_BUILDINGS];
    int num_buildings;
    uint32_t city_seed;
} LibCity;

typedef enum {
    LIB_MODE_CITY,
    LIB_MODE_BUILDING,
} LibMode;

typedef struct {
    LibCity city;
    LibMode mode;
    int player_cx, player_cy;   // city grid position
    int player_bx, player_by;   // building interior position
    int player_floor;
    int current_building;       // index into buildings array, -1 = outside
    Direction player_dir;
    int clues_found;
    int target_building;        // mission objective
    bool mission_complete;
} LibState;

void lib_init(LibState *ls, uint32_t seed);
void lib_generate_city(LibState *ls);
void lib_generate_building(LibBuilding *b, LibBuildingType type, uint32_t seed);
bool lib_enter_current_building(LibState *ls);
bool lib_leave_current_building(LibState *ls);
bool lib_change_floor(LibState *ls, int direction);
bool lib_save_game(const LibState *ls, const char *path);
bool lib_load_game(LibState *ls, const char *path);
void lib_render_city(const LibState *ls, uint32_t *pixels, int width, int height);
void lib_render_building(const LibState *ls, uint32_t *pixels, int stride);

#endif
