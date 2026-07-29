# Liberation: Captive 2 Data Formats

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

## Identified data patterns

- IFF FORM chunks for animations and graphics
- RNC Method 1 compressed game data
- Standard Amiga 8SVX audio samples within IFF containers
