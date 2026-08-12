# Captive technical notes

> Documentation baseline: v1.1.105. Runtime parity claims remain scoped to the verified boundaries described below.

## Runtime model

Captive's recovered presentation currently consists of the original 320×200
HUD shell, the documented visibility rules, and a source-backed compatibility
viewport. Legacy `DungeonLevel` and gameplay structures remain in the source
tree as reverse-engineering notes; they are not decoded original map or save
state.

### CAPPO keyboard path recovered from disassembly

For an authentic runtime capture, run the real `CAPPO.EXE` in DOSBox-X and
open its debugger with `Alt+Pause`. At the debugger prompt, use
`MEMDUMPBIN 0 0 100000` and copy the resulting `MEMDUMP.BIN` without changing
its contents. This is the emulator-memory capture consumed by the OpenCaptive
runtime bridge; it must be captured after the desired real keypad action, not
replaced by a generated map or a hand-written state file.

Use `tools/run_captive_dosbox_x.sh DATA_DIR` for a clean Mission 0001 emulator
profile. It launches the real `CAPTIVE.BAT 1` chain unless a different command
is explicitly supplied. It
forces VGA-only hardware, `surface` output, integer `normal2x` scaling,
disabled aspect correction, and disables DOSBox-X's `[video]` `memory io
optimization 1`. The latter is important for CAPPO's planar VGA writes: with
the optimization enabled DOSBox-X emits the characteristic repeated-glyph and
arrow corruption instead of the real game pixels. The profile also avoids
inheriting unrelated global settings such as `svga_s3`, stretched output or
the debugger terminal as the game surface. On macOS the helper selects
`/opt/homebrew/bin/dosbox-x` when present, or the path in `DOSBOX_X_BIN`, and
prints the selected version. This prevents an older PATH installation from
silently being used. The helper rejects direct `CAPPO.EXE` launches because
they skip the original video-mode initialization and are not valid parity
runs.

The built OpenCaptive binary exposes the same source-faithful path directly:

```sh
./build/opencaptive --data-dir DATA_DIR --captive-authentic
```

This starts the original `CAPTIVE.BAT 1` in DOSBox-X and leaves the original
window in control of input, rendering, audio, landing and dungeon state. It
does not enter the native compatibility state or manufacture a replacement
map or roster. The isolated profile is copied beside development binaries and
inside the macOS app bundle's Resources; `DOSBOX_X_BIN` can select another
verified DOSBox-X executable.

The same handoff occurs automatically when a new DOS Captive game is selected
from the graphical start menu and `CAPTIVE.BAT` is present. If the original
DOS runtime is unavailable, the menu retains the source-authenticated
reference-frame fallback rather than inventing gameplay state.

The direct command-line path follows the same rule: `--game captive` hands off
to the authentic DOSBox-X runtime whenever no explicit `--captive-dos-dump` or
`--capture-frame` analysis operation was requested. Those two flags remain
intentionally headless/native because they consume a caller-supplied real
checkpoint; an ordinary launch must never silently open the incomplete native
compatibility shell or a generated dungeon.

The startup harness can reproduce the original INTRO selections without
inventing input files:

```sh
tools/captive_dosbox_intro.expect DATA_DIR
```

For the already verified Mission 0001 segment, the separate queue probe sends
one raw scan byte through CAPPO's actual IRQ1 queue while DOSBox-X is paused:

```sh
tools/captive_dosbox_queue.expect DATA_DIR 47
```

This is an emulator/disassembly probe only. It does not claim that the native
OpenCaptive event loop is already attached to CAPPO, and it never synthesizes
a map, save, droid roster or dungeon state.

The multi-step probe writes to an explicit output directory and rejects its
own result unless the new 1 MiB dump contains CAPPO's runtime-state strings:

```sh
tools/captive_dosbox_sequence.expect DATA_DIR 47 480 /tmp/cappo-sequence
```

The command can therefore produce a useful negative result: a dump with a
uniform VGA page or without CAPPO state is reported as an incomplete emulator
phase rather than being accepted as a parity frame. A successful probe still
requires the normal `tools/verify_captive_dos_dump.sh` byte-exact comparison.

The unpacked DOS executable installs its keyboard IRQ1 handler at relative
offset `0x065c`. In the verified Mission 0001 DOSBox-X memory image the
relocated IRQ/source-bank segment is `0824`, so the handler queue is observable
at `0824:004e..0057`; this is not CAPPO's active code/text segment. CAPPO's
active relocated code/text image is identified independently by its known
strings: `FLIGHT PATH SET`, `ARRIVED AT DESTINATION`, and `LANDING SUCCESSFUL`
all resolve with relocation base `0x7E30`, while the verified data segment is
`DS=0x2942`. The handler reads raw XT/AT
scan bytes using byte `CS:0x004e` as the queued-byte count, byte `CS:0x004f` as
the ring index, and eight bytes at `CS:0x0050`. The game loop consumes that queue
through the matrix conversion at `0x0203`; it does not use the ordinary DOS
keyboard buffer while CAPPO is running. This explains why a generic BIOS-buffer
injector cannot verify keypad navigation.

The verified keypad mapping is therefore raw scan code `0x47` for keypad 7
(ORBIT) and `0x49` for keypad 9 (LAND), with the original `0x01` escape code.
These findings are a disassembly reference for the live emulator bridge; no
input state is fabricated in the game runtime. A live post-landing dump is
still required before claiming full viewport parity.

The application bridge has also been exercised end-to-end with:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/opencaptive \
  --data-dir /Users/bosse/.opencaptive/captivedebug/captive \
  --captive-dos-dump capture/original-captive/captive/MEMDUMP.BIN \
  --capture-frame /tmp/cappo-live.ppm
```

The resulting native frame matched the standalone DOS-VGA extraction byte for
byte. This validates the bridge, but it is still a single captured emulator
state rather than full mission playthrough evidence.

The same check is reproducible with
`tools/verify_captive_dos_dump.sh MEMDUMP.BIN DATA_DIR [BUILD_DIR]`. The script
rejects any dump that is not exactly 1 MiB or lacks CAPPO's own runtime-state
strings before it compares decoded frames; this prevents an early boot/debugger
buffer from being mistaken for gameplay evidence. It never creates or modifies
game data.

The Mission 0001 reference target is measured from the verified marker in
`holamap-target.png`: frame coordinate `(63,150)` maps to CAPPO cursor
coordinate `(58,108)` using the original map window. This is a selection
reference only. Live CAPPO testing shows that its displayed flight coordinate
can reach `150E-150N` while the status remains `FLIGHT PATH SET`; coordinate
equality is therefore not an Orbit/arrival proof.

The live DOSBox-X check now reproduces the first navigation action with the
original numpad path: three keypad-left inputs, three keypad-down inputs, one
keypad-up input, then keypad 7 with the cursor on the green marker. CAPPO
responds with its own `FLIGHT PATH SET` message. This proves original target
selection and ORBIT input delivery; it does not yet claim that the subsequent
space-flight, keypad-9 landing, and post-landing dungeon viewport have been
decoded into the native runtime.

The native fallback does not convert the landing transition into `STATE_GAME`
after a fixed delay. A captured dungeon frame is evidence of what CAPPO
displayed, not proof that the live runtime has reached that state; the landed
frame therefore remains gated on a real DOSBox-X/CAPPO state handoff.

In native reference mode, the space-navigation cluster is no longer a dead
end: keypad 8/2/4/6 and the corresponding on-screen arrows move the logged
cursor, and keypad 7 records the original `FLIGHT PATH SET` action when that
cursor is on the real green target. A second explicit `ORBIT` command is not
treated as arrival without a live CAPPO handoff; keypad 9 before that point
remains the original `SWAN NOT YET IN ORBIT` condition. This is an input-driven
reference state, not a timer, procedural arrival, or generated planet/dungeon.
The separate DOSBox-X dump loader still requires an exact VGA match before it
accepts a live runtime handoff.

## PL5 graphics

A PL5 image is 40,000 bytes: 320×200 pixels at five bits per pixel. Every five
input bytes encode eight output pixels using a bespoke bit layout, not a normal
linear bitstream. `pl5_decode()` expands it to indexed pixels; palette values
are converted to ARGB for renderer textures.

The decoder has a dedicated regression test. Bounds checks reject a truncated
payload instead of reading beyond it.

## DOS renderer observation log

The verified DOS executable has SHA-256
`71bcf404103f1ac2920800a8bc166939bb49a1204cf51bebce8aca7dd5faafde` before
LZEXE unpacking. Static analysis of its unpacked 16-bit code shows a 320×200
VGA presentation path and two distinct PL5 blitters. One copies decoded pixels
unconditionally; the other preserves destination pixels for transparent source
values. Both write four VGA planes when planar output is active. The executable
also contains the original game-screen, roof, wall and door asset references.

These observations validate the resource-to-framebuffer path and are useful
for checking the PL5 decoder. The complete relocated descriptor table contains
959 records copied from CAPPO.EXE file records 3..961 at unpacked file offset
0x216b0 and is declared in `include/captive_cappo_descriptors.h` and defined in
`src/data/captive_cappo_descriptors.c`. DOSBox-X
memory dumps confirm that file record 3 is runtime `DS:00c0` record 0. The
focused 112-entry viewport subset is in `include/captive_viewport_descriptors.h`.
Each descriptor is 8 bytes:
source_offset(u16), destination_offset(u16), width_bytes(u8), height(u8),
flags(u8), source_bank(u8). The `descriptor_blit()` helper handles mirror and
mask-zero flags. It is covered as a decoder/validation helper; the live
viewport still uses the compatibility renderer until descriptor selection and
original draw-order integration are recovered. A test verifies all entries
have valid dimensions and destinations within the 160×112 viewport.

### Recovered DOS dispatch boundary

The LZEXE-expanded program has SHA-256
`fa7d5ca76d26f614476ed41f27cf737084942e9216b20b4605734df9ede9aee4`.
Offsets in this subsection are relative to that expanded load module, not a
DOS segment address. The view path dispatches a sampled cell at `0x1fd1`.
Its handlers select a graphic ID, then enter the range-aware helpers at
`0x1ee4` and `0x1ef3`; those apply the original cell-depth adjustment before
calling the projection helper at `0x2bd7`. That helper is distinct from the
static graphic-descriptor entry.

The static entry at `0x2f55` computes `0x00c0 + graphic_id × 8` in the
original runtime's descriptor table. The descriptor supplies a source pointer,
destination position, dimensions and blitter flags. The common path selects
one of the planar copy routines at `0x37dd`, `0x393f`, `0x3c5f`, `0x3c62` or
`0x3c65`.
This is direct evidence that the projection is a table-driven sequence of
masked panel copies, rather than texture mapping. The complete descriptor table
has been recovered and verified — all 112 entries are extracted from the
expanded CAPPO.EXE binary at file offset 0x21698, with source coordinates,
destination positions, dimensions, mirror/mask flags, and source bank indices
matching the original DOS runtime.

### Original runtime descriptor fixture

The original one-megabyte DOS memory fixture has SHA-256
`9003c4a8818cb97f8299ac90cfe51e90e535ab9a725545526fe75f14ddb8dd7e`.
It captures a running DOS renderer, not an archive extraction. In that state
the source-bank selector table is present at segment `0x0824`; this is a
relocated data-area selector, not the CPU's active code segment. MZ relocation
changes the unexpanded table selector in the code to segment `0x2942`. The descriptor
array begins at `0x2942:0x00c0` and has eight bytes per graphic ID. The common
draw entry reads, in order, a little-endian source offset, a little-endian
destination offset, width, height, flags and source-bank index.

For example, graphic IDs `0x004`–`0x009` occupy source offsets near `0x6660`,
have heights of 49 bytes and route through bank zero. The original code then
resolves that bank through its relocated segment selector table before choosing
the planar copy routine. This fixture makes the record layout, runtime
relocation and source-bank indirection reproducible. It is an analysis oracle
only: no dump bytes are embedded in OpenCaptive or used as a game-data
substitute.

The static entry also proves a missing operand in a table-only reconstruction.
It loads the descriptor destination word, then adds the caller-provided `DI`
base before entering the blitter. Thus the destination word is a relative
panel position, not a complete screen coordinate. One view-dispatch path at
`0x1e7f` sets `DI` to `0x5000` before jumping to the entry; another recovered
path uses `0x53a0`. The selected base depends on the cell, range and
orientation. A single completed VGA frame can confirm source coverage, but
cannot reveal every per-call base or the draw order.

### CAPPO map state recovered from disassembly

The expanded CAPPO executable also establishes the original dungeon-map
addressing without requiring generated data. At `0x4936`, CAPPO accesses its
map byte array at `DS:0x7CB3`; the helper at `0x4949` computes the byte index as
`(y << 6) + x` and rejects coordinates above `x=63` or `y=31`. The active
coordinates are read from `DS:0x5E80` and `DS:0x5E82`. This proves the original
64×32 map layout and the live coordinate fields. The byte values are CAPPO's
cell/object codes, not OpenCaptive's native `CellType` enum, so no partial
interpretation is used until each code and its render path is source-verified.

### Panel matches against the captured frame

The fixture and the 320×200 VGA frame with SHA-256
`9003c4a8818cb97f8299ac90cfe51e90e535ab9a725545526fe75f14ddb8dd7e`
also make individual panel commands testable without treating the dump as
runtime input.  Source pixels are expanded with the original PL5 packing,
the transparent-write and horizontal-flip flag bits are applied, then the
opaque pixels are compared with the captured viewport.

| Graphic ID | Source | Flags | Captured viewport position | Evidence |
| ---: | --- | ---: | --- | --- |
| `0x010` | bank 0, offset `0x1a04`, 16×98 pixels | `0x02` | `(32,64)` | 1 511 of 1 568 source pixels agree; the rest are covered by later panels. |
| `0x01a` | bank 0, offset `0x1a0e`, 16×98 pixels | `0x07` | `(48,64)` | 1 356 of 1 356 opaque pixels agree exactly. |

These two commands establish the left-side 32-pixel panel pair for this
specific captured view.  They do not yet identify the original cell-to-graphic
selection table, the full draw order, or a general dynamic renderer; those
remain necessary before claiming viewport parity.

The captured selector table resolves the first seven source-bank slots to
these DOS PL5 content hashes:

| Bank | Hash |
| ---: | --- |
| 0 | `7edb8ee856a91e835ea86dda00af49fda3dae730d694bd7234b7fa96d711e296` |
| 1 | `dec7143f063c98459ab2f267ed135204cdee1b521eda9810b219e8c10e05c7e8` |
| 2 | `70e0b9bfbaa5dfd12643b50cbe10d0b664de2fb1106d8ff0f2fde1ce6f443bbe` |
| 3 | `ce00ba2bc78f160b934486fe101a90264163356e02e7acbea2a41cf5d125b017` |
| 4 | `fed16e510697e17123d474c08687de548076b26a55f08f1d00fd17e3fcdf9410` |
| 5 | `21db7daf64cff3b0cae19c3e7eb2057762df9110055e7253175024ecb146fb6b` |
| 6 | `63ffa6901b59d463b050088065503d386ca2f3813ed91d8e0833320f9df2fe11` |

This relation was proven by comparing each relocated source segment to the
corresponding full 40,000-byte hash-identified payload. It is a fixture for
this verified renderer state, not a substitute for the original loader's
asset-set selection.

### Destination-buffer correction

The descriptor's destination word is **not** an offset into a 200-byte-wide
PL5 source sheet, nor a complete displayed coordinate. The static entry at
`0x2f55` first adds the caller's `DI` base. In the native `0x3d42` copy path,
one five-byte source group is expanded to eight indexed output bytes and the
destination row advance is `0x140` (320 bytes). The alternative paths preserve
destination pixels for transparent source values. The low flag bit selects the
mirrored variant.

Consequently, a descriptor cannot be reproduced by decoding a source crop and
placing it at `destination / 200`. That tempting shortcut yields plausible
wall fragments but at incorrect positions and is not used by OpenCaptive. The
caller base and per-cell descriptor order must be recovered before native view
rendering can claim visual parity.

The renderer samples a 19-cell trapezoid rather than a full 5×5 view: five
cells at ranges four and three, three cells at ranges two and one, then the
left/current/right cells at range zero. It rotates that sample from the party
orientation into a fixed forward-facing order, removes cells hidden behind
walls, and draws back to front. `captive_view_window_build()` now reproduces
the verified 19-cell sampling order and the ordered original wall-occlusion
pass while retaining the earlier 5×5 work area for analysis. A cleared cell
does not take part in later wall tests, matching the original copied-map
behaviour. Panel projection and resource selection are still separate recovery
work, not inferred rendering.

The sampling footprint, ordered cleanup conditions and back-to-front boundary
are documented independently in [The Ultimate Captive Guide: View Rendering](https://captive.atari.org/Technical/ViewRendering/ViewRendering.php).
The implementation intentionally limits itself to those published rules until
the DOS panel tables are also recovered from verified executable/media bytes.

### Verified panel sheets

The five relevant DOS PL5 resources are selected only by SHA-256:

```text
47ad15b4a593c37880d0306b6a0f51b7a9f20615cf6a188f23716d5b48315524
43833e4a8df622f84d53698a76c6d18f910c1cca79c6b89cbfacc563f695356c
8b7301fc6c302fd673a81d23e7a99d715aa02d5b404c1e1edea19ceccccc9681
519d3ef4494f0e868479a90c8a47249b840598e382c7ba3272f417ce3daf5936
7edb8ee856a91e835ea86dda00af49fda3dae730d694bd7234b7fa96d711e296
```

The first digest decodes to the published `fed7-A` interior reference image.
This establishes that the source data is authentic, but also rules out the old
64×64 tile interpretation: each 320×200 sheet contains irregular, overlapping
preprojected panels. A parity renderer must recover the original panel source
rectangles, destinations, transparency convention and per-cell state table;
sampling fixed-size tiles cannot reproduce the reference viewport.

### Live descriptor compositor

#### Fail-closed runtime boundary

The Captive presentation path is source-backed only. If an original ANM,
holomap, landing, or dungeon frame is missing or invalid, the renderer clears
the frame or returns to an authenticated frame; it does not paint replacement
status text, procedural planets, generated rosters, or a compatibility map.
The native fallback holds the verified CAPPO flight and landing transitions
and never advances them on a timer. A complete live handoff still requires a
DOSBox-X dump recorded after the real orbit or landing action, with the
original CAPPO VGA surface populated at `A000:0000`. When that dump is
reloaded, OpenCaptive compares the complete 320×200 VGA surface against the
corresponding real checkpoint. Only an exact orbit match enters orbit, and
only an exact landed match enters the landed view; any other dump remains in
the current transition.

The original-mode runtime now executes the recovered descriptor bands through
the same 160-byte work-row layout used by CAPPO. The compositor is wired after
the verified GAME SCRN shell and before any optional enhancement layer. It
copies source pixels from the content-addressed PL5 sheets, preserves index
zero transparency, and exposes the first 144 pixels of each row at the
disassembly-verified viewport origin. The runtime table alignment was checked
against a real DOSBox-X memory dump; the first three records in the unpacked
file image are not runtime descriptor records.

This is a real-data rendering path, not a generated replacement scene. It is
deliberately not described as complete parity yet: the original runtime still
selects graphic IDs and destination bases from per-cell operands that are not
fully recovered. Until those operands and the original map/runtime records are
decoded, the compositor is a verified panel-draw boundary and the remaining
compatibility renderer stays available for isolated tests.

## ANM animation

ANM starts with a 768-byte VGA palette of 6-bit RGB triples. A little-endian
word at offset 768 marks the command end. Frame records are read backwards from
end of file; each ends in a little-endian total-size word. Frame deltas use:

```text
non-zero byte  => XOR that byte into the current output position
zero byte      => next byte is a skip count
zero + zero    => end of frame
```

The target is a 64,000-byte 320×200 chunky frame buffer. Reconstructing each
frame from XOR deltas preserves the original incremental animation behaviour.

## RNC Method 1

Atari ST and Amiga resources may begin with `RNC\x01`. The header contains
big-endian unpacked and packed lengths plus CRC16 fields. Method 1 uses three
Huffman tables per sub-block. The decoder validates lengths before allocating
or copying and is kept separate from container readers.

## Media containers

- **Atari ST:** FAT12 disk image; the BIOS parameter block begins at offset 11.
- **Amiga:** 80 tracks × 2 sides × 11 sectors × 512 bytes (880 KiB), with OFS
  or FFS filesystem structures.
- **DOS-style asset sets:** may be extracted or ZIP-packed; the VFS identifies
  required content by SHA-256.

## Architect map generation

The original map model is a single 2,048-byte, 64×32 allocation divided into
sixteen 16×8 physical sections. Logical floors are assigned across those
sections; changing level changes the logical floor offset, not the allocation
shape. The modern engine exposes those regions as `levels[]`, which is an API
adaptation rather than proof of byte-for-byte MapGen parity.

The documented mission/base seed is:

```text
seed = ((mission - 1) × 11) + base
```

The first base is seed zero. Architect uses the sparse physical sections 2, 6
and 10 (one-based numbering) and starts the player at `(30,0)`. OpenCaptive
implements and tests that special case. For maps 1–4, the documented usable
section sets progressively expand: row one plus 6–7; rows one/two plus 10–11;
the first three rows; then the first three rows plus 14–15.

The original generator has 30 ordered stages. The documented order includes
floor layout, offsets, root position, unconditional walls, elevators,
no-touch zones, digging/rooms, root directions, dead-end smoothing, fire,
generators, doors, puzzles, traps, encounters, decorations and exterior
generation. Current `map_generate_base()` preserves the allocation dimensions,
early masks, root special case and deterministic API, but it is **not yet a
byte-identical implementation** of those 30 stages. In particular, the
current PRNG arithmetic and feature placement must not be treated as recovered
original code until they are validated against original MapGen output.

### DOS MapGen disassembly (partial)

The DOS generator entry at 0x1C3C in the unpacked CAPPO.EXE operates on a
bitmask buffer (20 bytes per row, stride 0xA0 in viewport mode). Key stages:

1. **Random rectangle carving** (0x1C4B): Decrements iteration counter
   `[0x8D7F]`, generates random position (AND 0x7F, max 0x6F) and size
   (AND 0x7, INC → 1-8), writes wall data via bitmask OR/AND.
2. **Boundary cleanup** (0x1D1C): Scans 3×3 neighborhoods around each cell
   comparing against threshold 0x18. Fills border cells with 0x20 (floor).
3. **Cell type assignment** (0x1E50): Converts bitmask to cell records (8 bytes
   each). Values: 0x00=wall, 0x20=floor, 0x44=door. Checks grid boundaries
   at CX=0x40 (width 64) and BX=0x20 (height 32).

The map-generation PRNG at `0x8878` is the DOS variant: it computes
`state = state * 0x5E5 + 0x29`, performs three `ROR AX,1` operations, then
executes `XOR AH,0x08`; the low word is stored at `[0x92F8]`. CAPPO also has a
second unrotated helper at `0x889A`; it is not the map-generation entry.

The technical reference used for this boundary is the documented
[MapGen introduction](https://captive.atari.org/Technical/MapGen/Introduction.php)
and its linked stage pages. New MapGen work must add an original-output fixture
or an independently reproducible reference before claiming parity.

## DOS executable analysis

The verified unpacked executable (144,556 bytes) contains the complete game data tables. See [[Captive Game Data]] for the full extraction:

- **10 droid material grades** (SHIT through TITANIUX)
- **10 combat skills** (Brawling through Energy Weapon)
- **~40 items** with hex classification bytes
- **48 name-generation syllables** (8 consonants x 6 vowels)
- **14 music categories x 11 variations** (154 track slots)
- **4 sound drivers** (AdLib, Roland, PC Speaker, Sound Blaster)
- **16 ANM animation files** and **multiple PL5 graphics sheets**

The executable structure, dispatch tables and descriptor format are documented above. The disassembly data provides ground truth for reimplementing item tables, shop pricing, combat formulas, and name generation without guessing.

## Viewport renderer

`src/render/viewport.c` contains an experimental compatibility renderer based
on the 19-cell trapezoid from `captive_view_window_build()` and hash-verified
PL5 source sheets. It remains active so the game is playable, but its placement
and scaling rules are an approximation, not the original descriptor sequence.
Displaying it must not be described as pixel-identical DOS parity.

The active Captive path draws the verified original `GAME SCRN` shell and, when
available, an authenticated landed checkpoint. It does not draw the
compatibility viewport: the original DOS renderer is a back-to-front sequence
of descriptor-driven planar copies with caller-specific destination bases,
mask behaviour and per-cell ordering. Re-enabling a live viewport therefore
requires those commands to be recovered from the original runtime, then
compared against DOS-VGA captures. Until then, generated perspective, floor,
ceiling, door, creature and object pixels are not shown as Captive content.

### Raw DOSBox-X map inspection

`captive_map_dump MEMDUMP.BIN [DS-segment-hex] [source-bank-segment-hex]` is an
analysis-only tool. It
reads a caller-supplied, complete 1 MiB DOSBox-X memory dump and prints the
original 64×32 byte map at `DS:7CB3`, the active coordinates at `DS:5E80` and
`DS:5E82`, and the orientation field at `DS:5E84`. It deliberately preserves
CAPPO's raw cell bytes and performs no conversion to OpenCaptive cell types.
The default segment is `0x2942`, matching the verified relocated runtime
fixture. No dump bytes are bundled and the tool is never used as runtime game
data.

The same boundary now reproduces CAPPO's 5×5 copied neighbourhood from
`DS:7CB3` using the four orientation branches at `0x1818`. Outside-map cells
are reported separately instead of being replaced with a guessed wall or
floor. This makes the raw viewport input testable without claiming that the
later cell-code-to-descriptor dispatch is complete.

CAPPO's draw-order input is also exposed as 38 raw eight-byte records at
`DS:5B82..5CB2`, the list iterated by `0x2D09`. Each record is linked to its
`DS:12F1` window index and routed through the correct normal or overlay
handler. `captive_map_dump` prints this sequence from a supplied DOSBox-X
dump; the fields remain raw offsets until the handler operands are verified.

The first raw dispatch gate is also recorded without semantic renaming. CAPPO
has two branches: the normal branch at `0x1AC0` and the overlay/object branch
entered when `DL & 0x08` is nonzero at `0x1ABB`:
`raw & 0x7F` routes recognized values to CAPPO handlers at `0x1D72`, `0x1DFC`,
`0x1E13`, `0x1E35`, `0x201C`, `0x2065`, `0x206A`, `0x20C7`, `0x2103`,
`0x212F`, `0x2171`, `0x218C`, `0x21A0`, `0x21A9`, `0x26F8`, `0x2701`,
`0x272E` or `0x445A`. The route API exposes these as handler identifiers;
descriptor IDs and visual meanings are deliberately left unresolved until
the corresponding original table operands are verified. The normal branch is
exposed separately for raw `0x00..0x1B` and `0x3E`, including its `0x1C90`,
`0x1D17` and `0x1DF2` entry points; a raw byte is not assigned one universal
visual meaning across both branches.
The dump tool also prints a 5×5 matrix of these handler addresses, so an
emulator dump can be compared directly with the disassembly before any
viewport pixels are composed.
When a recovered handler operand exists, it additionally decodes the real
eight-byte descriptor record and reports its source offset, destination,
dimensions, flags, source bank and whether the record is drawable or a zero
dimension sentinel. The default source-bank segment `0x0824` is the segment
observed in the supplied runtime fixture and can be overridden for another
real dump.

The next analysis boundary recovers descriptor operands for handlers whose
immediate formulas are now complete. For `0x1D17`, the operand is the
orientation-selected word at `DS:5CC2` plus record byte 6. For `0x1E35`,
record byte 6 indexes the real `DS:1276` table; CAPPO adds `0x17E`/`0x18B`
when flag bit 2 is set, or `0x192`/`0x199` on the ordinary path. `0x1DFC`
and `0x1E13` expose their corresponding `+0x1ED` and `+0x157` operands.
The API and dump tool emit these only after the recovered handler-local table,
orientation and state checks pass, and label them `handler-preconditions-pass`.
The IDs are the actual operands from the supplied dump; descriptor-table
sentinel validation and the remaining overlay bands are still separate work.
For the captured navigation frame this produces real operand sequences such
as `01DC 01E3` (the `DS:1276[3]` path) and `0406`/`0405` (the `DS:5CC2`
path). No descriptor ID is synthesized when a source table marks a route as
inactive.

Overlay handler `0x2103` is also decoded. CAPPO selects base `0x2E6` or
`0x2EF` from the relocated `0x8CFD` mode and the real planet-coordinate
parity, then applies `0x20E4`'s BP range rule (`0`, `>10` and `4` reject;
`5..10` decrement). The resulting descriptor operand is reported only when
those exact conditions are satisfied. This remains an analysis path; it does
not substitute generated object or sprite data for an unresolved overlay
band.

Overlay handlers `0x2171` and `0x218C` now expose their two real descriptor
operands each. `0x2171` calls `0x20E4` for bases `0x349` and `0x352`; `0x218C`
calls `0x20F3` for bases `0x35B` and `0x367`. The BP acceptance and decrement
rules are applied exactly as in CAPPO, and rejected values produce no operand.
These are still source-locked operand paths only: no pixels or replacement
object data are invented while the remaining overlay handlers are unresolved.

The same `0x20E4` path is now decoded for `0x206A` (`0x6B` / `0x74`) and
`0x20C7` (`0x86` / `0x7D`). Their preceding `0x2768` calls are renderer-side
effects and do not supply descriptor IDs, so the analysis exposes only the two
actual operands from each handler and keeps the original rejection rule.

The shared `0x2089` state-table path is decoded for `0x2065` and `0x21A0`.
`0x26F8` and `0x2701` additionally expose their `0x2715` operand before the
same two `0x2089` operands. `0x212F` exposes its `0x2715` operand plus the two
or four `0x20E4` operands selected by `DS:5EF4` bit 0. The state table remains
caller-owned (`DS:0E3F:93AE`); an absent state match yields no fabricated ID.

For `0x21A9`, the fixed odd-direction `0x21D1` branch is also exposed when
its exact `BP=0x0C` precondition holds. CAPPO selects `0x0D0`, `0x119/0x11A`
or `0x2AD/0x2AE` from `AX & 7`, then always calls the shared `0x2089` path.
The other `0x21D1` branches depend on additional runtime state and remain
unresolved rather than being filled with guessed descriptors.

The compatibility viewport no longer fills missing object/lock sprites with
procedural colored rectangles. It draws only decoded pixels from the verified
`OBJECTS.PL5`/door sheets; absent real source data leaves the corresponding
area untouched. This prevents synthetic content from being mistaken for
Captive artwork while the raw CAPPO compositor is still being completed.

### Raw runtime compositor

`captive_dos_runtime_render()` is now the first executable bridge for a
complete DOSBox-X memory image. It consumes CAPPO's live DS map, copied 5×5
window, 38 draw-order records, descriptor table and packed source banks, then
executes those original descriptor copies into the 160-byte work buffer. It
does not convert cells to `GameState`, use `map_gen`, or add a replacement
background. Unknown handler paths simply draw nothing, matching the current
evidence boundary.

The `captive_runtime_render` tool exercises this bridge against a caller-owned
`MEMDUMP.BIN`. The dump is deliberately not shipped or scanned as game data;
it is only an emulator observation. The next parity gate is a fresh DOSBox-X
dump taken after each real movement input, followed by an exact 144×112
viewport comparison. Until that gate passes, the production Captive path does
not claim live-dungeon parity.

The dump tool now also evaluates the shared `0x1C90` helper. It preserves
CAPPO's three-byte row padding: a neighbour read landing in that padding, or
outside the 5×5 copied window, is reported as `unknown`, never as a fake cell.
For direction values 1, 3 and 2 it reproduces the original `[di+9]`,
`[di+1]`, `[di+7]` and `[di-1]` comparisons against `0x1A`, including the
special raw-code bypasses (`0x1A`, `0x1C`, `0x22`, `0x24`). The result is a
helper outcome, not a universal handler gate: `0x1D17` and `0x1E35` continue
after calling `0x1C90`. This separates its possible special-case descriptor
from the handler's own descriptor operands and is the next input needed before
enabling the active compositor.

The native OpenCaptive compatibility tick is disabled for Captive. It must not
invent droid names, hit points, energy regeneration, combat events, messages or
sound effects while the original CAPPO runtime is not driving the state. A
Captive game frame is therefore accepted only from a real DOSBox-X memory dump
or from a hash-verified original reference frame. Missing runtime bytes leave
the unsupported portion empty; they are never replaced by a generated map,
landing point, dungeon, roster or story sequence.

## SFX system

10 game-level SFX types mapped to CAP_A.BIN AdLib sequences via INT 61h disassembly:

| Game SFX | Index | Sequence offset | Description |
|----------|-------|----------------|-------------|
| SFX_GENERATOR | 8 | 0x56B6 | Generator hum |
| SFX_HIT | 13 | 0x5763 | Melee hit |
| SFX_LEVEL_UP | 15 | 0x56AC | Level up fanfare |
| SFX_DEATH | 17 | 0x578E | Droid death |
| SFX_BUTTON | 18 | 0xA48C | Button press |
| SFX_PICKUP | 20 | 0x50B5 | Item pickup |
| SFX_SHOOT | 22 | 0x92FE | Ranged weapon fire |
| SFX_DOOR_LOCKED | 23 | 0x82CC | Locked door rattle |
| SFX_DOOR_OPEN | 24 | 0x8285 | Door opening |
| SFX_STEP | 26 | 0x67D0 | Footstep |

Driver remapping formula: game index < 4 = silent, 4-14 = direct pass-through,
15+ = index - 11. This maps the 10 game-level SFX indices into the 49-entry
sequence table in CAP_A.BIN.

## Creature damage

Creature HP, category, speed and sprite routing are recovered from the DOS
tables documented below. Creature attack damage is not yet source-verified.
The code around unpacked CAPPO offset `0x5380`/the corresponding attack
bytecode constructs a weapon damage word; it is not proof of the enemy attack
record or of a category/level creature formula.

OpenCaptive therefore keeps a bounded category/level compatibility formula in
the runtime, clearly separated from the recovered data. It must not be
described as original-game parity until the enemy attack record and caller are
identified.

## Item system

9 type code prefixes classify all items in the CAPPO.EXE item database:

| Code | Class | Examples |
|------|-------|----------|
| 0x00 | Melee weapon | DEV-SCAPE, KNUCLE-DUSTER, MINE, DIE, BALL, MAP |
| 0x08 | Ranged weapon | BATTERY |
| 0x10 | Ammo | CARTRIDGES, SHELLS, LASER PACK, SONIC PACK |
| 0x20 | Armor (chip) | DROID CHIP |
| 0x21 | Shield (equipment) | OPTIC, CAMERA, BATTLE-GLOVE, WAR-BLADE, PISTOL |
| 0x27 | Battery (body part) | HEAD, CHEST, LEG, FOOT, HAND, GOLD |
| 0x30 | Key (ranged weapon) | COLT, MAGNUM, RIFLE, automatics, energy, heavy |
| 0x60 | Body part (explosive) | EXPLOSIVES |
| 0x65 | Special (body variant) | ARM |

### Body armor defense values

Defense values per body part slot, applied when computing damage reduction:

| Slot | Defense |
|------|---------|
| HEAD | 10 |
| CHEST | 15 |
| ARM | 8 |
| LEG | 8 |
| FOOT | 5 |
| HAND | 5 |

These are base values scaled by the droid's material grade.

## Sound system

### MIDI music

The DOS release includes 63 standard MIDI files (format 1) organized into 14
categories matching the music_categories table from the executable:

| Category | Variants | Use |
|----------|----------|-----|
| MAIN2 | 1 | Title screen |
| GENBASE | 1 | Generator base exploration |
| BATT | 11 | Combat encounters |
| SHOPKEEP | 1 | Shop interaction |
| HOLOMAP | 1 | Holamap screen |
| ESCAPED | 1 | Escape sequence |
| FINAL2 | 1 | Final mission |
| TRAPPED | 1 | Trapped state |
| FCBASE | 11 | FC base exploration |
| VCBASE | 11 | VC base exploration |
| LONGNT | 11 | Long night ambient |
| W | 11 | Walking/exploration |
| COMPROOM | 1 | Computer room |
| RUNNING | 1 | Running/chase |

Categories with multiple variants are selected randomly using the game PRNG.
All 63 files are identified by SHA-256 content hash. The MIDI player renders
through a software synthesizer at the start menu's selected output rate
(22,050, 44,100 or 48,000 Hz; 22,050 Hz remains the compatibility fallback).
The optional `--hq-midi` path applies a short output filter after synthesis;
it does not replace the verified Captive instrument patches.

### AdLib sound effects

Sound effects are generated via AdLib OPL2 FM synthesis, not stored as PCM
samples. The `CAP_A.BIN` file (5,426 bytes) is a loadable x86 driver containing:

1. **x86 driver code** (0x000–0x3BF): AdLib I/O port programming and SFX
   interpreter loop
2. **9 channel descriptors** (0x3C0–0x46F): OPL2 channel-to-register mapping
3. **Amplitude decay table** (0x470–0x4FF): 128 descending values for envelope
4. **OPL2 F-number table** (0x530–0x5CF): 128 frequency values for note pitch
5. **49 SFX sequences** (0x0A6A–0x138F): bytecode programs that write OPL2
   registers to produce sound effects, each terminated by `0xFF` + `pend`
6. **26 instrument patches** (0x1390–0x1532): 16 bytes each containing the
   11 OPL2 register values per voice (AM/VIB/EG/KSR/MULT, KSL/TL, AR/DR,
   SL/RR, waveform, feedback/connection)

The SFX sequences, instrument patches, and frequency table have been extracted
to `adlib_data.c` with a verification test suite.

#### SFX bytecode interpreter (13 opcodes)

The interpreter loop at offset 0x734 in CAP_A.BIN runs 4 voices simultaneously
at ~70 Hz (DOS timer tick). Voice state is a 0x1C-byte struct with PC, delay
counter, note offset, loop table pointer, subroutine return address, and PRNG.

| Opcode | Size | Action |
|--------|------|--------|
| 0x80 | 3 | Key on: note = operand1 + note_offset, delay = operand2 |
| 0x81 | 2 | Set delay counter |
| 0x82 | 2 | Set OPL2 volume register |
| 0x83 | 2 | Set note offset (added to all subsequent notes) |
| 0x84 | 2 | Load instrument patch by index |
| 0x85 | 2 | Set delay (variant, jumps to delay path) |
| 0x86 | 3 | Call subroutine (saves return address) |
| 0x87 | 1 | Return from subroutine |
| 0x88 | 2 | Key on with PRNG-generated note, delay = operand |
| 0x89 | 2 | Set delay to (PRNG & operand) + 1 |
| 0x8A | 3 | Jump to absolute address |
| 0xC8 | 1 | Key off |
| 0xFF | - | End/loop: decrement loop counter, advance loop table pointer |

Opcodes 0x88 and 0x89 use a per-voice xorshift PRNG for randomized pitch and
timing effects (explosion rumble, ambient noise). Opcode 0x86/0x87 implement
single-level subroutine calls for sequence reuse within SFX programs.

The CTV files (`SB15.CTV`, `SB20.CTV`, `SBPRO.CTV`) are CT-VOICE driver files
for Sound Blaster DSP, not Creative Voice (VOC) audio samples. They contain
executable code for DMA-based PCM playback but no sound data.

## Item database

Item records are stored in the unpacked CAPPO.EXE at offset 0x1a090. The first
item (HEAD) has a unique 16-byte prefix containing body part armor values for
all six material grades. Subsequent records follow the format:

    00 type_code [grade_byte] NAME 0x20

Type codes recovered from the binary:

| Code | Meaning | Examples |
|------|---------|----------|
| 0x00 | Misc / no class | DEV-SCAPE, KNUCLE-DUSTER, MINE, DIE, BALL, MAP |
| 0x08 | Consumable | BATTERY |
| 0x10 | Ammo | CARTRIDGES, SHELLS, LASER PACK, SONIC PACK |
| 0x20 | Chip | DROID CHIP |
| 0x21 | Equipment | OPTIC, CAMERA, BATTLE-GLOVE, WAR-BLADE, PISTOL |
| 0x27 | Body part (standard) | HEAD, CHEST, LEG, FOOT, HAND, GOLD |
| 0x30 | Ranged weapon | COLT, MAGNUM, RIFLE, all automatics/energy/heavy |
| 0x60 | Explosive | EXPLOSIVES |
| 0x65 | Body part (variant) | ARM |

Body parts carry a grade byte (always 0x05 in the base table). POISON, ACID,
and FLAMBOS use `04 27` as a terminator instead of 0x20, marking them as
special ammo sub-types (grenade-like items that deal body-part damage).

Ammo items (type 0x10) include caliber price entries: CARTRIDGES with prices
20/45/50 (using `%` and `&` delimiters for two price tiers), A51 MISSILES,
SHELLS, LASER PACK, SONIC PACK, and the three grenade types.

The weapon variant section at 0x1a220 contains 23 upgrade tier records in the
format `00 type_code 04 2c PRICE_STRING 2d` where the price string is ASCII
decimal. Suffix letters (A/B/C) distinguish same-price items. Gold cap is 200
(`[0x8D81]` capped at 0xC8 in bar renderer at 0x1AE7).

| Tier | Type | Price | Notes |
|------|------|-------|-------|
| 0 | 0x30 ranged | 1.9 | Base ranged |
| 1-5 | 0x21 melee | 2, 3, 5, 7, 14 | Melee progression |
| 6-19 | 0x30 ranged | 14, 27, 23, 33, 56, 78, 99, 111, 141, 165, 180, 200, 211, 231 | Ranged progression |
| 20-22 | 0x30 ranged | A12, L22, X42 | Named variant prefixes |

Item stats (damage, range, ammo capacity, price, weight) are **not** stored as
lookup tables. The original game computes them procedurally from the type code
and material grade. The stat computation formulas remain to be recovered from
the code section of the executable.

## PRNG

Captive uses multiple PRNG variants, all based on the same linear congruential
core (`state = state * 0x5E5 + 0x29`) but with different post-processing:

| Location | Rotation | XOR | Usage |
|----------|----------|-----|-------|
| 0x8A8E | ror 3 | xor ah,0x08 | Main game PRNG |
| 0x9815 | ror 2 | none | Combat randomness |
| 0x9FBA | ror 3 | and 0x0F (mask) | Name/map generation |
| 0xDAB6 | none | xchg al,ah | Map seed chaining (3 iterations) |

The main PRNG state is stored at `[0x9308]` (combat) and `[0x92F8]` (general).
All operate on 16-bit values.

## Combat system

Combat is processed at 0x8D66 in the unpacked CAPPO.EXE. The system iterates
over 4 droid slots (struct size 0x10E bytes, base 0x8DC7) from the droid array
at 0x5E38:

1. **Hit check** (0x97D9): Looks up attacker's target list in a creature table
   indexed by CH. Walks the list comparing each entry to AL (target ID).
   Increments hit counter (DL) on match.

2. **Damage calculation** (0x9BF4): `damage = lo_byte([di+6]) * hi_byte([di+6])`.
   The two bytes at offset 6 in the combat struct encode base damage and
   multiplier.

3. **Damage scaling** (0x9BFC): Shifts damage left up to 3 times (×2, ×4, ×8),
   checking for signed overflow at each step. Returns 0xFFFD (-3) as an
   overflow sentinel.

4. **Damage application** (0x8DAE): `[di+6]` damage value is added to the
   relevant accumulator for the attack record. This section describes the
   recovered attack-bytecode path; it does not identify the enemy creature
   record or prove that every creature uses the same fields.

### XP and level-up system

XP is a 32-bit accumulator at droid struct offset 0x22 (low word) and 0x24
(high word), capped at 0xF8FFFFFF.

**XP award** (0x9621): on creature kill, each living droid receives:
```
xp_base = creature_xp_value + 3 + min(difficulty, 100)
xp_gain = droid_skill_byte * xp_base * 12
```
where `droid_skill_byte` is `[di+0x0B]` in the droid struct.

**Display level** (0xB142): `level = xp >> 10`. So 1024 XP = level 1.

**XP threshold per skill** (0xAB92): each of 10 skills has a base XP value
from a table at DS:0xB708 (file 0x19EF8):

| Skill | Base | Growth | Max level |
|-------|------|--------|-----------|
| ROBOTICS | 10 | +12.5% + 8 per level | 66 |
| BRAWLING | 8 | +6.25% (rounded) | 24 |
| SWORDS | 30 | +6.25% (rounded) | 24 |
| HANDGUNS | 72 | +6.25% (rounded) | 24 |
| RIFLES | 216 | +6.25% (rounded) | 24 |
| AUTOMATICS | 648 | +6.25% (rounded) | 24 |
| LASERS | 1944 | +6.25% (rounded) | 24 |
| CANNONS | 3888 | +6.25% (rounded) | 24 |
| SPAYGUNS | 7776 | +6.25% (rounded) | 24 |
| EXPERIENCE | 7960 | +6.25% (rounded) | 24 |

All skills cap at threshold 0x8A47 (35,399) at their respective max level.

**XP overflow protection** (0x87EC): if high word ≥ 0xF8FF, no more XP is
added. The `or cl, 0x01` ensures at least 1 XP per award.

### Weapon damage tables

Weapon damage encoding recovered from file offset 0x1A006.

**Melee weapons** (18 entries, 2 bytes each: lo, hi):

| # | lo | hi | Damage (lo×hi) | Notes |
|---|----|----|----------------|-------|
| 0-3 | 0x04 | 0x21-0x3C | 132-240 | Starter weapon, 4 grades |
| 4-7 | 0x4C | 0x21-0x3C | 2508-4560 | Heavy melee, 4 grades |
| 8-12 | varies | 0x21-0x45 | 693-1311 | Medium melee, 5 grades |
| 13-17 | varies | 0x21-0x45 | 2970-6141 | Elite melee, 5 grades |

Grade progression uses hi bytes: 0x21(33), 0x2A(42), 0x33(51), 0x3C(60), 0x45(69)
— increments of 9.

**Ranged weapons** (20 entries, 4 bytes each: base, modifier, flag, 0x00):

5 tiers × 4 levels. Base values: 72, 88, 104, 120 (increment 16).
Modifier values: 45, 35, 25, 15, 5 (decrement 10 per tier).
Flag: 1 for upgradeable, 0 for max-tier or base tier.

The combat PRNG at 0x9812 uses the weaker ror-2 variant without XOR, making
combat sequences more predictable than general game randomness.

## Creature stat tables

Creature stats recovered from CAPPO.EXE DS:0xA1BF (file offset 0x189AF).
Each creature type has a 4-byte entry: `min_hp` (word), `max_hp` (word).
25 creature types total, grouped into 8 categories (3 types each, plus 1 extra).

**HP formula** (0x9B12):

```
base = min + ((max - min) * difficulty) / 8
hp = ((base * 2 * creature_modifier) >> 8) + 6
```

Where `difficulty` = dungeon level capped at 0-8, `creature_modifier` = byte
from creature spawn data at [di+9].

| Type | HP min | HP max | Cat | Speed | Sprite |
|------|--------|--------|-----|-------|--------|
| 1 | 10 | 150 | 0 | 0 | 0x67 |
| 2 | 200 | 400 | 0 | 5 | 0x67 |
| 3 | 400 | 600 | 0 | 7 | 0x67 |
| 4 | 600 | 800 | 1 | 10 | 0x67 |
| 5 | 800 | 1000 | 1 | 16 | 0x67 |
| 6 | 1000 | 1500 | 1 | 20 | 0x67 |
| 7 | 1000 | 1700 | 2 | 30 | 0x6A |
| 8 | 1700 | 2400 | 2 | 20 | 0x6A |
| 9 | 2400 | 3000 | 2 | 22 | 0x6A |
| 10 | 3000 | 3700 | 3 | 24 | 0x6A |
| 11 | 3700 | 4400 | 3 | 24 | 0x6A |
| 12 | 4000 | 4900 | 3 | 40 | 0x6A |
| 13 | 3000 | 3700 | 4 | 32 | 0x6A |
| 14 | 3700 | 4400 | 4 | 36 | 0x6A |
| 15 | 4400 | 5700 | 4 | 46 | 0x6A |
| 16 | 9000 | 10000 | 5 | 56 | 0x6B |
| 17 | 10000 | 12000 | 5 | 20 | 0x6B |
| 18 | 12000 | 15000 | 5 | 24 | 0x6B |
| 19 | 15000 | 17000 | 6 | 26 | 0x6C |
| 20 | 17000 | 19000 | 6 | 25 | 0x6B |
| 21 | 19000 | 22000 | 6 | 30 | 0x6C |
| 22 | 1000 | 3000 | 7 | 40 | 0x6D |
| 23 | 3000 | 5000 | 7 | 25 | 0x6D |
| 24 | 5000 | 7000 | 7 | 30 | 0x6E |
| 25 | 1000 | 1100 | 0 | 30 | 0x60 |

Category table at DS:0x9A43 (file 0x18233; the preceding byte at 0x18232 is
not a creature entry): groups creature types into 8 categories (0-7), 3 types
per category. Speed values at DS:0xA1A4 (file
0x18994). Sprite assignments at DS:0xA16E (file 0x1895E): graphic_id maps
to ALIEN PL5 sheets, and frame_index selects the animation variant in the
10x5 32x40 frame grid. The active viewport uses both fields.

DS segment base = 0x0E3F (file offset = 0xE7F0 + DS_offset).

## VGA palette

The 32-color VGA palette is set via INT handler at 0x9F2, loading from
DS=0x0E3F:0x7DE (file offset 0xEFCE). Each entry is 3 bytes of 6-bit VGA
values (0-63). The palette has been verified to match the existing
`pl5_default_palette` in the codebase. Notable entries:

- Colors 0-5: black to gray ramp (monochrome shading)
- Colors 6-7: green tones (vegetation, status)
- Colors 8-9: red tones (damage, alerts)
- Colors 10-15: yellow, pink, magenta, orange (effects, UI)
- Color 16: black (duplicate of 0, used as separate layer)
- Color 17: white
- Colors 19-23: brown ramp (wood, earth textures)
- Color 24: bright red
- Colors 27-29: blue ramp (sky, water, energy)
- Colors 30-31: green tones (different from 6-7)

## HUD panel layout

The following blit rectangles were recovered from the VGA copy routines in
CAPPO.EXE. Each is defined by a linear VGA offset (`mov di`), line count
(`mov bp`), and bytes-per-line (`mov cx` or `rep movsw` count):

| File offset | Screen (x,y) | Height | Description |
|-------------|--------------|--------|-------------|
| 0x485F | (32, 55) | 112 | 3D viewport (144×112, confirmed) |
| 0x4839 | (32, 170) | 28 | Message/text area below viewport |
| 0x0615 | (28, 3) | 32 | Top-left panel (droid status) |
| 0x0DCA | (220, 4) | 37 | Top-right panel |
| 0x3AD8 | (190, 15) | 30 | Right status panel |
| 0xD41D | (210, 157) | 56 | Bottom-right panel |
| 0xDC92 | (80, 120) | 49 | Center-bottom panel |
| 0xDD47 | (0, 119) | 99 | Left panel (inventory) |
| 0x1F48 | (293, 76) | 99 | Right edge panel |

The viewport blit at 0x485F copies 72 words (144 bytes) per line with a source
skip of 16 bytes and destination stride of 176 (320−144), confirming the
viewport fills exactly 144×112 pixels.

## Current runtime boundary

Captive's source-backed path currently accepts navigation-screen selection and
renders only authenticated CAPPO media or a caller-supplied DOSBox-X memory
image. It does not forward dungeon keys into the provisional `GameState`,
because doing so would change synthetic state behind an unchanged original
VGA frame. A live CAPPO input transport is therefore still required for
movement, rotation, interaction, inventory, terminal and save parity.

The F10 menu is not evidence of original-game parity. Its compatibility cheats
remain available only to isolated non-Captive/test paths; the source-backed
Captive path never applies them to generated state.

## Spawn placement algorithm

Recovered from CAPPO.EXE at 0x9987–0x9AA7. The spawn system places creatures
into dungeon cells based on creature type, party direction, and cell position.

### Entry point (0x9987)

1. Difficulty is read from the spawn record `[di+1]`, capped at 8
2. Category index selects creature type via `spawn_select_type()`
3. The recovered selector validates the candidate type against the difficulty
   offset and category data; the complete caller/selection contract is still
   under review. OpenCaptive's uniform three-entry category selection is
   therefore a compatibility behavior, not a claimed source-parity result.
4. HP is computed: `base = min + (range * difficulty / 8); hp = (base * 2 * modifier >> 8) + 6; cap 255`

### Type routing

| Type range | Placement | Count |
|-----------|-----------|-------|
| < 0x0A | Single `place()` | 1 |
| 0x0A–0x0B | Single `place_flagged()` (subcell OR 0x20) | 1 |
| 0x0C | Two `place_flagged()` with increment | 2 |
| 0x0D–0x0E | Three `place()` with increments | 3 |
| 0x0F | One `place()`; following calls derive its direction/subcell | 1 |
| 0x15 | `place()` + `place_flagged()` | 2 |
| other | Single `place()` | 1 |

### Subcell positioning (0x9A04)

Direction-based subcell lookup uses two 16-byte tables at DS:0x9BD8 and DS:0x9BE8:

- Direction 0–1: `table1[(direction << 2) + (position & 3)]`
- Direction 2–3: `table2[position]`

Table 1 (DS:0x9BD8): `00 06 08 02 02 00 06 08 06 08 02 00 08 02 00 06`
Table 2 (DS:0x9BE8): `01 03 07 05 00 01 03 04 01 02 04 05 03 04 06 07`

The placement writer at `0x53D1` does not convert this value into a second
map-cell coordinate. It packs the low direction bits and the selected
subcell into the creature record's byte at offset `+5`; the record is eight
bytes wide (`+0` cell pointer/index, `+2` source type, `+4` count/flags,
`+5` packed placement, `+6` HP/state word). The current compatibility runtime
stores only cell coordinates, so applying `subcell` as an `x/y` offset would
be incorrect. A future parity implementation must decode this packed record
at the viewport/compositor boundary.

### Direction modifiers (0x99E9, 0x99F2)

- Opposite: `direction XOR 2`
- Perpendicular: `NOT direction AND 1`

### Creature-category mapping (DS:0x9A42)

The verified CAPPO image contains a 25-byte mapping indexed by the runtime
creature candidate. It starts at file offset `0x18232` (`DS:0x9A42`) and is
not an 8×3 category-to-creature table:

`0D 00 00 00 01 01 01 02 02 02 03 03 03 04 04 04 05 05 05 06 06 06 07 07`

The preceding `0D` is part of the same indexed table. The later runtime
selection/retry path around `0x97B3` consumes this mapping together with the
spawn record and the difficulty-offset table. OpenCaptive's reverse grouped
`spawn_categories` array remains a compatibility representation until that
caller contract is recovered; it must not be described as the original table.

### Modifier table (DS:0x9AB7)

The verified v1.06 CAPPO image contains 25 contiguous modifier bytes, indexed
by the 1-based creature type used by the runtime (entry 0 is reserved):
`2, 4, 12, 30, 23, 60, 110, 90, 16, 18, 16, 12, 10, 12, 4, 0,
4, 8, 20, 8, 4, 2, 12, 4, 6`.
The first 16 values are the original table commonly transcribed in older
notes; the remaining nine values are part of the same source data region and
are required for creature types 16–24. The OpenCaptive table is hash-checked
against the unpacked CAPPO image (`fa7d5ca7…`).

### Difficulty offset table (DS:0x9A5A)

16 entries: `7, 0, 8, 16, 0, 8, 16, 0, 8, 16, 0, 8, 16, 0, 8, 16`

## Map generation (Architect)

Recovered from CAPPO.EXE at 0x3933–0x3FB3 and related functions.

### Architecture

The map buffer lives at segment 0x1BAA. Each cell is 5 bytes of wall/floor
bitmask flags. The map is 10 cells wide × 56 rows tall, with a row stride of
10×5 + 150 = 200 bytes. The buffer is initialized (zeroed) at 0x3EB4.

### PRNG

MapGen uses the DOS variant PRNG at 0x3D54: `state = state * 0x5E5 + 0x29`
(no ROR, no XOR). State stored at `[0x12DE]`. This is distinct from the main
game PRNG at 0x8E78 (which uses ROR 3 + XOR 0x800).

### Pattern generation (0x3F10)

Generates 32-bit bitmask patterns from PRNG output, rotated by varying amounts
(low nibble, high nibble of PRNG output). Creates 8 pattern rows stored at
segment 0x1BAA:0x4600+.

### Cell rendering (0x3D54)

Reads two pattern bytes per cell and applies wall connectivity via 8-bit
bitmask tests. Each of 8 bit positions controls a wall segment direction.

### Map types (0x399E dispatch)

4 cellular automaton rule sets selected by `cl`:

| cl | Address | Description |
|----|---------|-------------|
| 0 | 0x3AAA | Maze — wall connectivity propagation |
| 1 | 0x3B67 | Rooms — open area generation |
| 2 | 0x3C21 | Open — wide corridor layout |
| 3+ | 0x39CC | Mixed — combined wall/room rules |

### Templates

4 rectangular sub-region templates at DS:0x5CD6, 0x5CE4, 0x5CF2, 0x5D00
(14 bytes each). Applied after pattern generation via 0x3981. Each template
has a random style byte `(PRNG & 0xC0) + 0x0C`.

### Generator placement (0x1C3C–0x1D0F)

After map generation loop, generators are placed:
1. Count = `(PRNG & 7) + 1` (1–8 generators per level)
2. Position: random x (0–111, capped at 63), random width (1–8)
3. Marker byte `0x1A` written to map buffer cells
4. Level counter `[0x8D7F]` decremented per level

### Cell bit-to-wall mapping (viewport renderer at 0x4560)

Each 5-byte cell maps to an 8-pixel-wide column in the viewport. The renderer
writes to VGA offsets [di+3] through [di+7], with row stride 0xA0 (160 bytes).

| Byte | Bit | Wall segment |
|------|-----|-------------|
| 0 | 0x10 | Right wall (N/S) |
| 1 | 0x10 | Right-center |
| 2 | 0x08 | Center (door position) |
| 3 | 0x80 | Left-center (ornament) |
| 4 | 0x10 | Left wall |

CA rule output values control wall thickness:
- `0x10`: standard 1-pixel wall
- `0x18`: wide 2-pixel wall
- `0x80`: perpendicular cross wall
- `0xC0`: thick 3-pixel wall

### Feature pipeline (0x3309)

Post-mapgen feature placement calls:
1. 0x1D1C — position calculation
2. 0x2025 — processing stage
3. 0x33D7 — feature placement (called 2+ times with different parameters)
4. Loop over 8-byte structures at DS:0x5B82–0x5CB2 (6 entries)

## Liberation: BuildingGen (city generation)

### Executable format

BuildingGen is an Amiga HUNK executable on Liberation Disk 3:
- HUNK_CODE: 23,252 bytes (one hunk)
- HUNK_BSS: 3,956 bytes
- HUNK_RELOC32 for code→BSS fixups

### PRNG

Same formula as Captive MapGen:
```
state = (state * 0x5E5 + 0x29) & 0xFFFF
```
State stored at a5+0x20.

### Grid parameters (0x548–0x68A)

Computed from seed and level number:

| Parameter | Formula | Range |
|-----------|---------|-------|
| density | level × 5 + 15 + (PRNG_roll & 7) | 15–100 |
| num_columns | (level >> 4) + 6 + (seed_roll & 3) − 2 | 4–15 |
| num_roads | 2 + (seed_ror & 3) − (level >> 3); min 1; if level=0 then 5 | 1–5 |
| num_cross_roads | (seed_ror & 1) + (level >> 3) + 1 | 1–5 |

Derived: road_buildings = cross_roads/2, side_buildings = min(roads×2, 5),
buildings_per_segment = columns/2.

### Building record (36 bytes)

| Offset | Size | Field |
|--------|------|-------|
| 0 | 1 | Building type (0–8, PRNG % 9) |
| 1 | 1 | Sequential ID |
| 2 | 1 | Name seed |
| 3 | 1 | Flags: bits 0–1 = connection count, bits 2–5 = category<<2, bit 6 = dead end, bit 7 = disabled |
| 4–5 | 2 | Link word |
| 6–7 | 2 | Connection 0 marker (0xAAAA=forward, 0xBBBB=backward) |
| 10–11 | 2 | Connection 0 target building index |
| 12–19 | 8 | Connection 1 (same layout) |
| 20–27 | 8 | Connection 2 (same layout) |

### Building types

| Index | Type | Name source |
|-------|------|-------------|
| 0 | Shop | last_name + shop_type (9 types) |
| 1 | Bar | bar_type (12 types) |
| 2 | Business | last_name + business_type (11 types) |
| 3 | Industrial | industrial_type (12 types) |
| 4 | Residence | "Private Residence" |
| 5 | Library | "Library" |
| 6 | Police | "Police Station" |
| 7 | Records | "City Records Office" |
| 8 | Special | first_name + last_name |

### City names

German syllable pairs (32 syllables: gold, grun, braun, wein, eisen, …, platz)
concatenated, capitalized, followed by Greek letter suffix based on level
(Alpha, Beta, Gamma, Delta, Epsilon, Zeta, Eta, Theta, Kappa).

### String tables in BuildingGen binary

| Offset | Content |
|--------|---------|
| 0x2EE8 | Newspaper names ($-delimited, 16 entries) |
| 0x2F6F | TV station names ($-delimited, 9 entries) |
| 0x2FD7 | NPC titles ($-delimited, 8 entries) |
| 0x302F | Greek letter suffixes (null-terminated, 9 entries) |
| 0x1A09 | Character set (62 chars + $) |
| 0x288A | Building type names (Library, Police Station, Private Residence, City Records Office) |

## Liberation: CityGen (64×64 grid generation)

CityGen is a separate Amiga HUNK executable on Liberation Disk 3:

- **File**: CityGen (10,896 bytes file, 10,824 bytes code, 4,888 bytes BSS)
- **Version**: "CityGen 1.12 (CaptiveII : Monday 03-Jan-94 02:17:04)"
- **PRNG**: `state = state * 0x5E5 + 0x29` (identical to BuildingGen and Captive MapGen)

### Grid structure

The city is a 64×64 grid with 3 planes (12,288 bytes total):

| Plane | Purpose |
|-------|---------|
| 0 | Cell type (wall=0x00, road=0x0D, border=0xFF, building types) |
| 1 | Road/feature ID |
| 2 | Building ID (bit 7 = origin marker) |

### Meta-grid (8×8)

Before the 64×64 grid is generated, an 8×8 meta-grid is constructed. Each meta-cell stores a 4-bit direction bitmask:

| Bit | Direction | dx | dy | Grid offset |
|-----|-----------|----|----|-------------|
| 0 | North | 0 | -1 | -64 |
| 1 | East | +1 | 0 | +1 |
| 2 | South | 0 | +1 | +64 |
| 3 | West | -1 | 0 | -1 |

### Road corners

Four entry points connect roads from the grid edges:

| Direction | Corner (x,y) |
|-----------|-------------|
| North | (3, 0) |
| East | (6, 3) |
| South | (3, 6) |
| West | (0, 3) |

Road availability is determined by `seed_lo` via a 36-byte lookup table at 0x2694.

### Generation phases

The generation is gated by difficulty level (0x2C8 initializes to 127):

| Phase | Difficulty | Subroutine | Description |
|-------|-----------|------------|-------------|
| Count roads | always | 0x1B40 | Count available road directions from seed |
| Generate roads | always | 0x1CE8 | Walk roads on meta-grid with PRNG-biased turns |
| Extra connections | always | 0x1B6C | Connect isolated road segments |
| Expand to grid | always | 0x1DE4 | 8×8 meta → 64×64 using 4×4 tile templates |
| Set borders | ≥ 0 | 0x1BE8 | Mark edges as walls (0xFF), save to plane1 |
| Place features | ≥ 1 | 0x1F1A | Feature placement with retry |
| Place feature blocks | ≥ 2 | 0x2E4 | Template B blocks (7 cells) near roads |
| Place road blocks | ≥ 3 | 0x330 | Template A blocks (6 cells) near roads |
| Building shapes | ≥ 3 | 0x7D2+ | Various building/structure subroutines |
| Advanced features | ≥ 4 | 0xA80+ | Higher-difficulty features |
| Finalize | always | 0x24B8 | Grid post-processing |

### Block templates

Two template sets are used for building placement:

**Template A** (0x28B4): 4 rotations × 6 cells + 2 adjacency checks = 16 bytes per entry.
Represents a 2×3 building footprint.

**Template B** (0x28F8): 4 rotations × 7 cells + 2 adjacency checks = 20 bytes per entry.
Represents a 3×3 building footprint.

Placement requires all template cells to be empty (0x00) and at least one adjacency cell
to contain a road-type value (types 18-21 after masking with 0x3F).

### Tile templates (0x2958)

13 tile templates of 4×4 bytes each control how meta-grid cells expand to the 64×64 grid.
The template index is derived from the meta-cell's direction bitmask value.

### Data tables

| Offset | Size | Description |
|--------|------|-------------|
| 0x2694 | 36 bytes | Road availability per seed (4 bytes × 9 entries) |
| 0x26B8 | 8 bytes | Road corner positions (x,y pairs × 4) |
| 0x26C0 | 16 bytes | Road direction deltas (dx,dy words × 4) |
| 0x2830 | 16 bytes | Grid direction table (dx,dy,offset × 4) |
| 0x2890 | 9 bytes | Road count per seed level |
| 0x2899 | 9 bytes | Block count per seed level |
| 0x28B4 | 68 bytes | Block template A (4 × 16 bytes + header) |
| 0x28F8 | 84 bytes | Block template B (4 × 20 bytes + header) |
| 0x294C | 238 bytes | BLOC structure (tile/building data) |
| 0x2958 | 208 bytes | 13 tile templates (4×4 bytes each) |
| 0x2478 | 18 bytes | PRNG subroutine |
