#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "opencaptive.h"

#define LIBERATION_MISSION_BITMAP_BYTES 32

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
    CELL_ELEVATOR,
    CELL_PRESSURE_PLATE,
    CELL_PIT,
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
    uint8_t  ca_segments;    // 5-bit CA wall segments from original 5-byte cell:
                             // bit 0: right wall (byte 0 & 0x10)
                             // bit 1: right-center (byte 1 & 0x10)
                             // bit 2: center/door (byte 2 & 0x08)
                             // bit 3: left-center/ornament (byte 3 & 0x80)
                             // bit 4: left wall (byte 4 & 0x10)
    uint8_t  ca_thickness;   // wall thickness code from CA rules (0x10/0x18/0x80/0xC0)
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
    uint8_t body_parts[6];   // head, torso, l_arm, r_arm, l_leg, r_leg (item ID)
    uint8_t body_part_hp[6]; // condition per part (0=destroyed, 255=perfect)
    uint8_t weapons[2];      // left/right hand weapon ID
    uint8_t shield;          // equipped shield item ID (0 = none)
    int16_t shield_hp;       // shield durability (absorbs damage first)
    uint8_t status;          // active status effects (StatusEffect bitmask)
    uint8_t poison_timer;    // ticks remaining for poison damage
    uint8_t stun_timer;      // ticks remaining for stun
    uint8_t items[10];       // inventory slots
    uint8_t skill_levels[10]; // per-skill level (0x00-0xFF)
    uint32_t xp;              // 32-bit XP accumulator (max 0xF8FFFFFF)
    uint16_t weapon_damage;   // lo*hi encoding from 0x9BF4
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
    STATE_HOLAMAP,
    STATE_PAUSE,
    STATE_HELP,
    STATE_DROID_CONFIG,
    STATE_CITY_MAP,
    STATE_SPACE_FLIGHT,
    STATE_ORBIT,
    STATE_LANDING,
    STATE_DEMO,
    STATE_STORY,
    STATE_LOADING,
    STATE_BAR,
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

    // Liberation inventory
    struct {
        char    name[24];
        uint16_t item_type;
    } lib_inventory[40];
    int         lib_inventory_count;

    // Reputation (Liberation)
    int         reputation; // -100 to +100, starts at 0
    uint8_t     lib_mission_complete[LIBERATION_MISSION_BITMAP_BYTES];

    // Difficulty (0=easy, 1=normal, 2=hard)
    uint8_t     difficulty;

    // Secondary objectives
    uint8_t     secondary_obj_type;  // 0=none, 1=collect, 2=no-damage, 3=speed
    uint8_t     secondary_obj_done;
    uint16_t    secondary_obj_param; // target count or tick limit

    // Score
    uint32_t    score;

    // Space navigation
    float       space_x, space_y;
    float       space_vx, space_vy;
    float       space_fuel;
    float       space_target_x, space_target_y;
    float       orbit_angle;
    int         landing_tick;

    // Weather (Liberation city: 0=clear, 1=rain, 2=fog)
    uint8_t     weather;

    // Movement
    uint32_t    move_cooldown;   // ticks until next movement allowed

    // Crime system (Liberation)
    uint8_t     crime_level;     // 0-5, increases with crimes
    uint8_t     wanted;          // 0=no, 1=wanted by police

    // Bar mini-game
    uint8_t     bar_number;      // target number 1-10
    uint8_t     bar_guesses;     // attempts remaining

    // UI state
    int         selected_droid;  // 0-3
    bool        map_overlay;
    bool        paused;
} GameState;

void game_state_init(GameState *gs, GameType type, int mission);
bool game_state_new_mission(GameState *gs, int mission);
bool game_state_new_mission_seeded(GameState *gs, int mission, uint32_t seed);
bool game_state_change_floor(GameState *gs, int direction);
bool game_state_complete_mission(GameState *gs);

#endif
