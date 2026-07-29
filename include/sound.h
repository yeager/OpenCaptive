#ifndef SOUND_H
#define SOUND_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_SOUND_SAMPLES 64
#define SOUND_SAMPLE_RATE 22050
#define MAX_CHANNELS 8

typedef struct {
    int8_t  *data;
    uint32_t length;
    uint32_t sample_rate;
} SoundSample;

typedef struct {
    const SoundSample *sample;
    uint32_t position;
    float    volume;
    float    pitch;
    bool     playing;
    bool     looping;
} SoundChannel;

typedef struct {
    SDL_AudioStream *stream;
    SDL_AudioDeviceID device;
    SoundSample samples[MAX_SOUND_SAMPLES];
    int num_samples;
    SoundChannel channels[MAX_CHANNELS];
    float master_volume;
    bool initialized;
    bool enabled;
} SoundSystem;

bool sound_init(SoundSystem *snd);
void sound_shutdown(SoundSystem *snd);
int  sound_load_raw(SoundSystem *snd, const int8_t *data, uint32_t length, uint32_t rate);
int  sound_load_8svx(SoundSystem *snd, const uint8_t *data, uint32_t size);
void sound_play(SoundSystem *snd, int sample_id, float volume, float pitch);
void sound_play_loop(SoundSystem *snd, int sample_id, float volume, float pitch);
void sound_stop_all(SoundSystem *snd);
void sound_set_enabled(SoundSystem *snd, bool enabled);
void sound_mix(SoundSystem *snd);

#endif
