# Liberation: Captive 2 Data Formats

> Updated for v1.1.82. Data remains player-supplied and hash-verified.

## Amiga floppy version

- 5 ADF disks (901,120 bytes each = standard 880KB Amiga floppy)
- OFS (Old File System) — boot block `DOS\0` with flags byte 0x00
- IFF FORM/ANIM chunks (Amiga ANIM opt5 format) — at least one named `OutTube`
- RNC (Rob Northen Compression) signatures present (2-71 hits per disk)
- Heaviest compression on disk 5

## Amiga CD32 version

- 1 data track (MODE1/2352) + 10 CD audio tracks
- ISO9660 filesystem with CDTV extension
- Volume ID: `Liberation_1`
- Publisher: `Mindscape`
- Application ID: `Liberation CD32`
- Created: 1994-04-15
- Mastering tool: `ISOCD 1.04 by Pantaray, Inc.`
- CD audio tracks 2-11 are the game soundtrack by Mark Knight

## Data track structure

The data track uses standard ISO9660 with CDTV extended attributes. Primary Volume Descriptor at sector 16 contains `CD001` magic followed by `CDTV` extension tag.

## Audio tracks

10 Red Book CD audio tracks providing the full in-game soundtrack. Can be extracted and played as standard WAV/FLAC.

For FS-UAE or another CD32 emulator, mount all eleven tracks. Mounting just
the data track can leave the original introduction waiting for the missing CD
audio. `liberation_cd32_cue` writes a CUE and its sibling tracks using only
their SHA-256 identities:

```sh
mkdir -p /tmp/liberation-cd32
./build/liberation_cd32_cue /path/to/media /tmp/liberation-cd32
```

Mount `/tmp/liberation-cd32/liberation-cd32.cue`. The output names are SHA-256
digests; source archive member names are never used as game-data identity.

## Identified data patterns

- IFF FORM chunks for animations and graphics
- RNC Method 1 compressed game data
- Standard Amiga 8SVX audio samples within IFF containers

## Mission selection composition

The original mission-selection background, difficulty/strategy controls and
start button are the first sprite in the hash-identified AMOS bank
`d6bb0dd9c578beb8e84ddf9f458f0be43ec158b2b261491d023e972d2812c2d2`.
The bank contains one five-plane, 320×109 sprite. Its pixels directly match
the mission-selection scene observed from the verified CD32 original. The
runtime manifest verifies this bank by SHA-256; it never identifies the asset
through an ISO or archive filename.

## O3DG city objects

The CD32 data track also contains `FORM O3DG` containers. They are not ILBM
bitmaps: the outer form starts with an `OFFS` block followed by nested `FORM
VCDO` objects. Each inspected VCDO object contains `EXVL` and `PLST` blocks.
For example, the hash-identified 24,702-byte resource
`cd6ffc66eb2d535adc9124e7aa74013a704ba1fe9f2090e33c0b2d2a6fe19dd0`
contains 34 such objects. This makes O3DG the current evidence-backed source
format for the dynamic city geometry seen during a Liberation mission.

`liberation_form_inventory` walks the verified ISO recursively and reports
only SHA-256 identities, IFF form types, chunk layout and any standard ILBM
bitmap headers. It is an analysis tool: EXVL and PLST semantics still need a
renderer-level implementation before OpenCaptive can reproduce the original
city view.
