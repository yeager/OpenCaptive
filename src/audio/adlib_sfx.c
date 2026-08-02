#include "adlib_sfx.h"
#include <string.h>

/* The SFX bytecode format recovered from CAP_A.BIN:
 *
 * Opcodes (high bit set):
 *   0x80 NN  - delay NN ticks
 *   0x82 NN  - set note (frequency index into ftable)
 *   0x83 NN  - key off (NN = flags/channel modifier)
 *   0x84 NN  - load instrument patch (NN = patch index, adjusted)
 *
 * Data bytes (high bit clear):
 *   Used as inline register values or modifiers for the preceding opcode.
 *
 * 0xFF - end of sequence
 *
 * The original driver runs at ~70 Hz (DOS timer tick rate).
 * Each "tick" in a delay corresponds to one timer interrupt. */

#define SFX_TICK_RATE 70

static void load_patch(OPL2 *opl, int ch, int patch_idx) {
    if (patch_idx < 0 || patch_idx >= CAPTIVE_OPL2_PATCH_COUNT) return;

    const uint8_t *p = captive_opl2_patches[patch_idx];
    /* Channel-to-operator register offset */
    static const int ch_op_off[9] = { 0, 1, 2, 8, 9, 10, 16, 17, 18 };
    if (ch < 0 || ch >= 9) return;
    int off = ch_op_off[ch];

    opl2_write(opl, 0x20 + off,     p[2]);  // op1 AM/VIB/EG/KSR/MULT
    opl2_write(opl, 0x20 + off + 3, p[3]);  // op2
    opl2_write(opl, 0x40 + off,     p[4]);  // op1 KSL/TL
    opl2_write(opl, 0x40 + off + 3, p[5]);  // op2
    opl2_write(opl, 0x60 + off,     p[6]);  // op1 AR/DR
    opl2_write(opl, 0x60 + off + 3, p[7]);  // op2
    opl2_write(opl, 0x80 + off,     p[8]);  // op1 SL/RR
    opl2_write(opl, 0x80 + off + 3, p[9]);  // op2
    opl2_write(opl, 0xE0 + off,     p[10]); // op1 waveform
    opl2_write(opl, 0xE0 + off + 3, p[11]); // op2 waveform
    opl2_write(opl, 0xC0 + ch,      0x00);  // FB/connection (reset)
}

static void key_on(OPL2 *opl, int ch, uint8_t note) {
    if (ch < 0 || ch >= 9) return;
    int fnum_idx = note & 0x7F;
    if (fnum_idx >= 128) fnum_idx = 127;

    uint16_t fnum = captive_opl2_ftable[fnum_idx];
    int block = (note >> 5) & 7;
    if (block > 7) block = 7;

    opl2_write(opl, 0xA0 + ch, fnum & 0xFF);
    opl2_write(opl, 0xB0 + ch, 0x20 | ((block & 7) << 2) | ((fnum >> 8) & 3));
}

static void key_off(OPL2 *opl, int ch) {
    if (ch < 0 || ch >= 9) return;
    uint8_t cur = opl->regs[0xB0 + ch];
    opl2_write(opl, 0xB0 + ch, cur & ~0x20);
}

static void process_tick(AdlibSfxPlayer *player, AdlibSfxVoice *v) {
    if (!v->active) return;
    if (v->delay > 0) {
        v->delay--;
        return;
    }

    while (v->pos < v->size) {
        uint8_t b = v->data[v->pos];

        if (b == 0xFF) {
            v->active = false;
            key_off(&player->opl, v->channel);
            return;
        }

        if (b == 0x80 && v->pos + 1 < v->size) {
            v->delay = v->data[v->pos + 1];
            v->pos += 2;
            return;
        }

        if (b == 0x82 && v->pos + 1 < v->size) {
            /* Set frequency without key-on (pitch change) */
            int fnum_idx = v->data[v->pos + 1] & 0x7F;
            if (fnum_idx >= 128) fnum_idx = 127;
            uint16_t fnum = captive_opl2_ftable[fnum_idx];
            int block = (v->data[v->pos + 1] >> 5) & 7;
            uint8_t cur_b0 = player->opl.regs[0xB0 + v->channel];
            opl2_write(&player->opl, 0xA0 + v->channel, fnum & 0xFF);
            opl2_write(&player->opl, 0xB0 + v->channel,
                       (cur_b0 & 0x20) | ((block & 7) << 2) | ((fnum >> 8) & 3));
            v->pos += 2;
            continue;
        }

        if (b == 0x83 && v->pos + 1 < v->size) {
            uint8_t note = v->data[v->pos + 1];
            if (note == 0x00) {
                key_off(&player->opl, v->channel);
            } else {
                key_on(&player->opl, v->channel, note);
            }
            v->pos += 2;
            continue;
        }

        if (b == 0x84 && v->pos + 1 < v->size) {
            int patch = v->data[v->pos + 1];
            if (patch >= 0x80) patch -= 0x80;
            load_patch(&player->opl, v->channel, patch);
            v->pos += 2;
            continue;
        }

        /* Unknown or data byte — skip */
        v->pos++;
    }

    v->active = false;
}

void adlib_sfx_init(AdlibSfxPlayer *player, int sample_rate) {
    memset(player, 0, sizeof(*player));
    opl2_init(&player->opl);
    opl2_write(&player->opl, 0x01, 0x20); // enable waveform select
    player->samples_per_tick = sample_rate / SFX_TICK_RATE;
    if (player->samples_per_tick < 1) player->samples_per_tick = 1;
}

bool adlib_sfx_play(AdlibSfxPlayer *player, int sfx_id) {
    if (sfx_id < 0 || sfx_id >= CAPTIVE_SFX_COUNT) return false;

    /* Find a free voice slot */
    int slot = -1;
    for (int i = 0; i < ADLIB_SFX_MAX_ACTIVE; i++) {
        if (!player->voices[i].active) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        /* Steal oldest */
        slot = 0;
        key_off(&player->opl, player->voices[0].channel);
    }

    AdlibSfxVoice *v = &player->voices[slot];
    v->data = captive_sfx_table[sfx_id].data;
    v->size = captive_sfx_table[sfx_id].size;
    v->pos = 0;
    v->delay = 0;
    v->channel = slot; // each voice gets its own OPL2 channel
    v->active = true;

    return true;
}

void adlib_sfx_stop_all(AdlibSfxPlayer *player) {
    for (int i = 0; i < ADLIB_SFX_MAX_ACTIVE; i++) {
        if (player->voices[i].active) {
            key_off(&player->opl, player->voices[i].channel);
            player->voices[i].active = false;
        }
    }
}

void adlib_sfx_render(AdlibSfxPlayer *player, int16_t *buffer, int num_samples) {
    int pos = 0;
    while (pos < num_samples) {
        int remaining_in_tick = player->samples_per_tick - player->tick_counter;
        int to_render = num_samples - pos;
        if (to_render > remaining_in_tick) to_render = remaining_in_tick;

        opl2_render(&player->opl, buffer + pos, to_render);
        pos += to_render;
        player->tick_counter += to_render;

        if (player->tick_counter >= player->samples_per_tick) {
            player->tick_counter = 0;
            for (int i = 0; i < ADLIB_SFX_MAX_ACTIVE; i++)
                process_tick(player, &player->voices[i]);
        }
    }
}

bool adlib_sfx_is_playing(const AdlibSfxPlayer *player) {
    for (int i = 0; i < ADLIB_SFX_MAX_ACTIVE; i++)
        if (player->voices[i].active) return true;
    return false;
}
