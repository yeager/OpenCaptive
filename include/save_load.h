#ifndef SAVE_LOAD_H
#define SAVE_LOAD_H

#include "game_state.h"

bool save_game(const GameState *gs, const char *path);
bool load_game(GameState *gs, const char *path);

#endif
