# Captive Viewport Rendering

Reference: captive.atari.org Technical/ViewRendering

## Status

The 19-cell traversal and ordered visibility cleanup below are implemented and
tested.  The original panel compositor is not yet recovered, so OpenCaptive
does **not** claim a pixel-parity Captive viewport and must not substitute a
generated perspective scene for it.  Exact screen offsets are deliberately
not specified here until they are measured from a verified original frame.

## Visible area

- 19 cells total
- Player at canonical cell 18
- Extends 4 cells forward, 2 cells left to 2 right
- All 4 facing directions normalized to "facing north" before rendering

## Draw order

Back-to-front:
1. Front panels of the farthest wall row
2. Farthest drawable cells
3. For each nearer cell: its interior, then its wall panels and decorations

## Wall composition

- Each wall = 4 separate panels
- Each panel separates 2 individual cells (not a monolithic block)

## Performance characteristics (original)

- Pixel-by-pixel software rendering
- 1.5–12 fps depending on scene complexity

## Wall types

| Type | Behavior |
|------|----------|
| Raiser | 4× door damage, max 4 open, destroys items when closing |
| Pushable Ball | Right-click forward push |
| Hole | Toggleable, works with Anti-Grav Dev-Scape |
| Fake | Visually solid but passable; disabled by Vision Correction Optic |
| Panel | Single trigger, variable outcomes |
| Bars Puzzle | Number-matching, needs clipboard |
| Button Puzzle | Blue-square combo, needs clipboard |
| Lever | Single/triple, binary state |
| Hidden Button | Grate-trap unlock |
| Power Socket | Charges droids; kills non-droids |
| Shootable Blockage | Invisible, dissipates on any hit |

## Internal graphics

- Sprites: monochrome bit patterns (black=colored, white=transparent)
- Composited via bitplane overlay
- Explosion/death: 2 bitmaps, pixel-level combination
- Font: 40 chars, bold = draw twice offset 1px right
- Radar: 28 distance-banded tiles (brown/white/red)
