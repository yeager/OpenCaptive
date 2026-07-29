# Captive technical notes

## Runtime model

Captive is implemented as a grid-based first-person crawler. The engine keeps
logical `DungeonLevel` arrays at 64×32 cells. A cell has a type, per-face wall
textures, floor/ceiling textures, ornaments and small object fields. The
viewport renders the current level while HUD, inventory, terminal, shop and
puzzle systems operate on the same `GameState`.

## PL5 graphics

A PL5 image is 40,000 bytes: 320×200 pixels at five bits per pixel. Every five
input bytes encode eight output pixels using a bespoke bit layout, not a normal
linear bitstream. `pl5_decode()` expands it to indexed pixels; palette values
are converted to ARGB for renderer textures.

The decoder has a dedicated regression test. Bounds checks reject a truncated
payload instead of reading beyond it.

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

The original map model is one flattened 64×32 allocation divided into sixteen
16×8 physical sections. Logical floors are assigned across those sections; the
modern engine exposes them as a `levels[]` array while retaining section-based
layout.

The recovered original MapGen PRNG has a 16-bit state:

```text
state = (state × 1509 + 41) mod 65536
result = ror16(state, 4) XOR 0x0800
```

Mission/base seed selection is:

```text
seed = ((mission - 1) × 11) + base
```

Early maps use restricted section masks. The generator reserves a root section
on row zero, grows contiguous floor assignments, carves bounded paths, inserts
paired stair transitions, then places generators, shops and up to eight
choke-point doors. Tests cover seeds 0–127 for connected floors, valid stairs,
root access and exactly one generator per logical floor.

The original generator contains more features than the current reimplementation
(for example raisers, push walls and specialised door families). Treat
section/PRNG parity as implemented, and remaining feature-level parity as
under analysis.

## Campaign and combat

The mission objective is generator destruction, not elapsed time or creature
count. Completing every generator advances to the next mission; mission ten
sets victory. Combat uses grid line-of-sight: walls and closed doors block both
droid and enemy attacks. Normal doors require interaction to change to floor.
