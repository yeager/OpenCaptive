#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "opencaptive.h"

// Map dimensions
#define MAP_WIDTH   64
#define MAP_HEIGHT  32
#define MAX_LEVELS  15

// Cell types
typedef enum {
    CELL_WALL = 0,
    CELL_FLOOR,
    CELL_DOOR,
    CELL_DOOR_LOCKED,
    CELL_STAIRS_UP,
    CELL_STAIRS_DOWN,
    CELL_SHOP,
    CELL_TELEPORTER,
    CELL_GENERATOR,
    CELL_TERMINAL,
} CellType;

// Facing directions
typedef enum {
    DIR_NORTH = 0,
    DIR_EAST  = 1,
    DIR_SOUTH = 2,
    DIR_WEST  = 3,
} Direction;

// Wall ornament types
typedef enum {
    ORNAMENT_NONE = 0,
    ORNAMENT_BUTTON,
    ORNAMENT_SCREEN,
    ORNAMENT_VENT,
    ORNAMENT_PIPE,
    ORNAMENT_SIGN,
    ORNAMENT_POSTER,
    ORNAMENT_PANEL,
} OrnamentType;

typedef struct {
    CellType type;
    uint8_t  wall_tex[4];    // wall texture per direction (N/E/S/W)
    uint8_t  floor_tex;
    uint8_t  ceil_tex;
    OrnamentType ornament[4]; // ornament per wall face
    uint8_t  item_id;        // item on floor (0 = none)
    uint8_t  creature_id;    // creature in cell (0 = none)
    uint8_t  flags;          // misc flags
} MapCell;

typedef struct {
    MapCell cells[MAP_HEIGHT][MAP_WIDTH];
    int     level;
    uint32_t seed;
} DungeonLevel;

// Droid stats
typedef struct {
    char    name[16];
    int16_t hp;
    int16_t hp_max;
    int16_t energy;
    int16_t energy_max;
    uint8_t body_parts[6];   // head, torso, l_arm, r_arm, l_leg, r_leg
    uint8_t weapons[2];      // left/right hand weapon ID
    uint8_t items[10];       // inventory slots
    uint8_t level;
    uint16_t xp;
} Droid;

typedef enum {
    GAME_CAPTIVE,
    GAME_LIBERATION,
} GameType;

typedef enum {
    STATE_MENU,
    STATE_INTRO,
    STATE_GAME,
    STATE_SHOP,
    STATE_TERMINAL,
    STATE_GAMEOVER,
    STATE_VICTORY,
    STATE_INVENTORY,
} GameStateMode;

typedef struct {
    GameType     game_type;
    GameStateMode mode;
    OpenCaptiveConfig config;

    // Party
    Droid       droids[4];
    int         party_x;
    int         party_y;
    Direction   party_dir;
    int         current_level;
    int         mission;

    // Dungeon
    DungeonLevel levels[MAX_LEVELS];
    int          num_levels;

    // Mission
    int         base_id;
    uint32_t    mission_seed;
    int         generators_total;
    int         generators_destroyed;
    int         gold;

    // Timing
    uint32_t    tick;
    uint32_t    last_frame_ms;

    // UI state
    int         selected_droid;  // 0-3
    bool        map_overlay;
    bool        paused;
} GameState;

void game_state_init(GameState *gs, GameType type, int mission);
void game_state_new_mission(GameState *gs, int mission);
bool game_state_change_floor(GameState *gs, int direction);

#endif
