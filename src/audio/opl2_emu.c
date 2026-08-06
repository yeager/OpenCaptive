#include "opl2_emu.h"
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* OPL2 multiplier table (register 0x20 bits 3-0) */
static const uint8_t mult_table[16] = {
    1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 12, 12, 15, 15
};
/* The OPL2 spec says index 0 = 0.5×, but we use integer math with a later >>1 */

/* Sine table: 1024 entries, 16-bit log-sin values */
#define SINE_TABLE_SIZE 1024
static int16_t sine_table[SINE_TABLE_SIZE];
static int16_t exp_table[256];
static bool tables_initialized = false;

static void init_tables(void) {
    if (tables_initialized) return;
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        double phase = (i + 0.5) * M_PI / SINE_TABLE_SIZE;
        double val = sin(phase);
        if (val > 0.0)
            sine_table[i] = (int16_t)(-log2(val) * 256.0 + 0.5);
        else
            sine_table[i] = 0x7FFF;
    }
    for (int i = 0; i < 256; i++) {
        exp_table[i] = (int16_t)(pow(2.0, (255 - i) / 256.0) * 1024.0 + 0.5);
    }
    tables_initialized = true;
}

/* Convert log-sin + attenuation to linear sample */
static int16_t log_sin_to_linear(uint16_t logsin) {
    bool sign = (logsin & 0x8000) != 0;
    uint16_t level = logsin & 0x7FFF;
    if (level >= 0x2000) return 0;
    int idx = (level >> 3) & 0xFF;
    int shift = (level >> 11) & 0xF;
    int16_t result = exp_table[idx] >> shift;
    return sign ? -result : result;
}

/* Waveform generation */
static uint16_t calc_phase_output(uint32_t phase, uint8_t waveform) {
    uint32_t idx = (phase >> 10) & 0x3FF;
    uint16_t out;

    switch (waveform & 3) {
    case 0: /* full sine */
        if (idx < SINE_TABLE_SIZE / 2)
            out = sine_table[idx];
        else
            out = sine_table[SINE_TABLE_SIZE - 1 - idx] | 0x8000;
        break;
    case 1: /* half sine (positive only) */
        if (idx < SINE_TABLE_SIZE / 2)
            out = sine_table[idx];
        else
            out = 0x7FFF;
        break;
    case 2: /* absolute sine */
        if (idx < SINE_TABLE_SIZE / 2)
            out = sine_table[idx];
        else
            out = sine_table[SINE_TABLE_SIZE - 1 - idx];
        break;
    case 3: /* quarter sine (rising only) */
        if (idx < SINE_TABLE_SIZE / 4)
            out = sine_table[idx];
        else
            out = 0x7FFF;
        break;
    }
    return out;
}

/* Envelope rate calculation */
static uint32_t calc_rate(uint8_t rate, uint8_t ksr_val) {
    if (rate == 0) return 0;
    int effective = (rate << 2) + ksr_val;
    if (effective > 63) effective = 63;
    return (uint32_t)effective;
}

/* Update envelope for one operator */
static void update_envelope(OPL2Operator *op, bool key_on, uint8_t ksr_val) {
    uint32_t rate;

    switch (op->env_stage) {
    case 0: /* off */
        op->env_level = 0x1FF;
        if (key_on) {
            op->env_stage = 1;
            op->env_counter = 0;
        }
        break;
    case 1: /* attack */
        rate = calc_rate(op->ar, ksr_val);
        if (rate >= 60) {
            op->env_level = 0;
            op->env_stage = 2;
        } else if (rate > 0) {
            int shift = 13 - (rate >> 2);
            if (shift < 0) shift = 0;
            if ((op->env_counter & ((1U << shift) - 1)) == 0) {
                int step = 4 + (rate & 3);
                op->env_level -= ((op->env_level * step) >> 4) + 1;
                if (op->env_level <= 0) {
                    op->env_level = 0;
                    op->env_stage = 2;
                }
            }
        }
        op->env_counter++;
        break;
    case 2: /* decay */
        rate = calc_rate(op->dr, ksr_val);
        if (rate > 0) {
            int shift = 13 - (rate >> 2);
            if (shift < 0) shift = 0;
            if ((op->env_counter & ((1U << shift) - 1)) == 0) {
                op->env_level += (4 + (rate & 3));
            }
        }
        if (op->env_level >= (int32_t)(op->sl << 5)) {
            op->env_level = op->sl << 5;
            op->env_stage = op->eg ? 3 : 4;
        }
        op->env_counter++;
        break;
    case 3: /* sustain */
        if (!key_on) {
            op->env_stage = 4;
            op->env_counter = 0;
        }
        break;
    case 4: /* release */
        rate = calc_rate(op->rr, ksr_val);
        if (rate > 0) {
            int shift = 13 - (rate >> 2);
            if (shift < 0) shift = 0;
            if ((op->env_counter & ((1U << shift) - 1)) == 0) {
                op->env_level += (4 + (rate & 3));
            }
        }
        if (op->env_level >= 0x1FF) {
            op->env_level = 0x1FF;
            op->env_stage = 0;
        }
        op->env_counter++;
        break;
    }
}

/* Channel-to-operator register offset mapping */
static const int ch_to_op[9] = { 0, 1, 2, 8, 9, 10, 16, 17, 18 };

static void update_channel_params(OPL2 *opl, int ch) {
    OPL2Channel *c = &opl->channels[ch];
    int base = ch_to_op[ch];

    for (int op = 0; op < 2; op++) {
        int reg_off = base + op * 3;
        OPL2Operator *o = &c->op[op];

        uint8_t r20 = opl->regs[0x20 + reg_off];
        o->am   = (r20 >> 7) & 1;
        o->vib  = (r20 >> 6) & 1;
        o->eg   = (r20 >> 5) & 1;
        o->ksr  = (r20 >> 4) & 1;
        o->mult = mult_table[r20 & 0x0F];

        uint8_t r40 = opl->regs[0x40 + reg_off];
        o->ksl = (r40 >> 6) & 3;
        o->tl  = r40 & 0x3F;

        uint8_t r60 = opl->regs[0x60 + reg_off];
        o->ar = (r60 >> 4) & 0x0F;
        o->dr = r60 & 0x0F;

        uint8_t r80 = opl->regs[0x80 + reg_off];
        o->sl = (r80 >> 4) & 0x0F;
        o->rr = r80 & 0x0F;

        o->waveform = opl->waveform_enable ? (opl->regs[0xE0 + reg_off] & 3) : 0;
    }

    uint8_t rA0 = opl->regs[0xA0 + ch];
    uint8_t rB0 = opl->regs[0xB0 + ch];
    c->fnum = rA0 | ((rB0 & 3) << 8);
    c->block = (rB0 >> 2) & 7;
    bool new_key = (rB0 >> 5) & 1;

    if (new_key && !c->key_on) {
        c->op[0].env_stage = 1;
        c->op[0].env_level = 0x1FF;
        c->op[0].env_counter = 0;
        c->op[0].phase = 0;
        c->op[1].env_stage = 1;
        c->op[1].env_level = 0x1FF;
        c->op[1].env_counter = 0;
        c->op[1].phase = 0;
    } else if (!new_key && c->key_on) {
        if (c->op[0].env_stage != 0) c->op[0].env_stage = 4;
        if (c->op[1].env_stage != 0) c->op[1].env_stage = 4;
    }
    c->key_on = new_key;

    uint8_t rC0 = opl->regs[0xC0 + ch];
    c->fb  = (rC0 >> 1) & 7;
    c->cnt = rC0 & 1;
}

void opl2_init(OPL2 *opl) {
    if (!opl) return;
    init_tables();
    memset(opl, 0, sizeof(*opl));
    opl->noise_rng = 1;
    for (int ch = 0; ch < OPL2_NUM_CHANNELS; ch++) {
        opl->channels[ch].op[0].env_level = 0x1FF;
        opl->channels[ch].op[1].env_level = 0x1FF;
    }
}

void opl2_write(OPL2 *opl, uint8_t reg, uint8_t val) {
    if (!opl) return;
    opl->regs[reg] = val;

    if (reg == 0x01) {
        opl->waveform_enable = (val >> 5) & 1;
    }

    if (reg >= 0x20 && reg <= 0x35) {
        for (int ch = 0; ch < 9; ch++) update_channel_params(opl, ch);
    } else if (reg >= 0x40 && reg <= 0x55) {
        for (int ch = 0; ch < 9; ch++) update_channel_params(opl, ch);
    } else if (reg >= 0x60 && reg <= 0x75) {
        for (int ch = 0; ch < 9; ch++) update_channel_params(opl, ch);
    } else if (reg >= 0x80 && reg <= 0x95) {
        for (int ch = 0; ch < 9; ch++) update_channel_params(opl, ch);
    } else if (reg >= 0xA0 && reg <= 0xB8) {
        int ch = reg & 0x0F;
        if (ch < 9) update_channel_params(opl, ch);
    } else if (reg >= 0xC0 && reg <= 0xC8) {
        int ch = reg - 0xC0;
        update_channel_params(opl, ch);
    } else if (reg >= 0xE0 && reg <= 0xF5) {
        for (int ch = 0; ch < 9; ch++) update_channel_params(opl, ch);
    }
}

void opl2_render(OPL2 *opl, int16_t *buffer, int num_samples) {
    if (!opl || !buffer || num_samples <= 0) return;
    for (int s = 0; s < num_samples; s++) {
        int32_t output = 0;

        for (int ch = 0; ch < OPL2_NUM_CHANNELS; ch++) {
            OPL2Channel *c = &opl->channels[ch];
            if (c->op[0].env_stage == 0 && c->op[1].env_stage == 0)
                continue;

            uint32_t freq = (uint32_t)c->fnum << c->block;
            uint8_t ksr_val = c->block;

            /* Update envelopes */
            update_envelope(&c->op[0], c->key_on, ksr_val);
            update_envelope(&c->op[1], c->key_on, ksr_val);

            /* Advance phases */
            uint32_t op0_freq = freq * c->op[0].mult;
            uint32_t op1_freq = freq * c->op[1].mult;
            if (c->op[0].mult == 1 && (opl->regs[0x20 + ch_to_op[ch]] & 0x0F) == 0)
                op0_freq >>= 1;
            if (c->op[1].mult == 1 && (opl->regs[0x20 + ch_to_op[ch] + 3] & 0x0F) == 0)
                op1_freq >>= 1;

            c->op[0].phase += op0_freq;
            c->op[1].phase += op1_freq;

            /* Operator 0 output with feedback */
            int32_t fb_mod = 0;
            if (c->fb > 0)
                fb_mod = (c->feedback[0] + c->feedback[1]) >> (9 - c->fb);

            uint16_t op0_out = calc_phase_output(c->op[0].phase + (uint32_t)fb_mod, c->op[0].waveform);
            int32_t op0_env = (c->op[0].tl << 3) + c->op[0].env_level;
            if (op0_env > 0x1FF) op0_env = 0x1FF;
            int16_t op0_sample = log_sin_to_linear(op0_out + (uint16_t)(op0_env << 3));

            c->feedback[1] = c->feedback[0];
            c->feedback[0] = op0_sample;

            /* Operator 1 output */
            uint32_t op1_phase = c->op[1].phase;
            if (!c->cnt) {
                /*
                 * The feedback phase is a signed sample scaled by 1024.
                 * Multiplication avoids shifting a negative signed value
                 * (undefined in C), while the final conversion preserves
                 * the intended modulo-2^32 phase arithmetic.
                 */
                int64_t phase_mod = (int64_t)op0_sample * 1024;
                op1_phase += (uint32_t)phase_mod;
            }

            uint16_t op1_out = calc_phase_output(op1_phase, c->op[1].waveform);
            int32_t op1_env = (c->op[1].tl << 3) + c->op[1].env_level;
            if (op1_env > 0x1FF) op1_env = 0x1FF;
            int16_t op1_sample = log_sin_to_linear(op1_out + (uint16_t)(op1_env << 3));

            if (c->cnt)
                output += op0_sample + op1_sample;
            else
                output += op1_sample;
        }

        /* Clamp */
        if (output > 32767) output = 32767;
        if (output < -32768) output = -32768;
        buffer[s] = (int16_t)output;
    }
}
