# Audio System

> Updated for v1.1.101. The release includes the SDL mixer, OPL2/MIDI paths, and optional reverb configuration covered by the audio tests.

## Overview

OpenCaptive has five audio subsystems: sound effects (SFX), 8SVX sample
playback, MIDI music synthesis, CD audio (CDDA) playback, and speech/voice
samples.

The start-menu audio setting offers 22,050, 44,100 and 48,000 Hz. The selected
rate is applied when the audio device is created, and is shared by SFX mixing
and MIDI timing. Changing it in the start menu takes effect on the next
session start; F10 remains reserved for runtime display, audio toggles and
cheats.

## Sound Effects (SFX)

Captive's AdLib SFX bytecode is decoded and rendered through the software OPL2
emulator. The runtime includes the recovered 49 sequence table, four active
voices, bounded opcode execution, key-on/key-off handling, patch loading and
per-tick timing. `sfx_play()` starts a mapped sequence and `sfx_update()` mixes
its output into the SDL audio stream.

The original SFX data is embedded only after its provenance has been recorded
in the source comments; no user-supplied game-data scan is required for this
subsystem. The implementation is not claimed to be cycle-exact OPL2 hardware,
but the test suite verifies that every sequence starts and that representative
sequences produce non-zero audio.

## 8SVX Sample Loader

Parses IFF 8SVX format (Amiga standard):
- FORM/8SVX container
- VHDR chunk: sample rate at offset 12
- BODY chunk: raw signed 8-bit PCM data

## 8-Channel Mixer

- 8 simultaneous channels
- Per-channel: volume, pitch (playback rate ratio), looping
- Channel stealing: oldest channel reused when all busy
- Mixing: signed 16-bit with clamping, output to the selected SDL3 sample rate

## MIDI Music

Software synthesizer (at the selected output sample rate):
- Standard MIDI file parser (MThd/MTrk)
- Variable-length quantity (VLQ) delta time decoding
- Running status support
- 32-voice polyphony using square wave synthesis
- Per-voice: frequency, velocity, ADSR envelope
- Tempo tracking (microseconds per quarter note)
- Looping support for continuous playback

`--hq-midi` enables a short causal low-pass filter after OPL2 synthesis. It
reduces high-frequency quantization stair steps while preserving MIDI timing;
the default path remains the unfiltered compatibility output.

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

## CD Audio (CDDA)

Liberation: Captive 2 on CD32 includes 10 Red Book audio tracks (tracks 02-11
on the disc). The CDDA player loads individual track .bin files extracted from
the multi-file CUE disc image and plays them through a dedicated SDL3 stereo
audio stream at 44,100 Hz, 16-bit signed, 2 channels.

The player supports per-track file loading (`cdda_load_track_file`), raw PCM
buffer loading (`cdda_load_track_raw`), and single-file BIN/CUE parsing
(`cdda_load_bin_cue`). Volume is controlled independently from the main sound
system. Playback supports looping.

## Speech/Voice Samples

The speech system loads voice samples via the existing 8SVX decoder and plays
them through the main sound system. Each sample is identified by a content
hash pair (hash_hi, hash_lo) for verification. Up to 64 samples can be loaded
simultaneously.
