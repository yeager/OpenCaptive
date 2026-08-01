# OpenCaptive Wiki

OpenCaptive is a C/SDL3 reimplementation of **Captive** (1990) and **Liberation: Captive 2** (1993) by Tony Crowther, originally published by Mindscape.

The project targets source-level accuracy against the original DOS, Amiga, Atari ST and CD32 releases. No original game data is distributed; users supply their own files, verified by SHA-256 content identity.

## For players

- [[User Guide]] — Building, running, CLI flags, graphics settings, controls
- [[Data Identity and Verification]] — How game data files are identified and verified
- [[Game Preservation]] — Platform variants, media formats, and preservation notes

## For developers

- [[Developer Guide]] — Code layout, build/test contract, contribution workflow
- [[File Formats]] — PL5, ANM, RNC, ADF, ISO9660, x3g, VGM, 8SVX, MIDI

## Game reference

- [[Captive Technical]] — DOS engine analysis, viewport, map generation, runtime
- [[Captive Game Data]] — Items, droid materials, skills, music system, name generation
- [[Liberation Technical]] — CD32 engine analysis, CityGen, sprite banks, ANIM
- [[Liberation Game Data]] — Procedural cities, shops, bars, NPCs, 3D vector data

## Scope and provenance

Original media remains copyrighted and must be supplied by the player. The
project verifies content hashes before loading assets. Do not add original game
assets to issues, pull requests, release artifacts or this wiki.

## Project links

- **Repository**: [github.com/yeager/OpenCaptive](https://github.com/yeager/OpenCaptive)
- **Releases**: [GitHub Releases](https://github.com/yeager/OpenCaptive/releases) — Linux deb/rpm/AppImage, macOS DMG, Windows installer
- **Author**: Daniel Nylander
- **License**: MIT
- **External reference**: [The Ultimate Captive Guide](https://captive.atari.org/Technical/MapGen/Introduction.php)
