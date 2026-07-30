# Captive Map Generation

Reference: captive.atari.org Technical/MapGen

## Implementation status

This document records the observed **original** Architect algorithm. It is not
a statement that the current C implementation reproduces every phase below.
`map_generate_base()` currently keeps the 64×32 allocation, documented seed
formula, early-map section restrictions and deterministic API, but its PRNG,
floor assignment, digging and feature placement remain a research prototype.
It is not used by the default Captive runtime.

Any parity implementation must be validated against a map emitted by the
original MapGen executable or an independently reproducible original fixture;
matching the prose below is not sufficient.

## Recovered original module

The verified Amiga medium contains the MapGen recovery payload with compressed
SHA-256 `bb5a96b9041e98e5b65a36b5645f2bfe0cbecf68c07479d35f7b4f76ed191118`.
RNC1 expansion has SHA-256
`b746eb7619a7746eacab2d0a0b2b4b7c42ab34177a22974248d710acf3047ee0` and
produces one 29 304-byte Amiga HUNK code segment. The module contains the
documented 12 289-move, 900-attempt, 300-attempt, 80-attempt and 50-cell
limits. OpenCaptive verifies those bytes and HUNK shape during Amiga media
verification, but does not execute or emulate this code at runtime.

### Code observations

Offsets below are relative to the start of the recovered 29 304-byte code
segment, not an Amiga load address. They are reproducible checkpoints for a
future port:

| Code offset | Observed instruction/data behaviour | Constraint it establishes |
| --- | --- | --- |
| `0x4dd4` | Initializes a word to `300` before the bounded generation calls. | One original placement/search loop is capped at 300 attempts. |
| `0x4dda` | Initializes a word to `900` in the same setup cluster. | A separate original search loop is capped at 900 attempts. |
| `0x4e34` | Loads `99` and uses `DBRA`, producing exactly 100 iterations. | This cluster has an explicit 100-pass bound. |
| `0x4f68`–`0x505c` | Stores up to six candidate coordinate records and rejects coordinates above `63` or `31`. | Candidate-space coordinates are constrained to the documented 64×32 map. |

These observations establish control-flow facts only. They do not assign a
specific game feature to each loop and must not be used to alter the C
prototype until an original map output can confirm the associated phase.

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
