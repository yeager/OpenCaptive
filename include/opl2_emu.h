#ifndef OPL2_EMU_H
#define OPL2_EMU_H

#include <stdint.h>
#include <stdbool.h>

/* Minimal OPL2 (YM3812) emulator for rendering Captive's AdLib sound effects.
 * Implements the 9-channel, 2-operator FM synthesis pipeline at the register
 * level. Accuracy is sufficient for SFX playback — not a cycle-exact clone. */

#define OPL2_NUM_CHANNELS 9
#define OPL2_SAMPLE_RATE 49716

typedef struct {
    uint32_t phase;
    uint32_t freq;
    uint8_t  mult;
    uint8_t  ksl;
    uint8_t  tl;
    uint8_t  ar, dr, sl, rr;
    uint8_t  waveform;
    bool     am, vib, eg, ksr;

    int32_t  env_level;
    uint8_t  env_stage;  // 0=off, 1=attack, 2=decay, 3=sustain, 4=release
    uint32_t env_counter;
} OPL2Operator;

typedef struct {
    OPL2Operator op[2];
    uint16_t fnum;
    uint8_t  block;
    uint8_t  fb;
    bool     cnt;       // 0=FM, 1=additive
    bool     key_on;
    int32_t  feedback[2];
} OPL2Channel;

typedef struct {
    OPL2Channel channels[OPL2_NUM_CHANNELS];
    uint8_t     regs[256];
    bool        waveform_enable;
    uint8_t     timer_ctl;
    uint32_t    noise_rng;
} OPL2;

void opl2_init(OPL2 *opl);
void opl2_write(OPL2 *opl, uint8_t reg, uint8_t val);
void opl2_render(OPL2 *opl, int16_t *buffer, int num_samples);

#endif
