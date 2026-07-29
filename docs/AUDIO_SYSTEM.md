# Audio System

## Overview

OpenCaptive has three audio subsystems: sound effects (SFX), 8SVX sample playback, and MIDI music synthesis.

## Sound Effects (SFX)

Procedurally generated at startup using sine waves, noise, and frequency sweeps:

| Effect | Type | Parameters |
|--------|------|-----------|
| HIT | Noise burst | 150ms, fast decay |
| SHOOT | Frequency sweep | 800→200 Hz, 200ms |
| DOOR_OPEN | Frequency sweep | 100→400 Hz, 300ms |
| DOOR_LOCKED | Tone | 150 Hz, 200ms |
| STEP | Noise burst | 80ms, very fast decay |
| BUTTON | Tone | 1200 Hz, 100ms |
| PICKUP | Frequency sweep | 400→1200 Hz, 150ms |
| DEATH | Frequency sweep | 600→50 Hz, 500ms |
| LEVEL_UP | Frequency sweep | 300→900 Hz, 400ms |
| GENERATOR | Noise | 400ms, moderate decay |

All generated at 22050 Hz sample rate.

## 8SVX Sample Loader

Parses IFF 8SVX format (Amiga standard):
- FORM/8SVX container
- VHDR chunk: sample rate at offset 12
- BODY chunk: raw signed 8-bit PCM data

## 8-Channel Mixer

- 8 simultaneous channels
- Per-channel: volume, pitch (playback rate ratio), looping
- Channel stealing: oldest channel reused when all busy
- Mixing: signed 16-bit with clamping, output to SDL3 audio stream

## MIDI Music

Software synthesizer:
- Standard MIDI file parser (MThd/MTrk)
- Variable-length quantity (VLQ) delta time decoding
- Running status support
- 32-voice polyphony using square wave synthesis
- Per-voice: frequency, velocity, ADSR envelope
- Tempo tracking (microseconds per quarter note)
- Looping support for continuous playback

### Music Tracks

| Track | File | Usage |
|-------|------|-------|
| MUSIC_TITLE | MAIN2.MID | Title screen |
| MUSIC_BASE | GENBASE.MID | Base/exploration |
| MUSIC_BATTLE | BATT1.MID | Combat |
| MUSIC_SHOP | SHOPKEEP.MID | Shop interface |
| MUSIC_HOLOMAP | HOLOMAP.MID | Holographic map |
| MUSIC_ESCAPE | ESCAPED.MID | Victory/escape |
| MUSIC_FINAL | FINAL2.MID | Final mission |
| MUSIC_TRAPPED | TRAPPED.MID | Game over |

Music files loaded from `<data_path>/SOUND/` directory.
