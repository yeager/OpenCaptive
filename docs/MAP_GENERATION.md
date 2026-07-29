# Captive Map Generation

Reference: captive.atari.org Technical/MapGen

## Seed formula

```
seed = ((mission - 1) * 11) + base
```

Example: Mission 17, Base 3 → seed 179.

## Map dimensions

- 64 × 32 cells, 1 byte per cell
- Divided into 16×8 sections (4 columns × 4 rows = 16 sections)
- 2-5 floors per map (Map 0 forced to 1 floor)

## Generation phases (30 total)

### Floor assignment
1. Pick random section, assign level 1
2. Pick adjacent connected section, assign same level
3. On out-of-bounds, increment level, restart
4. Remaining undefined sections randomly filled to match neighbors

### Path carving (digger)
- Max 12,289 moves
- Random direction from 6 possible (N/E/S/W/Up/Down), weighted
- Only carves if cell surrounded by 3 walls (prevents breaking existing paths)
- Floor transitions: destination must be surrounded by 4 walls; ladders added both sides
- Random rooms: max 50 cells, rectangle saved to internal list
- "Jumping Jack" T-intersections: up to 900 attempts

### Root path
- Every carved cell gets sequential ID (entrance = ID 1)
- Full scan: each cell points toward smallest neighboring ID → convergence on root

### Player start
- Always row 0, randomly chosen column (validated to avoid section edges)
- Map 0 hardcodes usable sections

## Feature placement

All placed via bounded random-attempt loops:

| Feature | Attempts | Conditions |
|---------|----------|------------|
| Doors | 300 | Free cell flanked by walls; laser-door computers 3-18 cells back |
| Raiser walls | map# (1-10), 20 (11+) | Max 4 open simultaneously, 4× door damage |
| Pushable walls | per room | Exit from room's stored block, target cell reserved |
| Wall holes | map# (<20), 80 (≥20) | Solid wall one side, open other; scans 4 cells east |
| Fake walls | from Map 7+ | Max 30 attempts, on floor cells |
| Spinners | from Map 5+ | Max 20 attempts, random facing |
| Holes (from ladders) | map# (≤40) | Converts unidirectional ladders |

## Door types

- Iris (max 8, 24 combinations)
- Laser (max 8)
- Blue
- Punchable (→ Button on map 1)
- Button
- Elevator (placeholder, replaced later)

## Final map

- Explosives object placed
- 2 "Ratt" messages (probe computer + explosives)
- "Base ready" signal
