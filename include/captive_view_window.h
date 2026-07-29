#ifndef CAPTIVE_VIEW_WINDOW_H
#define CAPTIVE_VIEW_WINDOW_H

#include "game_state.h"

/* Captive's renderer samples a 19-cell trapezoid, not a full 5x5 square:
 * five cells at range four, five at range three, three at range two, three at
 * range one, then the left/current/right cells at range zero.  The sample is
 * rotated into a fixed forward-facing order before visibility cleanup and
 * screen-space drawing. */
#define CAPTIVE_VIEW_WINDOW_SIZE 5
#define CAPTIVE_VIEW_WINDOW_ORIGIN 2
#define CAPTIVE_VISIBLE_CELL_COUNT 19

typedef struct {
    MapCell cells[CAPTIVE_VIEW_WINDOW_SIZE][CAPTIVE_VIEW_WINDOW_SIZE];
    MapCell visible[CAPTIVE_VISIBLE_CELL_COUNT];
    /* True when the original visibility pass clears the copied cell.  The
     * source map is never mutated; a renderer must skip hidden cells. */
    bool hidden[CAPTIVE_VISIBLE_CELL_COUNT];
} CaptiveViewWindow;

typedef struct {
    int8_t forward;
    int8_t lateral;
} CaptiveViewCellPosition;

/* Canonical renderer order: the farthest row first, left to right. */
extern const CaptiveViewCellPosition captive_visible_cell_positions[
    CAPTIVE_VISIBLE_CELL_COUNT];

void captive_view_window_build(const GameState *gs, CaptiveViewWindow *window);

#endif
