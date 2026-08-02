# OpenCaptive Release Notes

## v1.1.20 (2026-08-02)

### Liberation: FNT font decoder + wiki updates
- `CHAR` container: 114 proportional-width glyphs, 7 rows, 2 bitplanes (foreground + drop-shadow)
- 2 font variants: 0Liberation (large, up to 9px) and 1Liberation (small, up to 6px)
- gamemenu.spr confirmed as standard AmSp bank
- Updated all wiki pages with decoded format documentation

## v1.1.19 (2026-08-02)

### Liberation: Img sprite format decoded
- `ImgA` container: uint16 sprite count + uint32 offset table
- Simple sprites: width, height, 1–6 color bitplanes (planar Amiga layout) + mask plane
- Multi-frame sprites: frame count + offset table, each frame a sub-sprite (LOD variants)
- Verified all 361 sprites across 4 files: MainSp (158), backpack (176), taxi (4), 3dView (23 multi×6 frames)
- FNT font format identified: `CHAR` magic, 114 glyphs × 16 bytes, 8px wide 2-plane bitmap
- gamemenu.spr confirmed as standard AmSp bank (already decoded)

## v1.1.18 (2026-08-02)

### Custom features system
All features behind flags, parity mode stays pristine. Enable individually or `--all-features`.

- **HD upscaling**: xBRZ 2x/3x/4x edge-aware pixel art upscaler (`--hd-upscale`, `--upscale-factor`)
- **Widescreen viewport**: extended viewport width beyond original bounds (`--widescreen`)
- **Multi-slot quicksave**: 10 save slots, F5 save, F6 cycle slot, F9 load (`--quicksave`)
- **Minimap overlay**: real-time minimap with configurable size/opacity, F8 toggle (`--minimap`)
- **Mouse-look**: FPS-style mouse turning with configurable sensitivity (`--mouse-look`)
- **Debug HUD**: cell type, position, direction, droid stats, PRNG seed, F7 toggle (`--debug-hud`)
- **Speed control**: adjustable game speed 0.25x–4x, numpad +/- (`--speed`)
- **Fast travel**: skip traversal in cities (`--fast-travel`)
- **Automap**: permanently reveals visited cells on minimap (`--automap`)
- **Dynamic lighting**: distance and normal-based per-pixel shading (`--dynamic-lighting`)
- **Audio reverb**: 4-tap delay reverb on dungeon audio (`--reverb`)
- **Replay system**: record and playback input sequences (`--replay-record`, `--replay-play`)
- **Cross-save**: portable binary save format for cross-platform transfer (`--cross-save-export`)
- **Texture filtering**: bilinear filtering on wall textures (`--bilinear`)
- **Config persistence**: save/load feature settings from file (`--features-config`)

## v1.1.17 (2026-08-02)

### Liberation: x3g polygon record format decoded
- 36-byte fixed header: type, record size, normals, render flags, color/texture, UV rect
- Variable vertex refs as EXVL byte offsets (÷16 = vertex index), with closing ref
- Record sizes: 40 (point/sprite), 44 (triangle), 46 (quad)
- Full X3gPolygon struct with parsed fields (normals, flags, color, UV, vertex indices)
- Verified against all 3 x3g test files (Objects, people, 0CityVectors)

## v1.1.16 (2026-08-02)

### Liberation: VGM wall texture decoder
- Decoded VGM format: 4 concatenated AmSp (AMOS Sprite Bank) banks per file
- 152 total sprites per VGM file (42+45+24+41 across banks), 4bpp with mask
- 71 wall texture sets (Wall01–Wall71.VGM, each 167,766 bytes)
- Delegates per-sprite decoding to existing AmSp decoder

### Liberation: x3g 3D vector format parser
- Decoded IFF FORM O3DG container: OFFS header (object count + offsets), VCDO sub-forms
- EXVL vertex lists: 16 bytes per vertex (x, y, z, group, 4 reserved int16s)
- PLST polygon list data preserved as raw buffer (record format TBD)
- Tested against Objects.x3g (3 objects), people.x3g (4), 0CityVectors.x3g (33)

## v1.1.15 (2026-08-02)

### Liberation: CityGen grid — remaining subroutines
- Building shape resolution (sub_07D2): walks building records, resolves origin via directional traversal
- Connection table init (sub_1766): 51-byte table, seed-derived coordinates
- Building connectivity (sub_097A): PRNG-based direction assignment for unconnected buildings
- Building record cleanup (sub_0A08): mask AND 0x0FFF/0xBF/0x2F on 4-byte records
- Road feature placement: 3 variants — lamp post (0x21, ×10), post box (0x22, ×4), phone box (0x23, ×1)
- Road feature inner loop (sub_160E): directional walk from PRNG position, target cell matching, adjacent-cell placement
- Advanced feature placement (sub_0A80, difficulty ≥ 4): retry limit 0xFF, up to 51 iterations with plane0 backup/restore
- Road-adjacent wall placement (sub_0ECC, difficulty ≥ 4): walk from random position to cell 0x1F, search for connectable cells
- Entry point finder (sub_0180, difficulty ≥ 4): 30 attempts to find road cell via feature placement, walk to building origin
- Finalize pass (sub_24B8): iterates all 4096 cells, ~20-case type dispatch converting generation cells to output values, sets entry point to 0x0A

## v1.1.14 (2026-08-02)

### Liberation: ArcD compression decoder
- Disassembled PlotGen 68k decompressor (offsets 0x302-0x520) with full parity
- Huffman+LZSS format: 3 tables per block, canonical codes, bit-reversed match values
- Verified bit-exact decompression of all 3 text files: PGE.txt (16,304), DTE.txt (14,136), CTE.txt (17,809)

### Liberation: CityGen grid (64x64)
- Disassembled CityGen 1.12 Amiga executable (10,824 bytes code)
- 8x8 meta-grid with PRNG-biased road walking, 13 tile templates for expansion
- 3 grid planes, difficulty-gated generation phases, block placement with road adjacency

### Liberation: BuildingGen (city generation)
- Disassembled BuildingGen Amiga executable (23,252 bytes code)
- 9 building types, road connection graph, city/building name generation
- German syllable pairs + Greek letter suffix for city names

### Captive: combat and creature systems
- Combat damage formula (lo*hi encoding, shift-left scaling)
- 25 creature types with HP/speed/sprite tables from CAPPO.EXE
- Spawn placement algorithm (8 categories, subcell positioning, direction modifiers)
- XP/level-up formulas (per-skill thresholds, growth rates, caps)
- Weapon damage tables (18 melee, 20 ranged entries)

### Captive: MapGen cellular automaton
- 4 CA rule types recovered from CAPPO.EXE (maze/rooms/open/mixed)
- Generator placement algorithm
- DOS PRNG variant at 0x3D54

### Captive: sound system
- AdLib MIDI playback with OPL2 FM synthesis (26 instrument patches)
- Complete SFX bytecode interpreter (13 opcodes, 4 simultaneous voices)
- All 63 MIDI files mapped to 14 music categories
- SFX event mapping from INT 61h call sites

### Captive: items and pricing
- Item database with type code classification from CAPPO.EXE
- Item pricing formula (rol16-based scaling)
- 23 weapon variant tiers with prices
- Item availability gating by difficulty

### Documentation
- Updated README with comprehensive status tables
- Updated all wiki pages with reverse engineering progress
- ArcD format fully documented in File-Formats wiki

## v1.1.13

Initial public release with Captive and Liberation format decoders, verified
original presentation, viewport renderer, sound system, and CI/CD pipeline.
