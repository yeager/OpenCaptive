#include "sfx.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SFX_RATE 22050

static int gen_tone(SoundSystem *snd, float freq, float duration, float decay) {
    int len = (int)(SFX_RATE * duration);
    int8_t *buf = malloc(len);
    if (!buf) return -1;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SFX_RATE;
        float env = 1.0f - t / duration;
        if (decay > 0) env = powf(env, decay);
        float val = sinf(2.0f * 3.14159f * freq * t) * env * 80.0f;
        buf[i] = (int8_t)val;
    }
    int id = sound_load_raw(snd, buf, len, SFX_RATE);
    free(buf);
    return id;
}

static int gen_noise(SoundSystem *snd, float duration, float decay) {
    int len = (int)(SFX_RATE * duration);
    int8_t *buf = malloc(len);
    if (!buf) return -1;
    uint32_t rng = 12345;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SFX_RATE;
        float env = powf(1.0f - t / duration, decay);
        rng = rng * 1103515245 + 12345;
        int8_t noise = (int8_t)((rng >> 16) & 0xFF);
        buf[i] = (int8_t)(noise * env * 0.4f);
    }
    int id = sound_load_raw(snd, buf, len, SFX_RATE);
    free(buf);
    return id;
}

static int gen_sweep(SoundSystem *snd, float freq_start, float freq_end,
                     float duration, float decay) {
    int len = (int)(SFX_RATE * duration);
    int8_t *buf = malloc(len);
    if (!buf) return -1;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SFX_RATE;
        float frac = t / duration;
        float freq = freq_start + (freq_end - freq_start) * frac;
        float env = powf(1.0f - frac, decay);
        float val = sinf(2.0f * 3.14159f * freq * t) * env * 80.0f;
        buf[i] = (int8_t)val;
    }
    int id = sound_load_raw(snd, buf, len, SFX_RATE);
    free(buf);
    return id;
}

bool sfx_init(SfxSystem *sfx, SoundSystem *snd) {
    memset(sfx, 0, sizeof(*sfx));
    sfx->sound = snd;

    sfx->sample_ids[SFX_HIT]         = gen_noise(snd, 0.15f, 3.0f);
    sfx->sample_ids[SFX_SHOOT]       = gen_sweep(snd, 800.0f, 200.0f, 0.2f, 2.0f);
    sfx->sample_ids[SFX_DOOR_OPEN]   = gen_sweep(snd, 100.0f, 400.0f, 0.3f, 1.0f);
    sfx->sample_ids[SFX_DOOR_LOCKED] = gen_tone(snd, 150.0f, 0.2f, 4.0f);
    sfx->sample_ids[SFX_STEP]        = gen_noise(snd, 0.08f, 5.0f);
    sfx->sample_ids[SFX_BUTTON]      = gen_tone(snd, 1200.0f, 0.1f, 3.0f);
    sfx->sample_ids[SFX_PICKUP]      = gen_sweep(snd, 400.0f, 1200.0f, 0.15f, 1.5f);
    sfx->sample_ids[SFX_DEATH]       = gen_sweep(snd, 600.0f, 50.0f, 0.5f, 1.0f);
    sfx->sample_ids[SFX_LEVEL_UP]    = gen_sweep(snd, 300.0f, 900.0f, 0.4f, 0.5f);
    sfx->sample_ids[SFX_GENERATOR]   = gen_noise(snd, 0.4f, 1.5f);

    sfx->loaded = true;
    return true;
}

void sfx_play(SfxSystem *sfx, SfxType type) {
    if (!sfx->loaded || type < 0 || type >= SFX_COUNT) return;
    if (sfx->sample_ids[type] < 0) return;
    sound_play(sfx->sound, sfx->sample_ids[type], 0.5f, 1.0f);
}
