#include "captive_view_window.h"

#include <string.h>

static const int forward_x[] = {0, 1, 0, -1};
static const int forward_y[] = {-1, 0, 1, 0};

const CaptiveViewCellPosition captive_visible_cell_positions[
    CAPTIVE_VISIBLE_CELL_COUNT] = {
    {4, -2}, {4, -1}, {4, 0}, {4, 1}, {4, 2},
    {3, -2}, {3, -1}, {3, 0}, {3, 1}, {3, 2},
    {2, -1}, {2, 0}, {2, 1},
    {1, -1}, {1, 0}, {1, 1},
    { 0, -1}, { 0, 0}, { 0, 1},
};

static int visible_opaque(const CaptiveViewWindow *window, int cell) {
    int index = cell - 1; /* Original renderer/source documentation numbers 1..19. */
    return index >= 0 && index < CAPTIVE_VISIBLE_CELL_COUNT &&
        !window->hidden[index] &&
        (window->visible[index].type == CELL_WALL ||
         window->visible[index].type == CELL_DOOR ||
         window->visible[index].type == CELL_DOOR_LOCKED);
}

static void hide_cell(CaptiveViewWindow *window, int cell) {
    int index = cell - 1;
    if (index >= 0 && index < CAPTIVE_VISIBLE_CELL_COUNT)
        window->hidden[index] = true;
}

/* Exact visibility rule order recovered in the original renderer's published
 * analysis. A cleared cell ceases to participate in later wall tests, matching
 * the original copied-map state rather than line-of-sight approximation. */
static void captive_view_window_hide_occluded(CaptiveViewWindow *window) {
    if (visible_opaque(window, 9)) {
        hide_cell(window, 4); hide_cell(window, 5); hide_cell(window, 10);
    }
    if (visible_opaque(window, 7)) {
        hide_cell(window, 1); hide_cell(window, 2); hide_cell(window, 6);
    }
    if (visible_opaque(window, 11) || visible_opaque(window, 14) || visible_opaque(window, 17)) {
        hide_cell(window, 1); hide_cell(window, 6);
    }
    if (visible_opaque(window, 13) || visible_opaque(window, 16) || visible_opaque(window, 19)) {
        hide_cell(window, 5); hide_cell(window, 10);
    }
    if (visible_opaque(window, 15)) {
        if (visible_opaque(window, 17)) {
            hide_cell(window, 14); hide_cell(window, 11); hide_cell(window, 7);
            hide_cell(window, 6); hide_cell(window, 1); hide_cell(window, 2);
        } else if (visible_opaque(window, 14)) {
            hide_cell(window, 11); hide_cell(window, 7); hide_cell(window, 6);
            hide_cell(window, 1); hide_cell(window, 2);
        } else if (visible_opaque(window, 11)) {
            hide_cell(window, 7); hide_cell(window, 6); hide_cell(window, 1); hide_cell(window, 2);
        }
        if (visible_opaque(window, 19)) {
            hide_cell(window, 16); hide_cell(window, 13); hide_cell(window, 9);
            hide_cell(window, 10); hide_cell(window, 5); hide_cell(window, 4);
        } else if (visible_opaque(window, 16)) {
            hide_cell(window, 13); hide_cell(window, 9); hide_cell(window, 10);
            hide_cell(window, 5); hide_cell(window, 4);
        } else if (visible_opaque(window, 13)) {
            hide_cell(window, 9); hide_cell(window, 10); hide_cell(window, 5); hide_cell(window, 4);
        }
        hide_cell(window, 12); hide_cell(window, 8); hide_cell(window, 3);
    }
    if (visible_opaque(window, 12)) {
        hide_cell(window, 8);
        if (visible_opaque(window, 14)) {
            hide_cell(window, 11); hide_cell(window, 7); hide_cell(window, 6);
            hide_cell(window, 1); hide_cell(window, 2);
        } else if (visible_opaque(window, 11)) {
            hide_cell(window, 7); hide_cell(window, 6); hide_cell(window, 1); hide_cell(window, 2);
        }
        if (visible_opaque(window, 16)) {
            hide_cell(window, 13); hide_cell(window, 9); hide_cell(window, 10);
            hide_cell(window, 5); hide_cell(window, 4);
        } else if (visible_opaque(window, 13)) {
            hide_cell(window, 9); hide_cell(window, 10); hide_cell(window, 5); hide_cell(window, 4);
        }
        hide_cell(window, 3);
    }
    if (visible_opaque(window, 8)) {
        if (visible_opaque(window, 11)) { hide_cell(window, 2); hide_cell(window, 7); }
        if (visible_opaque(window, 13)) { hide_cell(window, 4); hide_cell(window, 9); }
        hide_cell(window, 3);
    }
}

void captive_view_window_build(const GameState *gs, CaptiveViewWindow *window) {
    if (!window) return;
    memset(window, 0, sizeof(*window));
    if (!gs || gs->current_level < 0 || gs->current_level >= gs->num_levels ||
        gs->current_level >= MAX_LEVELS ||
        gs->party_dir < DIR_NORTH || gs->party_dir > DIR_WEST ||
        gs->party_x < 0 || gs->party_x >= MAP_WIDTH ||
        gs->party_y < 0 || gs->party_y >= MAP_HEIGHT) {
        return;
    }

    const DungeonLevel *level = &gs->levels[gs->current_level];
    int direction = gs->party_dir;
    window->facing = (Direction)direction;
    int fx = forward_x[direction];
    int fy = forward_y[direction];
    int rx = forward_x[(direction + 1) & 3];
    int ry = forward_y[(direction + 1) & 3];

    for (int forward = -CAPTIVE_VIEW_WINDOW_ORIGIN;
         forward <= CAPTIVE_VIEW_WINDOW_ORIGIN; ++forward) {
        for (int lateral = -CAPTIVE_VIEW_WINDOW_ORIGIN;
             lateral <= CAPTIVE_VIEW_WINDOW_ORIGIN; ++lateral) {
            int x = gs->party_x + forward * fx + lateral * rx;
            int y = gs->party_y + forward * fy + lateral * ry;
            MapCell *out = &window->cells[forward + CAPTIVE_VIEW_WINDOW_ORIGIN]
                                           [lateral + CAPTIVE_VIEW_WINDOW_ORIGIN];
            if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
                *out = level->cells[y][x];
            } else {
                out->type = CELL_WALL;
            }
        }
    }

    for (int i = 0; i < CAPTIVE_VISIBLE_CELL_COUNT; ++i) {
        int forward = captive_visible_cell_positions[i].forward;
        int lateral = captive_visible_cell_positions[i].lateral;
        int x = gs->party_x + forward * fx + lateral * rx;
        int y = gs->party_y + forward * fy + lateral * ry;
        if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
            window->visible[i] = level->cells[y][x];
        } else {
            window->visible[i].type = CELL_WALL;
        }
    }
    captive_view_window_hide_occluded(window);
}
