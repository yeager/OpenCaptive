#ifndef TERMINAL_H
#define TERMINAL_H

#include "game_state.h"
#include <stdbool.h>

typedef enum {
    TERM_MAIN,
    TERM_MAP,
    TERM_STATUS,
    TERM_MISSION,
} TerminalPage;

typedef struct {
    TerminalPage page;
    int cursor;
    bool active;
    int level;
} TerminalState;

void terminal_init(TerminalState *ts, int level);
void terminal_render(const TerminalState *ts, const GameState *gs,
                     uint32_t *pixels, int width, int height);
bool terminal_handle_key(TerminalState *ts, int key);

#endif
