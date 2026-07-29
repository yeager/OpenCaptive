#include "sound.h"
#include <stdlib.h>
#include <string.h>

bool sound_init(SoundSystem *snd) {
    memset(snd, 0, sizeof(*snd));
    snd->master_volume = 0.8f;

    SDL_AudioSpec spec = {
        .format = SDL_AUDIO_S16,
        .channels = 1,
        .freq = SOUND_SAMPLE_RATE,
    };

    snd->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                             &spec, NULL, NULL);
    if (!snd->stream) return false;

    SDL_ResumeAudioStreamDevice(snd->stream);
    snd->initialized = true;
    return true;
}

void sound_shutdown(SoundSystem *snd) {
    if (!snd->initialized) return;
    if (snd->stream) {
        SDL_DestroyAudioStream(snd->stream);
    }
    for (int i = 0; i < snd->num_samples; i++) {
        free(snd->samples[i].data);
    }
    memset(snd, 0, sizeof(*snd));
}

int sound_load_raw(SoundSystem *snd, const int8_t *data, uint32_t length, uint32_t rate) {
    if (snd->num_samples >= MAX_SOUND_SAMPLES) return -1;

    int id = snd->num_samples++;
    SoundSample *s = &snd->samples[id];
    s->data = malloc(length);
    if (!s->data) { snd->num_samples--; return -1; }
    memcpy(s->data, data, length);
    s->length = length;
    s->sample_rate = rate;
    return id;
}

// Parse IFF 8SVX format (Amiga standard sound format)
static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}

static uint16_t read_be16(const uint8_t *p) {
    return ((uint16_t)p[0]<<8)|p[1];
}

int sound_load_8svx(SoundSystem *snd, const uint8_t *data, uint32_t size) {
    if (size < 12) return -1;
    if (memcmp(data, "FORM", 4) != 0) return -1;
    if (memcmp(data + 8, "8SVX", 4) != 0) return -1;

    uint32_t sample_rate = 8000;
    const uint8_t *body = NULL;
    uint32_t body_len = 0;

    uint32_t pos = 12;
    while (pos + 8 <= size) {
        uint32_t chunk_size = read_be32(data + pos + 4);
        if (memcmp(data + pos, "VHDR", 4) == 0 && chunk_size >= 20) {
            // Voice header: samplesPerSec at offset 12 (2 bytes)
            sample_rate = read_be16(data + pos + 8 + 12);
            if (sample_rate == 0) sample_rate = 8000;
        } else if (memcmp(data + pos, "BODY", 4) == 0) {
            body = data + pos + 8;
            body_len = chunk_size;
        }
        pos += 8 + chunk_size;
        if (chunk_size & 1) pos++; // pad to even
    }

    if (!body || body_len == 0) return -1;
    return sound_load_raw(snd, (const int8_t *)body, body_len, sample_rate);
}

void sound_play(SoundSystem *snd, int sample_id, float volume, float pitch) {
    if (!snd->initialized || sample_id < 0 || sample_id >= snd->num_samples) return;

    // Find free channel
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (!snd->channels[i].playing) {
            snd->channels[i].sample = &snd->samples[sample_id];
            snd->channels[i].position = 0;
            snd->channels[i].volume = volume;
            snd->channels[i].pitch = pitch;
            snd->channels[i].playing = true;
            snd->channels[i].looping = false;
            return;
        }
    }
    // All channels busy: steal oldest
    snd->channels[0].sample = &snd->samples[sample_id];
    snd->channels[0].position = 0;
    snd->channels[0].volume = volume;
    snd->channels[0].pitch = pitch;
    snd->channels[0].playing = true;
    snd->channels[0].looping = false;
}

void sound_play_loop(SoundSystem *snd, int sample_id, float volume, float pitch) {
    sound_play(snd, sample_id, volume, pitch);
    // Set last played channel to looping
    for (int i = MAX_CHANNELS - 1; i >= 0; i--) {
        if (snd->channels[i].playing && snd->channels[i].sample == &snd->samples[sample_id]) {
            snd->channels[i].looping = true;
            return;
        }
    }
}

void sound_stop_all(SoundSystem *snd) {
    for (int i = 0; i < MAX_CHANNELS; i++)
        snd->channels[i].playing = false;
}

void sound_mix(SoundSystem *snd) {
    if (!snd->initialized) return;

    // Mix 1024 samples at a time
    int16_t buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    for (int i = 0; i < MAX_CHANNELS; i++) {
        SoundChannel *ch = &snd->channels[i];
        if (!ch->playing || !ch->sample) continue;

        float step = (float)ch->sample->sample_rate / SOUND_SAMPLE_RATE * ch->pitch;
        float vol = ch->volume * snd->master_volume * 256.0f;

        for (int s = 0; s < 1024; s++) {
            uint32_t pos = (uint32_t)(ch->position + s * step);
            if (pos >= ch->sample->length) {
                if (ch->looping) {
                    pos %= ch->sample->length;
                } else {
                    ch->playing = false;
                    break;
                }
            }
            int32_t sample = (int32_t)ch->sample->data[pos] * (int32_t)vol;
            int32_t mixed = buffer[s] + sample;
            if (mixed > 32767) mixed = 32767;
            if (mixed < -32768) mixed = -32768;
            buffer[s] = (int16_t)mixed;
        }
        ch->position += (uint32_t)(1024 * step);
        if (ch->position >= ch->sample->length) {
            if (ch->looping) ch->position %= ch->sample->length;
            else ch->playing = false;
        }
    }

    SDL_PutAudioStreamData(snd->stream, buffer, sizeof(buffer));
}
