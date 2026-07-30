# Audio System

## Overview

OpenCaptive has three audio subsystems: sound effects (SFX), 8SVX sample playback, and MIDI music synthesis.

## Sound Effects (SFX)

Sound effects are currently silent. Earlier builds generated placeholder tones
and noise at startup; that code is deliberately not used because it cannot be
compared with the original effects. The media must be identified by SHA-256,
then its effect format and playback semantics must be decoded before samples
are enabled.

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

Music is identified solely by the complete SHA-256 digest of its bytes. The
runtime searches the configured data source and does not depend on an original
path or filename.

| Track | SHA-256 | Usage |
|-------|---------|-------|
| MUSIC_TITLE | `ef9ec6b8fac6710c99f9ed037dfdb2767a3e20bc9259789ced977d322d3420be` | Title screen |
| MUSIC_BASE | `150db09bf3ba0914501b9be1353c21916a597e89ef711f7b2dff4989313f2810` | Base/exploration |
| MUSIC_BATTLE | `a4b8a8fdef37602e732af3435b0ade18219de456e5563e0229e0d9f1c5132e6f` | Combat |
| MUSIC_SHOP | `dfe6b899e1e499d3c9a326c4554c8da3e7e83d395b1d13c81341d2688c50a0bc` | Shop interface |
| MUSIC_HOLOMAP | `7be1dc97c004fc04f1ead07a2957c6e2a1b97ddf48dafa998964f74db816757f` | Holographic map |
| MUSIC_ESCAPE | `2ddacc25ece9e3e6bdd13e3f1e7f926bfce2e180daf0fce8d57a3beb5ec30d1a` | Victory/escape |
| MUSIC_FINAL | `d8cb990243dcdb885881f09a0bbe0788caee3c7b6ec9f62ed5d70c4c1d411587` | Final mission |
| MUSIC_TRAPPED | `8c1ad7905a95dacb8a57d9f34d97beeb8eb42a4f38a2188909d6c769ebdbab2d` | Game over |
