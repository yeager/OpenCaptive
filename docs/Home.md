# OpenCaptive Wiki

**Current version: v1.1.91**

OpenCaptive is a C/SDL3 reimplementation of **Captive** (1990) and **Liberation: Captive 2** (1993) by Tony Crowther, originally published by Mindscape.

OpenCaptive is an actively developed, source-faithful reimplementation.
Captive has a playable DOS-based compatibility runtime with verified data,
combat, audio, map-generation, save/load, and presentation slices. Liberation
has verified CD32 data, presentation, format, and city-system slices, while
its full campaign and original gameplay parity remain active work. No original
game data is distributed: assets are loaded from the player's own media and
verified by SHA-256 content identity.

## For players

- [[User Guide]] — Building, running, CLI flags, graphics settings, controls,
  xBRZ/widescreen presentation and the live F10 popup
- [[Start Menu]] — 8-item start menu layout, game cards, data scanner
- [[Save and Load]] — Save slots, quicksave, quickload
- [[Internationalization]] — 19 supported languages
- [[Mission Flow]] — Game progression, objectives, mission structure
- [[Energy and Body Parts]] — Droid energy system, body part mechanics
- [[Data Identity and Verification]] — How game data files are identified and verified
- [[Game Preservation]] — Platform variants, media formats, and preservation notes

## For developers

- [[Developer Guide]] — Code layout, build/test contract, contribution workflow
- [[File Formats]] — PL5, ANM, RNC, ADF, ISO9660, ArcD, CTV, AmSp, x3g, VGM, Img, FNT, 8SVX, MIDI

## Game reference

- [[Captive Technical]] — DOS engine analysis, viewport, map generation, runtime
- [[Captive Game Data]] — Items, droid materials, skills, music system, name generation
- [[Liberation Technical]] — CD32 engine analysis, CityGen, sprite banks, ANIM, asset format decoding
- [[Liberation Game Data]] — Procedural cities, shops, bars, NPCs, 3D vectors, sprites, fonts

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
