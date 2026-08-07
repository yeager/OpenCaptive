#include "custom_features.h"
#include <math.h>
#include <string.h>

#define REVERB_BUFFER_SIZE 8192
#define REVERB_DELAY_1 1557
#define REVERB_DELAY_2 2179
#define REVERB_DELAY_3 3119
#define REVERB_DELAY_4 4409

static int16_t delay_buf[REVERB_BUFFER_SIZE];
static int delay_pos = 0;

void reverb_reset(void) {
    memset(delay_buf, 0, sizeof(delay_buf));
    delay_pos = 0;
}

void reverb_process(int16_t *buffer, int num_samples, float amount) {
    if (!buffer || num_samples <= 0 || !isfinite(amount) || amount <= 0.0f) return;
    if (amount > 1.0f) amount = 1.0f;

    float feedback = amount * 0.4f;
    float wet = amount * 0.3f;

    for (int i = 0; i < num_samples; i++) {
        int16_t dry = buffer[i];

        int t1 = (delay_pos - REVERB_DELAY_1 + REVERB_BUFFER_SIZE) % REVERB_BUFFER_SIZE;
        int t2 = (delay_pos - REVERB_DELAY_2 + REVERB_BUFFER_SIZE) % REVERB_BUFFER_SIZE;
        int t3 = (delay_pos - REVERB_DELAY_3 + REVERB_BUFFER_SIZE) % REVERB_BUFFER_SIZE;
        int t4 = (delay_pos - REVERB_DELAY_4 + REVERB_BUFFER_SIZE) % REVERB_BUFFER_SIZE;

        float rev = (delay_buf[t1] + delay_buf[t2] * 0.7f +
                     delay_buf[t3] * 0.5f + delay_buf[t4] * 0.3f) * 0.25f;

        float delayed = dry + rev * feedback;
        if (delayed > 32767.0f) delayed = 32767.0f;
        if (delayed < -32768.0f) delayed = -32768.0f;
        delay_buf[delay_pos] = (int16_t)delayed;
        delay_pos = (delay_pos + 1) % REVERB_BUFFER_SIZE;

        int32_t out = (int32_t)(dry * (1.0f - wet) + rev * wet);
        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;
        buffer[i] = (int16_t)out;
    }
}
