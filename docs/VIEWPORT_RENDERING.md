# Captive Viewport Rendering

> Updated for v1.1.90. Rendering changes and test coverage are reflected in the release build.

Reference: captive.atari.org Technical/ViewRendering

## Status

The 19-cell traversal, ordered visibility cleanup, and the complete 112-entry
descriptor table extracted from CAPPO.EXE are implemented and tested as
recovery/compositor components. The descriptor table has been verified: all
entries have valid dimensions, source banks ≤ 4, and destinations within the
160×112 viewport with >75% pixel coverage. The active game path still uses a
source-backed compatibility renderer; the original per-cell dispatch sequence
and caller destination bases are not yet recovered, so playable pixel parity
is not claimed.

## Visible area

- 19 cells total
- Player at canonical cell 18
- Extends 4 cells forward, 2 cells left to 2 right
- All 4 facing directions normalized to "facing north" before rendering

The recovered DOS compositor builds a 160×112 work buffer. Its descriptor
destination fields are byte offsets in that buffer; the final screen copy
shows the first 144 pixels of every row. The complete descriptor table is in
`include/captive_viewport_descriptors.h` (112 entries organized by depth band:
d4 farthest at y=45 h=35, d3 at y=37 h=49, d2 at y=25 h=70, d1 at y=9 h=98,
full-height at y=0 h=112). Floor/ceiling strips use source bank 4 (roof
sheet); wall panels use source bank 0.

The `descriptor_blit()` function maps packed PL5 source coordinates to decoded
pixel coordinates, handling mirror (CAPTIVE_DESC_FLAG_MIRROR_H) and mask-zero
(CAPTIVE_DESC_FLAG_MASK_ZERO) flags. Enhanced mode additionally draws the modern HUD and
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
