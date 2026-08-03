# Save and Load

OpenCaptive uses two distinct binary save formats: **OCSV** for Captive and **LSAV** for Liberation.

## Quick Reference

| Action | Key |
|---|---|
| Save (quicksave) | F5 |
| Load (quickload) | F9 |
| Cycle save slot | F6 |

## Captive Save Format (OCSV)

Captive saves use the OCSV binary format (version 3). The default file is `opencaptive.sav`, with 10 numbered slots: `opencaptive_slot0.sav` through `opencaptive_slot9.sav`.

### Header

```
Offset  Size    Field
0x00    4       Magic: 0x4F435356 ("OCSV")
0x04    4       Version: 3
0x08    4       Game type (GAME_CAPTIVE or GAME_LIBERATION)
0x0C    4       Mission number
0x10    4       Mission seed
0x14    4       Base ID
0x18    4       Party X position (signed)
0x1C    4       Party Y position (signed)
0x20    4       Party direction (0-3)
0x24    4       Current level (signed)
0x28    4       Number of levels (signed)
0x2C    4       Generators total (signed)
0x30    4       Generators destroyed (signed)
0x34    4       Gold (signed)
0x38    4       Number of creatures (signed)
0x3C    4       Number of puzzles (signed)
0x40    4       Tick counter
0x44    4       Selected droid (0-3)
```

### Body

After the header:

1. **Droids**: 4 raw `Droid` structs (see [Energy and Body Parts](Energy-and-Body-Parts.md)).
2. **Map cells**: For each level (up to `num_levels`), `MAP_HEIGHT` (32) x `MAP_WIDTH` (64) bytes, one `CellType` byte per cell.
3. **Creatures**: `num_creatures` creature records.
4. **Puzzles**: `num_puzzles` puzzle records.

The format uses **native endian** (no byte swapping). The loader validates all fields strictly: magic must match, version must match, direction must be <= `DIR_WEST`, all counts must be non-negative and within bounds. Data is loaded into a temporary restored state and only committed on full success.

### API

```c
bool save_game(const GameState *gs, const CreatureList *creatures,
               const PuzzleList *puzzles, const char *path);
bool load_game(GameState *gs, CreatureList *creatures,
               PuzzleList *puzzles, const char *path);
```

## Liberation Save Format (LSAV)

Liberation uses its own binary format with **big-endian** byte order (explicit MSB-first encoding via `write_u16`/`write_u32`).

### Binary Layout

```
Offset  Size    Field
0x00    4       Magic: "LSAV" (ASCII)
0x04    2       Version: 1
0x06    2       Seed high word
0x08    2       Seed low word
0x0A    2       Difficulty
0x0C    2       Mission number
0x0E    4       Gold
0x12    4       Tick counter
0x16    2       City X position (signed)
0x18    2       City Y position (signed)
0x1A    1       Facing direction (0-3)
0x1B    1       Number of droids (max 4)
```

### Droid Records (per droid)

```
Offset  Size    Field
+0x00   16      Name (null-terminated, padded)
+0x10   2       HP (signed)
+0x12   2       HP max (signed)
+0x14   2       Energy (signed)
+0x16   2       Energy max (signed)
+0x18   1       Level
+0x19   8       Skills (8 bytes)
+0x21   16      Equipment (8 x uint16)
```

### Mission Complete Bitmap

After the droid records:

- **32 bytes**: a 256-bit bitmap tracking mission completion. Bit N is stored at byte `N/8`, bit position `N%8`.

### Trailer

```
Offset  Size    Field
+0x00   2       Generators destroyed
+0x02   2       Generators total
```

### API

```c
bool lib_save_write(const LibSaveData *data, const char *path);
bool lib_save_read(LibSaveData *data, const char *path);
void lib_save_from_state(LibSaveData *data, const GameState *gs);
bool lib_save_is_mission_complete(const LibSaveData *data, int mission);
void lib_save_set_mission_complete(LibSaveData *data, int mission);
```

## Continue from Menu

When the player selects Continue from the start menu, the most recent save file is loaded automatically. The menu checks for save file existence at startup via `start_menu_check_saves`.

## Source Files

- Captive save: `include/save_load.h`, `src/engine/save_load.c`
- Liberation save: `include/liberation_save.h`, `src/game/liberation_save.c`
