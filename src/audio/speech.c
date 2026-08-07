#include "speech.h"
#include <string.h>

void speech_init(SpeechSystem *sp, SoundSystem *snd) {
    if (!sp) return;
    memset(sp, 0, sizeof(*sp));
    sp->sound = snd;
    sp->volume = 1.0f;
    sp->enabled = true;
}

bool speech_load_8svx(SpeechSystem *sp, const uint8_t *data, uint32_t size,
                      uint32_t hash_hi, uint32_t hash_lo) {
    if (!sp || !sp->sound || !data || size < 12 ||
        sp->sample_count >= SPEECH_MAX_SAMPLES)
        return false;

    int sid = sound_load_8svx(sp->sound, data, size);
    if (sid < 0) return false;

    SpeechSample *s = &sp->samples[sp->sample_count++];
    s->sample_id = sid;
    s->hash_hi = hash_hi;
    s->hash_lo = hash_lo;
    return true;
}

void speech_play(SpeechSystem *sp, unsigned index, float volume) {
    if (!sp || !sp->sound || !sp->enabled ||
        index >= sp->sample_count) return;
    sound_play(sp->sound, sp->samples[index].sample_id,
               volume * sp->volume, 1.0f);
}

void speech_stop(SpeechSystem *sp) {
    if (!sp || !sp->sound) return;
    sound_stop_all(sp->sound);
}

void speech_set_volume(SpeechSystem *sp, float volume) {
    if (!sp) return;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    sp->volume = volume;
}

void speech_set_enabled(SpeechSystem *sp, bool enabled) {
    if (!sp) return;
    sp->enabled = enabled;
}

void speech_shutdown(SpeechSystem *sp) {
    if (!sp) return;
    sp->sample_count = 0;
}
