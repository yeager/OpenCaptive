# OpenCaptive Wiki

**Current version: v1.1.126**

OpenCaptive is a C/SDL3 reimplementation of **Captive** (1990) and **Liberation: Captive 2** (1993) by Tony Crowther, originally published by Mindscape.

OpenCaptive is an actively developed, source-faithful reimplementation.
Captive's original DOS startup, VGA presentation and Mission 0001 route
selection are verified; arrival, orbit, landing and dungeon entry remain
explicitly gated on original runtime evidence. Liberation's verified slices and
remaining campaign boundary are documented separately. No original game data
is distributed: assets are loaded from the player's own media and verified by
SHA-256 content identity.

## Install by platform

- [[Install Linux]] — deb, rpm, AppImage, tarball, building from source
- [[Install macOS]] — DMG, Gatekeeper bypass, building from source
- [[Install Windows]] — Installer, SmartScreen, portable use
- [[Install iOS]] — Sideloading via AltStore, SideStore, Sideloadly, Xcode
- [[Install Android]] — APK sideloading, ADB, Obtainium auto-updates

## For players

- [[User Guide]] — Building, running, CLI flags, graphics settings, controls
- [[Controls]] — Complete keyboard and mouse reference
- [[CLI Reference]] — All command-line options with examples
- [[Custom Features]] — Optional enhancements: HD upscale, widescreen, minimap, lighting, replay
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
- [[Captive DOSBox-X Startup]] — repeatable authentic-data emulator startup and verification
- [[Captive Game Data]] — Items, droid materials, skills, music system, name generation
- [[Liberation Technical]] — CD32 engine analysis, CityGen, sprite banks, ANIM, asset format decoding
- [[Liberation Game Data]] — Procedural cities, shops, bars, NPCs, 3D vectors, sprites, fonts

## Reverse engineering reference

- [[Captive Disassembly]] — Complete CAPPO.EXE v1.06 reverse engineering: PRNG, combat formulas, XP system, item database, map generation, viewport renderer, OPL2 audio, spawn system
- [[Liberation Disassembly]] — Complete CD32 binary reverse engineering: city/plot generation, X3G/VGM/IMG/FNT/ANIM formats, 3D renderer, text expansion engine, NPC/shop/combat systems

## Scope and provenance

Original media remains copyrighted and must be supplied by the player. The
project verifies content hashes before loading assets. Do not add original game
assets to issues, pull requests, release artifacts or this wiki.

## Project links

- **Repository**: [github.com/yeager/OpenCaptive](https://github.com/yeager/OpenCaptive)
- **Releases**: [GitHub Releases](https://github.com/yeager/OpenCaptive/releases) — Linux deb/rpm/AppImage, macOS DMG, Windows installer, Android APK, iOS IPA
- **Author**: Daniel Nylander
- **License**: MIT
- **External reference**: [The Ultimate Captive Guide](https://captive.atari.org/Technical/MapGen/Introduction.php)

## Verification status

The current Captive verification path starts the player's original DOS files
through CAPTIVE.BAT 1 in a normal DOSBox-X window. The real intro, Mission 0001 holomap,
keypad input, target selection and FLIGHT PATH SET response are verified.
ARRIVED AT DESTINATION, NOW IN ORBIT, LANDING SUCCESSFUL and the first-person
dungeon transition remain gated until an original runtime capture proves them.
The project does not replace those states with generated game data. Debugger
and FIFO sessions are reserved for disassembly and frame inspection, not
interactive play.

No synthetic planet, landing point, dungeon, status text, or fallback gameplay
is accepted as parity evidence. The original runtime must visibly change state
after ORBIT, reach the destination orbit, show the real white landing marker,
and produce the original landed view after LAND.
