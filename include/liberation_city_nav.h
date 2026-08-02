#ifndef LIBERATION_CITY_NAV_H
#define LIBERATION_CITY_NAV_H

#include <stdbool.h>
#include <stdint.h>
#include "liberation_citygen_grid.h"
#include "liberation_viewport_3d.h"
#include "liberation_x3g.h"

#define CITY_CELL_SIZE 256.0f
#define CITY_WALL_HEIGHT 128.0f
#define CITY_EYE_HEIGHT 64.0f

typedef enum {
    CITY_DIR_NORTH = 0,
    CITY_DIR_EAST  = 1,
    CITY_DIR_SOUTH = 2,
    CITY_DIR_WEST  = 3,
} CityDirection;

typedef struct {
    int cell_x, cell_y;
    CityDirection facing;
    float smooth_x, smooth_y;
    float smooth_yaw;
    bool moving;
    float move_progress;
    int move_dx, move_dy;
} CityNavState;

void city_nav_init(CityNavState *nav, int start_x, int start_y, CityDirection dir);

bool city_nav_can_move_forward(const CityNavState *nav, const CityGridState *grid);
bool city_nav_can_move_backward(const CityNavState *nav, const CityGridState *grid);

void city_nav_move_forward(CityNavState *nav, const CityGridState *grid);
void city_nav_move_backward(CityNavState *nav, const CityGridState *grid);
void city_nav_turn_left(CityNavState *nav);
void city_nav_turn_right(CityNavState *nav);
void city_nav_turn_around(CityNavState *nav);

void city_nav_update(CityNavState *nav, float dt);

void city_nav_render(CityNavState *nav, const CityGridState *grid,
                     Lib3dState *render, const X3gFile *city_vectors,
                     const uint32_t *palette, unsigned pal_size);

uint8_t city_nav_get_cell(const CityGridState *grid, int x, int y);
bool city_nav_is_road(const CityGridState *grid, int x, int y);
bool city_nav_is_wall(const CityGridState *grid, int x, int y);
bool city_nav_is_building_entrance(const CityGridState *grid, int x, int y);

#endif
