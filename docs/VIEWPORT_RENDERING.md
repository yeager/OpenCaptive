# Captive Viewport Rendering

> Updated for v1.1.82. Rendering changes and test coverage are reflected in the release build.

Reference: captive.atari.org Technical/ViewRendering

## Status

The 19-cell traversal and ordered visibility cleanup below are implemented and
tested.  The original panel compositor is not yet recovered, so OpenCaptive
does **not** claim a pixel-parity Captive viewport.  Exact screen offsets are
deliberately not specified here until they are measured from a verified
original frame.

## Visible area

- 19 cells total
- Player at canonical cell 18
- Extends 4 cells forward, 2 cells left to 2 right
- All 4 facing directions normalized to "facing north" before rendering

The recovered DOS compositor builds a 160×112 work buffer. Its descriptor
destination fields are byte offsets in that buffer; the final screen copy
shows the first 144 pixels of every row. OpenCaptive keeps this distinction
explicit in `captive_dos_descriptor_destination_xy()` while the remaining
descriptor order and panel masks are being recovered.

Until that compositor is wired into the game loop, both modes use the
source-backed compatibility renderer, including creature sprites, so Original
mode remains playable. Enhanced mode additionally draws the modern HUD and
effects. Neither mode is presented as pixel-identical original Captive output.

PL5 panel and sprite transparency is likewise index-based: palette index 0
is transparent, while black indices 16 and 18 remain visible. The renderer
uses the retained PL5 index plane instead of inferring transparency from RGB.
The shared compositor also applies the recovered DOS descriptor flags for
horizontal mirroring and index-zero masking in the 160-byte work buffer.

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

The in-game F10 popup can toggle scanlines, CRT curvature, bilinear scaling,
texture filtering and dynamic lighting without restarting the game. These are
presentation options; they do not change the verified source assets.

- Sprites: monochrome bit patterns (black=colored, white=transparent)
- Composited via bitplane overlay
- Explosion/death: 2 bitmaps, pixel-level combination
- Font: 40 chars, bold = draw twice offset 1px right
- Radar: 28 distance-banded tiles (brown/white/red)
