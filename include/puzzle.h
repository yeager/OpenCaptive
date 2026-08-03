#ifndef PUZZLE_H
#define PUZZLE_H

#include "game_state.h"

// Puzzle types matching Captive's wall interactions
typedef enum {
    PUZZLE_NONE = 0,
    PUZZLE_BUTTON,        // Single wall button
    PUZZLE_LEVER,         // Binary lever (on/off)
    PUZZLE_TRIPLE_LEVER,  // Three levers
    PUZZLE_BARS,          // Number-matching bars puzzle (needs clipboard)
    PUZZLE_BUTTON_COMBO,  // 8 blue buttons (needs clipboard)
    PUZZLE_LASER_CODE,    // Laser door password
    PUZZLE_POWER_SOCKET,  // Recharges droids
    PUZZLE_HIDDEN_BUTTON, // Hidden in grate
    PUZZLE_FLOOR_TRAP,    // Damage trap tile
    PUZZLE_TELEPORTER_TRAP, // Random teleport trap
    PUZZLE_WALL_ELECTRIC,   // Wall-mounted electric trap (damages on interact)
} PuzzleType;

typedef struct {
    PuzzleType type;
    int        x, y;       // map position
    int        level;
    int        face;       // wall face direction (0-3)
    uint8_t    state;      // current state
    uint8_t    solution;   // correct state for combination puzzles
    int        target_x;   // linked door/wall position
    int        target_y;
    bool       solved;
} Puzzle;

#define MAX_PUZZLES 128

typedef struct {
    Puzzle puzzles[MAX_PUZZLES];
    int    num_puzzles;
} PuzzleList;

void puzzle_init(PuzzleList *pl);
void puzzle_generate(PuzzleList *pl, DungeonLevel *lvl, int level_num, uint32_t seed);
bool puzzle_interact(PuzzleList *pl, GameState *gs, int x, int y, int face);
void puzzle_check_step(PuzzleList *pl, GameState *gs, int x, int y);
bool puzzle_get_clipboard_hint(const PuzzleList *pl, const GameState *gs,
                               int x, int y, int face, char *buf, int buf_size);

#endif
