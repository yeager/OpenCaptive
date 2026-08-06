#include "captive_view_window.h"

#include <assert.h>
#include <string.h>

static void set_cell(DungeonLevel *level, int x, int y, CellType type) {
    level->cells[y][x].type = type;
}

static void fill_level(DungeonLevel *level, CellType type) {
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x)
            level->cells[y][x].type = type;
}

/* Coordinates in Captive's published renderer convention: forward is north,
 * lateral is east.  Keeping the test expressed in canonical cell positions
 * makes the visibility rules independent of host screen coordinates. */
static void set_visible_cell(DungeonLevel *level, int party_x, int party_y,
                             int cell, CellType type) {
    const CaptiveViewCellPosition p = captive_visible_cell_positions[cell - 1];
    set_cell(level, party_x + p.lateral, party_y - p.forward, type);
}

int main(void) {
    GameState state;
    memset(&state, 0, sizeof(state));
    state.num_levels = 1;
    state.party_x = 10;
    state.party_y = 10;
    fill_level(&state.levels[0], CELL_WALL);

    set_cell(&state.levels[0], 10, 8, CELL_GENERATOR);
    set_cell(&state.levels[0], 12, 10, CELL_SHOP);
    set_cell(&state.levels[0], 10, 12, CELL_TERMINAL);
    set_cell(&state.levels[0], 8, 10, CELL_DOOR);

    CaptiveViewWindow window;
    state.party_dir = DIR_NORTH;
    captive_view_window_build(&state, &window);
    assert(window.cells[4][2].type == CELL_GENERATOR);
    assert(window.cells[2][4].type == CELL_SHOP);
    assert(window.visible[11].type == CELL_GENERATOR); /* two forward */
    assert(window.visible[17].type == CELL_WALL);      /* current cell */

    state.party_dir = DIR_EAST;
    captive_view_window_build(&state, &window);
    assert(window.cells[4][2].type == CELL_SHOP);
    assert(window.cells[2][4].type == CELL_TERMINAL);
    assert(window.visible[11].type == CELL_SHOP);

    state.party_x = 0;
    state.party_y = 0;
    state.party_dir = DIR_NORTH;
    captive_view_window_build(&state, &window);
    assert(window.cells[0][0].type == CELL_WALL);

    /* A wall in canonical cell 9 clears cells 4, 5 and 10. */
    memset(&state, 0, sizeof(state));
    state.num_levels = 1;
    state.party_x = 10;
    state.party_y = 10;
    state.party_dir = DIR_NORTH;
    fill_level(&state.levels[0], CELL_FLOOR);
    set_cell(&state.levels[0], 11, 7, CELL_WALL); /* cell 9: three forward, right one */
    captive_view_window_build(&state, &window);
    assert(window.hidden[3] && window.hidden[4] && window.hidden[9]);
    assert(!window.hidden[2]);

    /* Cell 15 is the near right-hand wall.  Its cleanup is conditional on
     * nearer/farther walls and is evaluated after cells cleared by prior
     * rules have stopped being walls. */
    fill_level(&state.levels[0], CELL_FLOOR);
    set_visible_cell(&state.levels[0], 10, 10, 15, CELL_WALL);
    set_visible_cell(&state.levels[0], 10, 10, 17, CELL_WALL);
    captive_view_window_build(&state, &window);
    assert(window.hidden[13] && window.hidden[10] && window.hidden[6]);
    assert(window.hidden[5] && window.hidden[0] && window.hidden[1]);
    assert(window.hidden[11] && window.hidden[7] && window.hidden[2]);

    /* A wall that has already been hidden cannot trigger a later cleanup
     * branch.  Wall 9 hides cell 10, so the cell-15 right branch must not
     * treat that copied-and-cleared cell as an occluder. */
    fill_level(&state.levels[0], CELL_FLOOR);
    set_visible_cell(&state.levels[0], 10, 10, 9, CELL_WALL);
    set_visible_cell(&state.levels[0], 10, 10, 10, CELL_WALL);
    set_visible_cell(&state.levels[0], 10, 10, 15, CELL_WALL);
    captive_view_window_build(&state, &window);
    assert(window.hidden[9]);
    assert(!window.hidden[15]); /* cell 16 is not hidden by stale cell 10 */

    memset(&state, 0, sizeof(state));
    state.num_levels = MAX_LEVELS + 1;
    state.current_level = MAX_LEVELS;
    state.party_dir = DIR_NORTH;
    captive_view_window_build(&state, &window);
    assert(window.visible[0].type == CELL_WALL);

    memset(&state, 0, sizeof(state));
    state.num_levels = 1;
    state.current_level = 0;
    state.party_x = MAP_WIDTH;
    state.party_y = 0;
    state.party_dir = DIR_NORTH;
    captive_view_window_build(&state, &window);
    assert(window.visible[0].type == CELL_WALL);
    return 0;
}
