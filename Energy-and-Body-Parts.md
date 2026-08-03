# Energy and Body Parts

Each droid in OpenCaptive has an energy system and six body parts that affect combat survivability and equipment.

## Droid Struct

The `Droid` struct is 68 bytes:

```c
typedef struct {
    char    name[16];          // null-terminated name
    int16_t hp;                // current hit points
    int16_t hp_max;            // maximum hit points
    int16_t energy;            // current energy
    int16_t energy_max;        // maximum energy
    uint8_t body_parts[6];     // equipped item ID per slot (0 = none)
    uint8_t body_part_hp[6];   // condition per slot (0 = destroyed, 255 = perfect)
    uint8_t weapons[2];        // left/right hand weapon ID
    uint8_t items[10];         // inventory slots
    uint8_t skill_levels[10];  // per-skill level (0x00 - 0xFF)
    uint32_t xp;               // experience (max 0xF8FFFFFF)
    uint16_t weapon_damage;    // lo*hi encoding from original address 0x9BF4
} Droid;
```

### Alive Check

There is no `active` field. A droid is alive when `hp > 0`. Dead droids (`hp <= 0`) are skipped for movement energy costs but remain in the party.

## Energy System

### Regeneration

Energy regenerates at **1 point per approximately 5 seconds** (every 300 game ticks) for each alive droid. HP also regenerates alongside energy at the same rate (+1 HP per tick if below `hp_max`).

### Consumption

| Action | Energy Cost |
|---|---|
| Weapon shot | 3 per shot |
| Movement step | 1 per alive droid |
| Battery use | Restores 50 energy (ENTER key) |

Dead droids do not consume movement energy.

### Initial Values

New droids start with:

- HP: 100 / HP max: 100
- Energy: 100 / Energy max: 100
- XP: 1024
- Weapon damage: 0x050A

## Body Parts

Each droid has 6 body part slots:

| Index | Slot |
|---|---|
| 0 | Head |
| 1 | Torso |
| 2 | Left Arm |
| 3 | Right Arm |
| 4 | Left Leg |
| 5 | Right Leg |

### Body Part Condition

Each slot has a condition value in `body_part_hp[6]`:

- **255**: perfect condition
- **0**: destroyed
- Displayed in the droid UI as a color-coded percentage

### Combat Damage

When a creature attacks, it damages a random body part. The damage to the body part is **1/4 of the attack damage** dealt to the droid.

### Armor Defense Values

Body armor provides damage reduction based on slot:

| Slot | Defense Value |
|---|---|
| Head | 10 |
| Chest | 15 |
| Arm | 8 |
| Leg | 8 |
| Foot | 5 |
| Hand | 5 |

Actual damage reduction scales with the body part's current condition -- a damaged armor piece provides less protection.

### Repair and Installation

- **Shop repair**: restores all body part conditions to 255.
- **Inventory installation**: pressing ENTER on an armor item in inventory installs it to the appropriate body part slot, resetting that part's condition to 255.

### Equipment

- `body_parts[6]`: stores the equipped item ID for each slot. Item ID 0 means empty, item ID 1 is the default starting armor.
- `weapons[2]`: left and right hand weapon IDs.
- `items[10]`: general inventory slots.

## Environmental Damage

| Hazard | Damage Formula |
|---|---|
| Pit fall (Captive) | 5 + current_level * 2 |
| Industrial hazard (Liberation) | 5 + mission * 2 |

## Droid Configuration Screen

Entered via `STATE_DROID_CONFIG` before the first mission. Displays all 4 droids with their stats and equipment. Controls:

- **R**: rename a droid
- **S**: swap weapons between hands

Initial droid names are generated procedurally using `captive_generate_name` with seed 42.

## Source Files

- Droid struct and game state: `include/game_state.h`
- Game state initialization and logic: `src/engine/engine.c`
- Combat and tick logic: `src/main.c`
