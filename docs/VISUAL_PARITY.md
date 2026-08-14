# Visual parity

> Updated for v1.1.126. This document separates reproducible original frames from prototype graphics.

Parity checks may use only decoded original resources or captures produced by
that decoding path. The former CI snapshot of a hand-built Captive corridor
has been removed: a stable hash of synthetic pixels is not a parity check.
Because original media is not stored in the source tree, the complete visual
check runs locally against the user's verified media.

Liberation also has two directly verified original regions. The first planar
image from the hash-identified CD32 resource is decoded separately and then
compared pixel by pixel with the corresponding rectangle in OpenCaptive's
native capture:

| Presentation | FORM/ANIM SHA-256 | Region | Differing pixels |
| --- | --- | --- | --- |
| Intro | `e7e35f1b491fafd95da260abcb1c1c402140601840c5f98ae7282069fd30b269` | 320×162 at `(0,47)` | 0 of 51,840 |
| City | `b94a450c12428af9a22b8bb8c31fca74cdc2b2bd3be3dc9c7a1eadd7e6576101` | 320×167 at `(0,44)` | 0 of 53,440 |

The directly decoded PPM images have SHA-256 values
`c65df735ccd785dee5cbe118c3f51153f270f6260411d3e04c6ca278b2d6fab3` and
`b7c326d1cdd36bb3574b33add3d68cff9739e7a5e339d800f44af3c79f510bb1`.
This proves only those first presentation frames, not the complete Liberation
view, its animated layers, or gameplay state.

The original mission menu is also decoded directly from the hash-verified AMOS
bank `d6bb0dd9c578beb8e84ddf9f458f0be43ec158b2b261491d023e972d2812c2d2`.
The bank's only sprite is a five-plane composition measuring 320×109 pixels.
OpenCaptive displays it between the intro and the current city reference, and
the original `Game on...` rectangle is clickable. This verifies the menu
image, not the later city simulation or UI state.

Run the check locally with:

```sh
./build/opencaptive --verify-data all --data /path/to/media
```

For Liberation, verification also includes the PPM hashes for the two decoded
first frames above. It confirms that the original pixels are decoded without
modification; it says nothing about later animation, gameplay state, or
interaction.

When making an intentional visual change, inspect the new image first. Update
only the affected reference hash and document the visible change in the same
change.

## Reproducible game captures

`--capture-frame <ppm>` starts the selected game, writes the first complete
internal framebuffer at the game's original resolution, and exits. Capture
happens before window scaling, system colour management, and external overlays.

Captive headless capture also starts in the verified holomap/navigation view,
the same state as normal interactive startup. It does not jump directly to a
landed dungeon checkpoint or fill the viewport with a generated mission.

```sh
./build/opencaptive --game captive --data /path/to/media \
  --capture-frame /tmp/captive.ppm
./build/opencaptive --game liberation --data /path/to/media \
  --capture-frame /tmp/liberation.ppm
./build/opencaptive --game liberation --skip-intro --data /path/to/media \
  --capture-frame /tmp/liberation-city.ppm
```

The Captive capture can be compared with the hash-identified HUD resource
outside the game view, but this is not a complete live-game comparison. A real
game frame also changes parts of the monitors, controls, and status area. The
complete DOS panel compositor and live cell-based dungeon rendering are not
restored yet; the verified landing image is therefore an explicitly
authenticated checkpoint, not a claim of full pixel parity.

A DOSBox dump from an owned original can be turned into a reproducible
reference without adding game media to the source tree. The tool accepts
exactly the 1 MiB output created by `MEMDUMPBIN 0 0 100000`, writes the dump's
SHA-256, and extracts VGA memory `0xA0000..0xAFA00` as a 320×200 PPM image:

```sh
./build/opencaptive --extract-dos-vga /path/to/MEMDUMP.BIN /tmp/captive-original.ppm
./build/opencaptive --compare-frames /tmp/captive-original.ppm /tmp/captive.ppm
```

`captive_descriptor_match` is a narrower analysis gate for the documented DOS
renderer. It accepts only the known hash-verified 1 MiB dump and an image
exactly extracted from that dump. It therefore does not identify game data by
filename and cannot accidentally compare a frame from another game moment:

```sh
./build/opencaptive --extract-dos-vga /path/to/MEMDUMP.BIN /tmp/captive-original.ppm
./build/captive_descriptor_match /path/to/MEMDUMP.BIN /tmp/captive-original.ppm
```

The check currently confirms two static panels in the Captive view. It does
not prove that dynamic dungeon rendering has been restored.

For areas with their own parity gate, the same exact comparison can be made on
a rectangle. It returns zero only when every pixel in the rectangle is
identical and two for an invalid rectangle:

```sh
./build/opencaptive --compare-frames-rect \
  /tmp/captive-original.ppm /tmp/captive.ppm 32 55 144 112
```

When the original reference is a standalone FORM image while the game capture
has a larger internal surface, use separate source coordinates without making
a padding image. The city FORM above is checked as follows:

```sh
./build/opencaptive --compare-frames-regions \
  /tmp/liberation-city-form.ppm /tmp/liberation-city.ppm \
  0 0 0 44 320 167
```

The command requires equal-sized regions and returns zero only when all RGB
pixels are identical. This allows HUD, viewport, and later Liberation layers
to be measured independently without hiding an unresolved area behind another.

The known reference with SHA-256
`9003c4a8818cb97f8299ac90cfe51e90e535ab9a725545526fe75f14ddb8dd7e` shows a
genuine Captive outdoor view. OpenCaptive's current Captive capture differs in
10,558 of 16,128 viewport pixels and 8,376 of 47,872 pixels outside it. These
are active error measurements, not an accepted parity result.
`--compare-frames` returns zero only when all pixels are identical and one when
any pixel differs, making it suitable for a local or future CI check after the
original images have been reviewed.

`captive_panel_match` uses all hash-identified Captive PL5 source surfaces,
including walls, exterior and interior variants, animations, doors, ceilings,
objects, and UI workspaces, together with a 320×200 reference image. It scans
for exact 8×8 regions from viewport `(32,55,144,112)` without relying on
filenames and reports target/source coordinates with their SHA-256 values.
Against reference
`9003c4a8818cb97f8299ac90cfe51e90e535ab9a725545526fe75f14ddb8dd7e`, 108 of
252 tiles are found directly. The remaining tiles are overlapped or
composited, as expected for the original panel blits, and identify where
renderer recovery must continue.

```sh
./build/captive_panel_match /path/to/media /tmp/captive-original.ppm
```

## Captive panel sheet

Fed7-E from the verified Amiga ADF is decoded through RNC1-old into five
bitplanes and converted to 64,000 palette indices. The index buffer has SHA-256
`d2efa8a9cbbbaa45e49c82465765836ba173676645e810fda5cd23ef85bd3431` and is
identical to the corresponding hash-identified DOS panel sheet: 64,000 of
64,000 pixels match. This proves that both panel sources are original data. It
does not yet prove the dynamic cell and panel composition in the game view.
