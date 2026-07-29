# OpenCaptive Wiki

OpenCaptive is a clean-room C/SDL3 reimplementation of **Captive** and
**Liberation: Captive II**. It contains no original game data.

This documentation separates observed file-format facts from reimplemented
gameplay behaviour. A claim marked **implemented** is backed by source and
tests in this repository. A claim marked **under analysis** is an observation
from original media and is not a promise of full game parity.

## For players

- [User guide](User-Guide) — build, launch, controls, saves and diagnostics.
- [Data identity and verification](Data-Identity-and-Verification) — why data
  is selected by SHA-256 content hashes, not archive or file names.

## For developers

- [Captive technical notes](Captive-Technical) — graphics, animation,
  compression, disk formats and Architect map generation.
- [Liberation technical notes](Liberation-Technical) — CD32 track layout,
  ISO9660 reading and verified resource inventory.
- [Developer guide](Developer-Guide) — code architecture, tests and safe
  reverse-engineering workflow.

## Scope and provenance

Original media remains copyrighted and must be supplied by the player. The
project verifies content hashes before loading assets. Do not add original game
assets to issues, pull requests, release artifacts or this wiki.

The main external technical reference for Captive is the
[Ultimate Captive Guide](https://captive.atari.org/Technical/MapGen/Introduction.php).
