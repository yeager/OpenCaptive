#ifndef SPEECH_H
#define SPEECH_H

#include "sound.h"
#include "data_vfs.h"
#include <stdbool.h>
#include <stdint.h>

#define SPEECH_MAX_SAMPLES 64

typedef struct {
    int sample_id;
    uint32_t hash_hi;
    uint32_t hash_lo;
} SpeechSample;

typedef struct {
    SoundSystem *sound;
    SpeechSample samples[SPEECH_MAX_SAMPLES];
    unsigned sample_count;
    float volume;
    bool enabled;
} SpeechSystem;

void speech_init(SpeechSystem *sp, SoundSystem *snd);
bool speech_load_8svx(SpeechSystem *sp, const uint8_t *data, uint32_t size,
                      uint32_t hash_hi, uint32_t hash_lo);
void speech_play(SpeechSystem *sp, unsigned index, float volume);
void speech_stop(SpeechSystem *sp);
void speech_set_volume(SpeechSystem *sp, float volume);
void speech_set_enabled(SpeechSystem *sp, bool enabled);
void speech_shutdown(SpeechSystem *sp);

#endif
