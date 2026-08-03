# Start Menu

The start menu is the first screen presented when OpenCaptive launches. It provides game selection, data verification, settings, and information screens.

## Resolution and Layout

The menu renders at a fixed **960x600 pixel** canvas. The layout is organized as a 2-column, 4-row navigation grid containing 8 items:

| Column 1 | Column 2 |
|---|---|
| Captive (game card) | Liberation (game card) |
| Continue Captive | Continue Liberation |
| Settings | About |
| Controls | Quit |

Each game card is 390x280 pixels with a 30-pixel gap between columns. The cards feature animated procedural dungeon/city scenes with torch flicker effects, driven by the `anim_tick` counter.

## Navigation

- **Arrow keys** move between menu items in the grid.
- **Enter** activates the selected item.
- **Mouse click** directly selects and activates items.
- **D key** opens the Data Scanner.

## Game Data Status

Each game card displays a status indicator showing whether game data has been verified:

- **Green checkmark** (checkmark): all SHA-256 content hashes verified.
- **Red cross** (cross): one or more files missing or hash mismatch.

Verification is content-addressed (SHA-256 hashes of file contents, no filenames trusted):

- **Captive** requires **12** verified files.
- **Liberation** requires **7** verified files.

The `start_menu_check_data` function performs verification at startup and after data path changes.

## Continue Buttons

Continue buttons only appear when save files exist. The menu checks for:

- `opencaptive.sav` and `opencaptive_slot0.sav` (Captive)
- `liberation.sav` (Liberation)

Loading the most recent save is automatic when Continue is selected.

## Data Scanner

Pressing **D** opens the Data Scanner overlay, which:

- Scans the configured data path for game files.
- Reports the number of ZIP archives found (`scanner_zip_count`).
- Shows per-game verification results: files found vs. files required for both Captive and Liberation.
- Sets `scanner_done` when the scan completes.

## Settings Panel

The settings panel contains **16 items** with vertical scrolling (`settings_scroll` offset). Items in order:

| # | Setting | Values |
|---|---|---|
| 1 | Renderer | Enhanced mode toggle |
| 2 | Scanlines | On/Off |
| 3 | CRT Curvature | On/Off |
| 4 | Bilinear Filtering | On/Off |
| 5 | Integer Scaling | On/Off |
| 6 | Scale Factor | 1x - 5x |
| 7 | Fullscreen | On/Off |
| 8 | VSync | On/Off |
| 9 | FPS Limit | 0 (unlimited), 30, 60, 120 |
| 10 | Brightness | 0 - 100 |
| 11 | Contrast | 0 - 100 |
| 12 | Music | On/Off |
| 13 | SFX | On/Off |
| 14 | Data Path | Editable text field |
| 15 | Language | 19 languages, cycle left/right |
| 16 | Back | Returns to main menu |

**Defaults:** scale 3x, music on, SFX on, VSync on, integer scaling on, brightness 50, contrast 50, FPS limit 60.

### Data Path

The data path is an editable text field (up to 512 characters). Press **Enter** to confirm or **Escape** to cancel editing. The `data_path_editing` flag tracks edit mode, and `data_path_cursor` tracks the cursor position within the field.

### Language Selector

Cycles through 19 languages using **Left/Right** arrows. The current selection is stored as `lang_index`. See [Internationalization](Internationalization.md) for the full language list.

## About Screen

Displays:

- OpenCaptive version number
- Original game credits (Tony Crowther / Mindscape)
- Technology stack information

## Controls Screen

Displays a full keyboard reference for gameplay controls.

## Visual Assets

- **Logo**: loaded from `captivelogo.png` if present (`logo_img` with dimensions `logo_img_w` x `logo_img_h`).
- **Game card images**: separate `captive_img` and `liberation_img` bitmaps.
- **TTF font rendering**: DejaVu Sans Mono Bold at three sizes:
  - Title: 36pt (`font_title`)
  - Body: 18pt (`font_body`)
  - Small: 14pt (`font_small`)

The `ttf_ready` flag indicates whether TTF fonts loaded successfully.

## Source Files

- Header: `include/start_menu.h`
- Implementation: `src/engine/start_menu.c`
