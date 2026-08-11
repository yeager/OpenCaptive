# Puzzle System

> Updated for v1.1.103. Puzzle behavior is documented as implemented prototype functionality unless explicitly marked source-verified.

## Overview

The start menu's Controls screen documents F10. During a game, F10 opens the
runtime options popup for live display changes and optional cheats.

Puzzles are wall-mounted interactive elements that control doors and provide resources. They are procedurally generated during map creation.

## Puzzle Types

### Button (PUZZLE_BUTTON)
- Single press toggles linked door open/closed
- Placed on wall faces adjacent to doors
- ~33% of doors get a button

### Lever (PUZZLE_LEVER)
- Binary on/off state
- Unlocks linked locked door when correct state matches solution bit 0
- Linked to nearest CELL_DOOR_LOCKED

### Triple Lever (PUZZLE_TRIPLE_LEVER)
- 8 states (0-7), cycles on each interaction
- Must match solution value (1-7) to unlock linked door
- 33% chance to be triple instead of single lever

### Power Socket (PUZZLE_POWER_SOCKET)
- Available from level 2+
- 9 charges, each restores 420 energy to selected droid, capped at the droid's energy maximum
- Placed on wall faces with ORNAMENT_PIPE visual

### Button Combo (PUZZLE_BUTTON_COMBO)
- The current prototype exposes one panel face and toggles the bit for that face.
- Generation therefore chooses a reachable one-bit solution; arbitrary 8-bit
  codes are not generated until the full eight-button panel is implemented.
- Clipboard hints show the stored code in hexadecimal.

## Generation

- Buttons placed near doors: walk door cells, check adjacent floor cells for viable wall face
- Levers placed in random rooms: find floor cell adjacent to wall, link to nearest locked door
- Power sockets: one per level from level 2+, placed in random rooms
- Maximum 128 puzzles per level set
- Seeded PRNG ensures deterministic placement for save/load

## Interaction

Press F to interact. Priority order:
1. Check current cell + facing direction for puzzle
2. Check cell ahead + opposite face for puzzle
3. Fall through to general combat interaction

## Wall Ornaments

Each puzzle sets a wall ornament on its face:
- Button → ORNAMENT_BUTTON
- Lever/Triple → ORNAMENT_PANEL
- Power Socket → ORNAMENT_PIPE
