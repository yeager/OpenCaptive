#include "liberation_city_nav.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static CityGridState make_test_grid(void) {
    CityGridState grid;
    memset(&grid, 0, sizeof(grid));
    for (int i = 0; i < CITYGRID_CELLS; i++)
        grid.plane0[i] = CITYGRID_CELL_WALL;

    /* Create a small road network */
    for (int x = 5; x <= 10; x++)
        grid.plane0[5 * CITYGRID_WIDTH + x] = CITYGRID_CELL_ROAD;
    for (int y = 3; y <= 8; y++)
        grid.plane0[y * CITYGRID_WIDTH + 7] = CITYGRID_CELL_ROAD;

    grid.plane0[5 * CITYGRID_WIDTH + 8] = 0x0A;

    return grid;
}

static void test_init(void) {
    CityNavState nav;
    city_nav_init(&nav, 5, 5, CITY_DIR_EAST);
    assert(nav.cell_x == 5);
    assert(nav.cell_y == 5);
    assert(nav.facing == CITY_DIR_EAST);
    assert(!nav.moving);
}

static void test_teleport_updates_interpolated_position(void) {
    CityNavState nav;
    city_nav_init(&nav, 5, 5, CITY_DIR_WEST);
    nav.moving = true;
    nav.move_progress = 0.5f;
    assert(city_nav_teleport(&nav, 20, 21));
    assert(nav.cell_x == 20 && nav.cell_y == 21);
    assert(nav.smooth_x == 20 * CITY_CELL_SIZE + CITY_CELL_SIZE * 0.5f);
    assert(nav.smooth_y == 21 * CITY_CELL_SIZE + CITY_CELL_SIZE * 0.5f);
    assert(nav.facing == CITY_DIR_WEST);
    assert(!nav.moving && nav.move_progress == 0.0f);
    assert(!city_nav_teleport(&nav, -1, 0));
}

static void test_cell_queries(void) {
    CityGridState grid = make_test_grid();
    assert(city_nav_is_road(&grid, 5, 5));
    assert(city_nav_is_road(&grid, 10, 5));
    assert(city_nav_is_wall(&grid, 0, 0));
    assert(city_nav_is_wall(&grid, 4, 5));
    assert(city_nav_is_building_entrance(&grid, 8, 5));
    assert(!city_nav_is_building_entrance(&grid, 6, 5));
    assert(city_nav_is_wall(&grid, -1, 0));
    assert(city_nav_is_wall(&grid, 64, 0));
}

static void test_move_forward(void) {
    CityGridState grid = make_test_grid();
    CityNavState nav;
    city_nav_init(&nav, 5, 5, CITY_DIR_EAST);

    assert(city_nav_can_move_forward(&nav, &grid));
    city_nav_move_forward(&nav, &grid);
    assert(nav.moving);

    city_nav_update(&nav, 1.0f);
    assert(!nav.moving);
    assert(nav.cell_x == 6);
    assert(nav.cell_y == 5);
}

static void test_blocked_move(void) {
    CityGridState grid = make_test_grid();
    CityNavState nav;
    city_nav_init(&nav, 5, 5, CITY_DIR_WEST);

    assert(!city_nav_can_move_forward(&nav, &grid));
    city_nav_move_forward(&nav, &grid);
    assert(!nav.moving);
    assert(nav.cell_x == 5);
}

static void test_nonroad_building_cell_is_blocked(void) {
    CityGridState grid = make_test_grid();
    CityNavState nav;
    city_nav_init(&nav, 6, 5, CITY_DIR_EAST);
    /* Building plane cells carry type/ID bits and are nonzero, but are not
     * traversable city roads. */
    grid.plane0[5 * CITYGRID_WIDTH + 7] = 0x41;
    assert(city_nav_is_wall(&grid, 7, 5));
    assert(!city_nav_can_move_forward(&nav, &grid));
    city_nav_move_forward(&nav, &grid);
    assert(!nav.moving);
    assert(nav.cell_x == 6 && nav.cell_y == 5);
}

static void test_road_feature_is_not_rendered_as_wall(void) {
    CityGridState grid = make_test_grid();
    grid.plane0[5 * CITYGRID_WIDTH + 7] = 0x0E; /* finalized post box */
    assert(!city_nav_is_road(&grid, 7, 5));
    assert(city_nav_is_wall(&grid, 7, 5) == false);
}

static void test_turn(void) {
    CityNavState nav;
    city_nav_init(&nav, 5, 5, CITY_DIR_NORTH);

    city_nav_turn_right(&nav);
    assert(nav.facing == CITY_DIR_EAST);

    city_nav_turn_right(&nav);
    assert(nav.facing == CITY_DIR_SOUTH);

    city_nav_turn_left(&nav);
    assert(nav.facing == CITY_DIR_EAST);

    city_nav_turn_around(&nav);
    assert(nav.facing == CITY_DIR_WEST);
}

static void test_move_backward(void) {
    CityGridState grid = make_test_grid();
    CityNavState nav;
    city_nav_init(&nav, 6, 5, CITY_DIR_EAST);

    assert(city_nav_can_move_backward(&nav, &grid));
    city_nav_move_backward(&nav, &grid);
    assert(nav.moving);

    city_nav_update(&nav, 1.0f);
    assert(nav.cell_x == 5);
}

static void test_smooth_movement(void) {
    CityGridState grid = make_test_grid();
    CityNavState nav;
    city_nav_init(&nav, 5, 5, CITY_DIR_EAST);

    float start_x = nav.smooth_x;
    city_nav_move_forward(&nav, &grid);

    city_nav_update(&nav, 0.1f);
    assert(nav.moving);
    assert(nav.smooth_x > start_x);
    assert(nav.smooth_x < start_x + CITY_CELL_SIZE);
}

static void test_update_rejects_invalid_state(void) {
    CityNavState nav;
    city_nav_init(&nav, 0, 0, CITY_DIR_NORTH);
    nav.moving = true;
    nav.move_dx = -1;
    nav.move_dy = 0;
    city_nav_update(&nav, 1.0f);
    assert(!nav.moving);
    assert(nav.cell_x == 0 && nav.cell_y == 0);

    nav.cell_x = CITYGRID_WIDTH;
    nav.moving = true;
    city_nav_update(&nav, 1.0f);
    assert(!nav.moving);

    city_nav_init(&nav, 5, 5, CITY_DIR_EAST);
    nav.moving = true;
    nav.move_dx = 1;
    nav.move_dy = 1;
    city_nav_update(&nav, 1.0f);
    assert(!nav.moving);
    assert(nav.cell_x == 5 && nav.cell_y == 5);

    city_nav_init(&nav, 5, 5, CITY_DIR_EAST);
    city_nav_move_forward(&nav, &(CityGridState){0});
    /* A NaN time step must not poison smooth coordinates. */
    nav.moving = true;
    float smooth_x = nav.smooth_x;
    city_nav_update(&nav, NAN);
    assert(nav.smooth_x == smooth_x);
}

static void test_render(void) {
    CityGridState grid = make_test_grid();
    CityNavState nav;
    city_nav_init(&nav, 7, 5, CITY_DIR_NORTH);

    Lib3dState render;
    lib3d_init(&render);

    city_nav_render(&nav, &grid, &render, NULL, NULL, 0);

    assert(render.vis_count > 0);

    uint32_t dest[320 * 200];
    memset(dest, 0, sizeof(dest));
    lib3d_present(&render, dest, 320, 200, 32, 20);

    bool has_wall = false;
    for (int i = 0; i < 320 * 200; i++)
        if (dest[i] != 0xFF4466AA && dest[i] != 0xFF446644 && dest[i] != 0)
            has_wall = true;
    assert(has_wall);
}

static void test_render_rejects_corrupt_coordinates(void) {
    CityGridState grid = make_test_grid();
    CityNavState nav;
    Lib3dState render;
    lib3d_init(&render);
    city_nav_init(&nav, 5, 5, CITY_DIR_NORTH);
    nav.cell_x = INT_MAX;
    city_nav_render(&nav, &grid, &render, NULL, NULL, 0);
    city_nav_render_textured(&nav, &grid, &render, NULL, NULL, NULL, 0);
    assert(render.vis_count == 0);
}

int main(void) {
    test_init();
    test_teleport_updates_interpolated_position();
    test_cell_queries();
    test_move_forward();
    test_blocked_move();
    test_nonroad_building_cell_is_blocked();
    test_road_feature_is_not_rendered_as_wall();
    test_turn();
    test_move_backward();
    test_smooth_movement();
    test_update_rejects_invalid_state();
    test_render();
    test_render_rejects_corrupt_coordinates();
    printf("All liberation_city_nav tests passed\n");
    return 0;
}
