#ifndef MIDI_PLAYER_H
#define MIDI_PLAYER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MIDI_MAX_TRACKS 16
#define MIDI_MAX_CHANNELS 16
#define MIDI_MAX_NOTES 128
#define MIDI_SAMPLE_RATE 22050

// MIDI event types
typedef enum {
    MIDI_NOTE_OFF     = 0x80,
    MIDI_NOTE_ON      = 0x90,
    MIDI_KEY_PRESSURE = 0xA0,
    MIDI_CONTROL      = 0xB0,
    MIDI_PROGRAM      = 0xC0,
    MIDI_PRESSURE     = 0xD0,
    MIDI_PITCH_BEND   = 0xE0,
    MIDI_SYSEX        = 0xF0,
    MIDI_META         = 0xFF,
} MIDIEventType;

typedef struct {
    const uint8_t *data;
    uint32_t       length;
    uint32_t       pos;
    uint32_t       next_tick;
    uint8_t        running_status;
    bool           ended;
} MIDITrack;

typedef struct {
    float    phase;
    float    freq;
    float    velocity;
    float    env;       // envelope (0-1)
    float    env_rate;  // release rate
    uint8_t  note;
    bool     active;
    bool     releasing;
} MIDIVoice;

typedef struct {
    uint8_t  program;
    uint8_t  volume;
    uint8_t  pan;
    uint16_t pitch_bend;
} MIDIChannelState;

typedef struct {
    MIDITrack   tracks[MIDI_MAX_TRACKS];
    int         num_tracks;
    uint16_t    ticks_per_beat;
    uint32_t    tempo;           // microseconds per beat
    uint32_t    current_tick;
    float       tick_accum;
    float       ticks_per_sample;

    MIDIChannelState channels[MIDI_MAX_CHANNELS];
    MIDIVoice   voices[32];      // polyphony limit

    bool        playing;
    bool        looping;
    float       volume;

    // Original data for restart
    const uint8_t *file_data;
    size_t         file_size;
} MIDIPlayer;

bool midi_load(MIDIPlayer *player, const uint8_t *data, size_t size);
void midi_play(MIDIPlayer *player, bool loop);
void midi_stop(MIDIPlayer *player);
void midi_render(MIDIPlayer *player, int16_t *buffer, int num_samples);
void midi_set_volume(MIDIPlayer *player, float vol);

#endif
