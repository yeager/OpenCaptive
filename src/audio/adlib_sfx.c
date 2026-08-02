#include "adlib_sfx.h"
#include <string.h>

/* SFX bytecode interpreter recovered from CAP_A.BIN (SHA-256: d77feb1b...).
 *
 * Voice struct layout (0x1C bytes each, 4 voices at 0xAB8/0xAD4/0xAF0/0xB0C):
 *   [di+0x00] word: active flag (0=inactive, else active)
 *   [di+0x02] word: delay counter (decremented per tick)
 *   [di+0x04] word: sequence pointer (PC)
 *   [di+0x05] byte: OPL2 channel base
 *   [di+0x08] byte: note offset (added to note values)
 *   [di+0x0E] word: saved delay for note-on
 *   [di+0x0F] byte: key-on flag (0xFF = playing)
 *   [di+0x10] byte: current note value
 *   [di+0x12] word: loop table pointer
 *   [di+0x14] word: loop counter (decremented on 0xFF)
 *   [di+0x16] word: subroutine flag (1 = in subroutine)
 *   [di+0x18] word: subroutine return address
 *
 * Opcodes (byte 0 is opcode, bytes 1+ are operands):
 *   0x80 NN DD - key on note NN+offset, delay DD ticks (3 bytes)
 *   0x81 DD    - set delay DD ticks (2 bytes)
 *   0x82 VV    - set volume VV on OPL2 registers (2 bytes)
 *   0x83 NN    - set note offset to NN (2 bytes)
 *   0x84 PP    - load instrument patch PP (2 bytes)
 *   0x85 DD    - set delay DD, jump to delay path (2 bytes)
 *   0x86 AAAA  - call subroutine at address AAAA (3 bytes)
 *   0x87       - return from subroutine (1 byte)
 *   0x88 DD    - key on with PRNG note, delay DD (2 bytes)
 *   0x89 MM    - set delay to (PRNG & MM) + 1 (2 bytes)
 *   0x8A AAAA  - jump to address AAAA (3 bytes)
 *   0xC8       - key off (1 byte)
 *   0xFF       - end/loop: advance loop pointer, dec counter, stop if 0
 *
 * Driver runs at ~70 Hz (DOS timer tick, INT 08h chain). */

#define SFX_TICK_RATE 70

static uint16_t sfx_prng(AdlibSfxVoice *v) {
    uint16_t s = v->prng_state;
    s ^= (s << 13);
    s ^= (s >> 9);
    s ^= (s << 7);
    v->prng_state = s;
    return s;
}

static void load_patch(OPL2 *opl, int ch, int patch_idx) {
    if (patch_idx < 0 || patch_idx >= CAPTIVE_OPL2_PATCH_COUNT) return;

    const uint8_t *p = captive_opl2_patches[patch_idx];
    static const int ch_op_off[9] = { 0, 1, 2, 8, 9, 10, 16, 17, 18 };
    if (ch < 0 || ch >= 9) return;
    int off = ch_op_off[ch];

    opl2_write(opl, 0x20 + off,     p[2]);
    opl2_write(opl, 0x20 + off + 3, p[3]);
    opl2_write(opl, 0x40 + off,     p[4]);
    opl2_write(opl, 0x40 + off + 3, p[5]);
    opl2_write(opl, 0x60 + off,     p[6]);
    opl2_write(opl, 0x60 + off + 3, p[7]);
    opl2_write(opl, 0x80 + off,     p[8]);
    opl2_write(opl, 0x80 + off + 3, p[9]);
    opl2_write(opl, 0xE0 + off,     p[10]);
    opl2_write(opl, 0xE0 + off + 3, p[11]);
    opl2_write(opl, 0xC0 + ch,      0x00);
}

static void key_on(OPL2 *opl, int ch, uint8_t note) {
    if (ch < 0 || ch >= 9) return;
    int fnum_idx = note & 0x7F;
    if (fnum_idx >= 128) fnum_idx = 127;

    uint16_t fnum = captive_opl2_ftable[fnum_idx];
    int block = (note >> 5) & 7;

    opl2_write(opl, 0xA0 + ch, fnum & 0xFF);
    opl2_write(opl, 0xB0 + ch, 0x20 | ((block & 7) << 2) | ((fnum >> 8) & 3));
}

static void key_off(OPL2 *opl, int ch) {
    if (ch < 0 || ch >= 9) return;
    uint8_t cur = opl->regs[0xB0 + ch];
    opl2_write(opl, 0xB0 + ch, cur & ~0x20);
}

static void set_volume(OPL2 *opl, int ch, uint8_t vol) {
    if (ch < 0 || ch >= 9) return;
    static const int ch_op_off[9] = { 0, 1, 2, 8, 9, 10, 16, 17, 18 };
    int off = ch_op_off[ch];
    opl2_write(opl, 0x40 + off + 3, vol);
}

static void process_tick(AdlibSfxPlayer *player, AdlibSfxVoice *v) {
    if (!v->active) return;
    if (v->delay > 0) {
        v->delay--;
        return;
    }

    int iterations = 0;
    while (v->pos < v->size && iterations < 256) {
        iterations++;
        uint8_t op = v->data[v->pos];

        switch (op) {
        case 0x80:
            if (v->pos + 2 >= v->size) goto end;
            {
                uint8_t note = v->data[v->pos + 1] + v->note_offset;
                v->delay = v->data[v->pos + 2];
                v->current_note = note;
                v->key_playing = true;
                key_on(&player->opl, v->channel, note);
                v->pos += 3;
            }
            return;

        case 0x81:
            if (v->pos + 1 >= v->size) goto end;
            v->delay = v->data[v->pos + 1];
            v->pos += 2;
            return;

        case 0x82:
            if (v->pos + 1 >= v->size) goto end;
            set_volume(&player->opl, v->channel, v->data[v->pos + 1]);
            v->pos += 2;
            continue;

        case 0x83:
            if (v->pos + 1 >= v->size) goto end;
            v->note_offset = v->data[v->pos + 1];
            v->pos += 2;
            continue;

        case 0x84:
            if (v->pos + 1 >= v->size) goto end;
            {
                int patch = v->data[v->pos + 1];
                if (patch >= 0x80) patch -= 0x80;
                load_patch(&player->opl, v->channel, patch);
            }
            v->pos += 2;
            continue;

        case 0x85:
            if (v->pos + 1 >= v->size) goto end;
            v->delay = v->data[v->pos + 1];
            v->pos += 2;
            return;

        case 0x86:
            if (v->pos + 2 >= v->size) goto end;
            {
                uint16_t addr = v->data[v->pos + 1] | (v->data[v->pos + 2] << 8);
                v->return_addr = v->pos + 3;
                v->in_subroutine = true;
                if (addr < v->size)
                    v->pos = addr;
                else
                    goto end;
            }
            continue;

        case 0x87:
            if (v->in_subroutine) {
                v->in_subroutine = false;
                v->pos = v->return_addr;
            } else {
                v->pos++;
            }
            continue;

        case 0x88:
            if (v->pos + 1 >= v->size) goto end;
            {
                uint8_t note = (uint8_t)(sfx_prng(v) & 0x7F) + v->note_offset;
                v->delay = v->data[v->pos + 1];
                v->current_note = note;
                v->key_playing = true;
                key_on(&player->opl, v->channel, note);
                v->pos += 2;
            }
            return;

        case 0x89:
            if (v->pos + 1 >= v->size) goto end;
            v->delay = (sfx_prng(v) & v->data[v->pos + 1]) + 1;
            v->pos += 2;
            return;

        case 0x8A:
            if (v->pos + 2 >= v->size) goto end;
            {
                uint16_t addr = v->data[v->pos + 1] | (v->data[v->pos + 2] << 8);
                if (addr < v->size)
                    v->pos = addr;
                else
                    goto end;
            }
            continue;

        case 0xC8:
            key_off(&player->opl, v->channel);
            v->key_playing = false;
            v->pos++;
            continue;

        case 0xFF:
            if (v->loop_counter > 0) {
                v->loop_counter--;
                if (v->loop_counter > 0 && v->loop_table_ptr + 1 < v->size) {
                    uint16_t addr = v->data[v->loop_table_ptr] |
                                    (v->data[v->loop_table_ptr + 1] << 8);
                    v->loop_table_ptr += 2;
                    if (addr < v->size) {
                        v->pos = addr;
                        continue;
                    }
                }
            }
            goto end;

        default:
            v->pos++;
            continue;
        }
    }

end:
    v->active = false;
    if (v->key_playing) {
        key_off(&player->opl, v->channel);
        v->key_playing = false;
    }
}

void adlib_sfx_init(AdlibSfxPlayer *player, int sample_rate) {
    memset(player, 0, sizeof(*player));
    opl2_init(&player->opl);
    opl2_write(&player->opl, 0x01, 0x20);
    player->samples_per_tick = sample_rate / SFX_TICK_RATE;
    if (player->samples_per_tick < 1) player->samples_per_tick = 1;
}

bool adlib_sfx_play(AdlibSfxPlayer *player, int sfx_id) {
    if (sfx_id < 0 || sfx_id >= CAPTIVE_SFX_COUNT) return false;

    int slot = -1;
    for (int i = 0; i < ADLIB_SFX_MAX_ACTIVE; i++) {
        if (!player->voices[i].active) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        slot = 0;
        key_off(&player->opl, player->voices[0].channel);
    }

    AdlibSfxVoice *v = &player->voices[slot];
    memset(v, 0, sizeof(*v));
    v->data = captive_sfx_table[sfx_id].data;
    v->size = captive_sfx_table[sfx_id].size;
    v->channel = slot;
    v->active = true;
    v->prng_state = 0xACE1;

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
