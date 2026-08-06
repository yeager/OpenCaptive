#ifndef MIDI_PLAYER_H
#define MIDI_PLAYER_H

#include "opl2_emu.h"
#include "adlib_data.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MIDI_MAX_TRACKS   16
#define MIDI_MAX_CHANNELS 16
#define OPL2_VOICE_COUNT  9

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
    uint8_t  program;
    uint8_t  volume;
} MIDIChannelState;

typedef struct {
    int     midi_channel;
    uint8_t note;
    bool    active;
    uint32_t age;
} OPL2VoiceAlloc;

typedef struct {
    OPL2          opl;
    MIDITrack     tracks[MIDI_MAX_TRACKS];
    int           num_tracks;
    MIDIChannelState channels[MIDI_MAX_CHANNELS];
    OPL2VoiceAlloc voices[OPL2_VOICE_COUNT];
    uint16_t      ticks_per_beat;
    uint32_t      tempo;
    uint32_t      current_tick;
    int           samples_per_tick;
    int           tick_remainder;
    int           tick_frac;
    int           sample_rate;
    int16_t       hq_previous_sample;
    float         master_volume;
    bool          high_quality;
    bool          playing;
    bool          looping;
    const uint8_t *file_data;
    size_t         file_size;
} MIDIPlayer;

bool midi_load(MIDIPlayer *player, const uint8_t *data, size_t size);
void midi_play(MIDIPlayer *player, bool loop);
void midi_stop(MIDIPlayer *player);
void midi_render(MIDIPlayer *player, int16_t *buffer, int num_samples);
void midi_set_volume(MIDIPlayer *player, float vol);
void midi_set_sample_rate(MIDIPlayer *player, int sample_rate);
void midi_set_high_quality(MIDIPlayer *player, bool enabled);

#endif
