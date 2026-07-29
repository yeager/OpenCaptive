#ifndef SAVE_LOAD_H
#define SAVE_LOAD_H

#include "game_state.h"
#include "combat.h"
#include "puzzle.h"

bool save_game(const GameState *gs, const CreatureList *creatures,
               const PuzzleList *puzzles, const char *path);
bool load_game(GameState *gs, CreatureList *creatures, PuzzleList *puzzles,
               const char *path);

#endif
