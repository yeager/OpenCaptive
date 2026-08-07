#include "sound.h"
#include "custom_features.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

bool sound_init(SoundSystem *snd, uint32_t sample_rate) {
    if (!snd) return false;
    if (sample_rate != 22050 && sample_rate != 44100 && sample_rate != 48000)
        sample_rate = SOUND_SAMPLE_RATE;
    memset(snd, 0, sizeof(*snd));
    snd->sample_rate = sample_rate;
    snd->master_volume = 0.8f;
    snd->reverb_amount = 0.3f;

    SDL_AudioSpec spec = {
        .format = SDL_AUDIO_S16,
        .channels = 1,
        .freq = (int)sample_rate,
    };

    snd->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                             &spec, NULL, NULL);
    if (!snd->stream) return false;

    SDL_ResumeAudioStreamDevice(snd->stream);
    snd->initialized = true;
    snd->enabled = true;
    return true;
}

void sound_shutdown(SoundSystem *snd) {
    if (!snd) return;
    if (snd->stream) {
        SDL_DestroyAudioStream(snd->stream);
    }
    for (int i = 0; i < snd->num_samples; i++) {
        free(snd->samples[i].data);
    }
    memset(snd, 0, sizeof(*snd));
}

int sound_load_raw(SoundSystem *snd, const int8_t *data, uint32_t length, uint32_t rate) {
    if (!snd || (!data && length != 0) || length == 0 || rate == 0) return -1;
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
    if (!snd || !data || size < 12) return -1;
    if (memcmp(data, "FORM", 4) != 0) return -1;
    if (memcmp(data + 8, "8SVX", 4) != 0) return -1;
    uint32_t form_size = read_be32(data + 4);
    if (form_size < 4U || form_size > size - 8U) return -1;
    uint32_t form_end = 8U + form_size;

    uint32_t sample_rate = 8000;
    const uint8_t *body = NULL;
    uint32_t body_len = 0;

    uint32_t pos = 12;
    while (pos + 8 <= form_end) {
        uint32_t chunk_size = read_be32(data + pos + 4);
        uint64_t next = (uint64_t)pos + 8U + chunk_size + (chunk_size & 1U);
        if (next > form_end) return -1;
        if (memcmp(data + pos, "VHDR", 4) == 0 && chunk_size >= 20) {
            // Voice header: samplesPerSec at offset 12 (2 bytes)
            sample_rate = read_be16(data + pos + 8 + 12);
            if (sample_rate == 0) sample_rate = 8000;
        } else if (memcmp(data + pos, "BODY", 4) == 0) {
            body = data + pos + 8;
            body_len = chunk_size;
        }
        pos = (uint32_t)next;
    }

    if (pos != form_end || !body || body_len == 0) return -1;
    return sound_load_raw(snd, (const int8_t *)body, body_len, sample_rate);
}

void sound_play(SoundSystem *snd, int sample_id, float volume, float pitch) {
    if (!snd || !snd->initialized || !snd->enabled || sample_id < 0 || sample_id >= snd->num_samples ||
        !snd->samples[sample_id].data || snd->samples[sample_id].length == 0 ||
        !isfinite(volume) || !isfinite(pitch) || pitch <= 0.0f) return;

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
    if (!snd || sample_id < 0 || sample_id >= snd->num_samples ||
        !isfinite(volume) || !isfinite(pitch) || pitch <= 0.0f) return;
    sound_play(snd, sample_id, volume, pitch);
    if (!snd->initialized || !snd->enabled || !snd->samples[sample_id].data ||
        snd->samples[sample_id].length == 0) return;
    // Set last played channel to looping
    for (int i = MAX_CHANNELS - 1; i >= 0; i--) {
        if (snd->channels[i].playing && snd->channels[i].sample == &snd->samples[sample_id]) {
            snd->channels[i].looping = true;
            return;
        }
    }
}

void sound_stop_all(SoundSystem *snd) {
    if (!snd) return;
    for (int i = 0; i < MAX_CHANNELS; i++)
        snd->channels[i].playing = false;
}

void sound_set_enabled(SoundSystem *snd, bool enabled) {
    if (!snd) return;
    snd->enabled = enabled;
    if (!enabled) sound_stop_all(snd);
}

void sound_set_volume(SoundSystem *snd, float volume) {
    if (!snd || !isfinite(volume)) return;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    snd->master_volume = volume;
}

void sound_set_reverb(SoundSystem *snd, bool enabled, float amount) {
    if (!snd || !isfinite(amount)) return;
    if (amount < 0.0f) amount = 0.0f;
    if (amount > 1.0f) amount = 1.0f;
    snd->reverb_amount = amount;
    bool was_enabled = snd->reverb_enabled;
    snd->reverb_enabled = enabled && amount > 0.0f;
    if (snd->reverb_enabled && !was_enabled)
        reverb_reset();
}

void sound_mix(SoundSystem *snd) {
    if (!snd || !snd->initialized || !snd->enabled || !snd->stream) return;
    if (!isfinite(snd->master_volume)) return;

    // Mix 1024 samples at a time
    int16_t buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    for (int i = 0; i < MAX_CHANNELS; i++) {
        SoundChannel *ch = &snd->channels[i];
        if (!ch->playing || !ch->sample || ch->sample->length == 0 ||
            !isfinite(ch->volume) || !isfinite(ch->pitch) || ch->pitch <= 0.0f) {
            if (ch->playing && ch->sample && ch->sample->length == 0)
                ch->playing = false;
            continue;
        }

        float step = (float)ch->sample->sample_rate / snd->sample_rate * ch->pitch;
        if (!isfinite(step) || step <= 0.0f) {
            ch->playing = false;
            continue;
        }
        float vol = ch->volume * snd->master_volume * 256.0f;
        if (!isfinite(vol)) {
            ch->playing = false;
            continue;
        }

        for (int s = 0; s < 1024; s++) {
            float sample_pos = (float)ch->position + (float)s * step;
            if (!isfinite(sample_pos) || sample_pos < 0.0f) {
                ch->playing = false;
                break;
            }
            uint32_t pos;
            if (sample_pos >= (double)ch->sample->length) {
                if (!ch->looping) {
                    ch->playing = false;
                    break;
                }
                pos = (uint32_t)fmod((double)sample_pos,
                                     (double)ch->sample->length);
            } else {
                pos = (uint32_t)sample_pos;
            }
            float scaled = (float)ch->sample->data[pos] * vol;
            /* Do not convert an out-of-range float directly to int32_t:
             * hostile or malformed configuration can otherwise invoke
             * undefined behaviour before the final 16-bit clamp. */
            if (scaled > 2147483647.0f) scaled = 2147483647.0f;
            if (scaled < -2147483648.0f) scaled = -2147483648.0f;
            int64_t mixed = (int64_t)buffer[s] + (int64_t)scaled;
            if (mixed > 32767) mixed = 32767;
            if (mixed < -32768) mixed = -32768;
            buffer[s] = (int16_t)mixed;
        }
        double advance = 1024.0 * (double)step;
        if (!isfinite(advance) || advance < 0.0) {
            ch->playing = false;
        } else if (ch->looping) {
            double wrapped = fmod(advance, (double)ch->sample->length);
            uint32_t delta = (uint32_t)wrapped;
            ch->position = (uint32_t)(((uint64_t)ch->position + delta) %
                                      ch->sample->length);
        } else if (advance >= (double)ch->sample->length - ch->position) {
            ch->playing = false;
        } else {
            ch->position += (uint32_t)advance;
        }
    }

    for (int a = 0; a < snd->aux_count; a++) {
        if (snd->aux_pending[a] && *snd->aux_pending[a]) {
            for (int s = 0; s < 1024; s++) {
                int32_t mixed = (int32_t)buffer[s] + (int32_t)snd->aux_buf[a][s];
                if (mixed > 32767) mixed = 32767;
                if (mixed < -32768) mixed = -32768;
                buffer[s] = (int16_t)mixed;
            }
            *snd->aux_pending[a] = false;
        }
    }

    if (snd->reverb_enabled)
        reverb_process(buffer, (int)(sizeof(buffer) / sizeof(buffer[0])),
                       snd->reverb_amount);
    SDL_PutAudioStreamData(snd->stream, buffer, sizeof(buffer));
}

void sound_register_aux(SoundSystem *snd, int16_t *buf, bool *pending) {
    if (!snd || !buf || !pending || snd->aux_count >= 4) return;
    snd->aux_buf[snd->aux_count] = buf;
    snd->aux_pending[snd->aux_count] = pending;
    snd->aux_count++;
}
