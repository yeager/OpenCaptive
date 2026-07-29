# Liberation: Captive 2 Engine

## Overview

The Liberation engine implements a cyberpunk city exploration game with procedural building generation, NPC encounters, and detective-style gameplay.

## City Generation

- 32x32 grid world
- Buildings placed in 6x6 blocks with 2-wide streets
- Each building: 3-4 cells wide/tall, 1-4 floors
- 8 building types with distinct colors and contents

### Building Types

| Type | Color | Content |
|------|-------|---------|
| Residential | Blue-grey | Living quarters |
| Commercial | Green-grey | NPCs, shops |
| Industrial | Brown | Machinery |
| Government | Blue | Terminals |
| Prison | Red | Target buildings |
| Hospital | Green | Medical |
| Police | Dark blue | Security |
| Shop | Yellow | NPCs, commerce |

## Building Interiors

Each floor is a 16x16 grid with:
- Perimeter walls
- 2-6 internal walls creating rooms
- Doors at random positions in walls
- Ground floor entrance at bottom center
- Elevators in multi-floor buildings (top-right area)
- Terminals in some rooms (green cells)
- NPCs in commercial/shop buildings

## Navigation

### City Mode
- WASD/arrows: move on city grid
- Enter/F: enter building at current position
- Overhead color-coded map view
- Red marker shows mission target

### Building Mode
- WASD: direction-based movement (FPS style)
- A/D: turn left/right
- F: interact (elevator, exit)
- ,/.: elevator up/down
- Escape: exit to city
- Top-down minimap rendering

## Mission System

- Random target building selected at initialization
- Player must navigate city to find and enter target
- Clue system (planned): terminals reveal target location
