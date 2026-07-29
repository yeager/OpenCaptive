# Captive technical notes

## Runtime model

Captive's recovered presentation currently consists of the original 320×200
HUD shell and the documented visibility rules. Legacy `DungeonLevel` and
gameplay structures remain in the source tree as reverse-engineering notes,
but are not driven by the runtime: they were generated substitutes, not decoded
original map or save state.

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
for checking the PL5 decoder. They do **not** yet identify the first-person
projection tables, draw ordering, wall/door state encoding or creature
placement. OpenCaptive therefore keeps the default dungeon viewport unpainted
until those parts can be reproduced from original behaviour rather than an
invented perspective approximation.

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

The technical reference used for this boundary is the documented
[MapGen introduction](https://captive.atari.org/Technical/MapGen/Introduction.php)
and its linked stage pages. New MapGen work must add an original-output fixture
or an independently reproducible reference before claiming parity.

## Current runtime boundary

The former generated map, combat, mission, save and cheat implementation is
not active in the runtime. Although it used plausible game rules, its state
was not decoded from hash-verified Captive media and could not establish visual
or logical parity with the original. The runtime currently displays verified
intro/HUD data and keeps the viewport untouched until the original panel
compositor and state format are recovered.

The F10 menu is limited to display and audio options. Its game-affecting cheat
commands remain visibly pending instead of mutating a generated substitute
state.
