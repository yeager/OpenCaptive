#include "midi_player.h"
#include <math.h>
#include <string.h>

static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}

static uint16_t read_be16(const uint8_t *p) {
    return ((uint16_t)p[0]<<8)|p[1];
}

static uint32_t read_vlq(const uint8_t *data, uint32_t *pos, uint32_t len) {
    uint32_t value = 0;
    while (*pos < len) {
        uint8_t b = data[(*pos)++];
        if (value > 0x01FFFFFFU) return 0xFFFFFFFFU;
        value = (value << 7) | (b & 0x7F);
        if (!(b & 0x80)) return value;
    }
    return 0xFFFFFFFFU;
}

static void recalc_tick_rate(MIDIPlayer *mp) {
    if (mp->ticks_per_beat == 0 || mp->tempo == 0) return;
    uint64_t us_per_tick = (uint64_t)mp->tempo / mp->ticks_per_beat;
    if (us_per_tick == 0) us_per_tick = 1;
    mp->samples_per_tick = (int)((uint64_t)mp->sample_rate * us_per_tick / 1000000);
    if (mp->samples_per_tick < 1) mp->samples_per_tick = 1;
}

static const int ch_op_off[9] = { 0, 1, 2, 8, 9, 10, 16, 17, 18 };

static void opl2_load_patch(OPL2 *opl, int ch, int patch_idx) {
    if (patch_idx < 0 || patch_idx >= CAPTIVE_OPL2_PATCH_COUNT || ch < 0 || ch >= 9)
        return;
    const uint8_t *p = captive_opl2_patches[patch_idx];
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
    opl2_write(opl, 0xC0 + ch,      p[0]);
}

static void opl2_note_on(OPL2 *opl, int ch, uint8_t note) {
    if (ch < 0 || ch >= 9) return;
    int fnum_idx = note & 0x7F;
    if (fnum_idx >= 128) fnum_idx = 127;
    uint16_t fnum = captive_opl2_ftable[fnum_idx];
    int block = (note >> 5) & 7;
    opl2_write(opl, 0xA0 + ch, fnum & 0xFF);
    opl2_write(opl, 0xB0 + ch, 0x20 | ((block & 7) << 2) | ((fnum >> 8) & 3));
}

static void opl2_note_off(OPL2 *opl, int ch) {
    if (ch < 0 || ch >= 9) return;
    uint8_t cur = opl->regs[0xB0 + ch];
    opl2_write(opl, 0xB0 + ch, cur & ~0x20);
}

static void opl2_set_volume(OPL2 *opl, int ch, uint8_t vol) {
    if (ch < 0 || ch >= 9) return;
    int off = ch_op_off[ch];
    uint8_t cur = opl->regs[0x40 + off + 3];
    uint8_t ksl = cur & 0xC0;
    uint8_t attenuation = 63 - ((vol * 63) / 127);
    opl2_write(opl, 0x40 + off + 3, ksl | (attenuation & 0x3F));
}

static uint8_t effective_channel_volume(const MIDIPlayer *mp, int channel) {
    if (!mp || channel < 0 || channel >= MIDI_MAX_CHANNELS ||
        !isfinite(mp->master_volume) || mp->master_volume <= 0.0f) return 0;
    float value = mp->channels[channel].volume * mp->master_volume;
    if (value >= 127.0f) return 127;
    if (value <= 0.0f) return 0;
    return (uint8_t)value;
}

static int alloc_voice(MIDIPlayer *mp, int midi_ch) {
    (void)midi_ch;
    int oldest_idx = -1;
    uint32_t oldest_age = 0;

    for (int i = 0; i < OPL2_VOICE_COUNT; i++) {
        if (!mp->voices[i].active) return i;
    }
    for (int i = 0; i < OPL2_VOICE_COUNT; i++) {
        if (mp->voices[i].age > oldest_age) {
            oldest_age = mp->voices[i].age;
            oldest_idx = i;
        }
    }
    if (oldest_idx >= 0) {
        opl2_note_off(&mp->opl, oldest_idx);
        mp->voices[oldest_idx].active = false;
    }
    return oldest_idx >= 0 ? oldest_idx : 0;
}

static void process_event(MIDIPlayer *mp, uint8_t status, const uint8_t *data,
                          int data_len) {
    uint8_t channel = status & 0x0F;
    uint8_t type = status & 0xF0;

    switch (type) {
    case MIDI_NOTE_ON:
        if (data_len >= 2 && data[1] > 0) {
            int v = alloc_voice(mp, channel);
            if (v < 0) break;
            int patch = mp->channels[channel].program;
            if (patch >= CAPTIVE_OPL2_PATCH_COUNT) patch = 0;
            opl2_load_patch(&mp->opl, v, patch);
            opl2_set_volume(&mp->opl, v, effective_channel_volume(mp, channel));
            opl2_note_on(&mp->opl, v, data[0]);
            mp->voices[v].midi_channel = channel;
            mp->voices[v].note = data[0];
            mp->voices[v].active = true;
            mp->voices[v].age = 0;
        } else if (data_len >= 1) {
            for (int i = 0; i < OPL2_VOICE_COUNT; i++) {
                if (mp->voices[i].active && mp->voices[i].note == data[0] &&
                    mp->voices[i].midi_channel == channel) {
                    opl2_note_off(&mp->opl, i);
                    mp->voices[i].active = false;
                }
            }
        }
        break;

    case MIDI_NOTE_OFF:
        if (data_len >= 1) {
            for (int i = 0; i < OPL2_VOICE_COUNT; i++) {
                if (mp->voices[i].active && mp->voices[i].note == data[0] &&
                    mp->voices[i].midi_channel == channel) {
                    opl2_note_off(&mp->opl, i);
                    mp->voices[i].active = false;
                }
            }
        }
        break;

    case MIDI_CONTROL:
        if (data_len >= 2) {
            if (data[0] == 7) {
                mp->channels[channel].volume = data[1];
            } else if (data[0] == 123) {
                for (int i = 0; i < OPL2_VOICE_COUNT; i++) {
                    if (mp->voices[i].midi_channel == channel) {
                        opl2_note_off(&mp->opl, i);
                        mp->voices[i].active = false;
                    }
                }
            }
        }
        break;

    case MIDI_PROGRAM:
        if (data_len >= 1) {
            mp->channels[channel].program = data[0];
        }
        break;

    default:
        break;
    }
}

static void process_track_tick(MIDIPlayer *mp, MIDITrack *trk) {
    while (!trk->ended && trk->next_tick <= mp->current_tick) {
        if (trk->pos >= trk->length) { trk->ended = true; break; }

        uint8_t b = trk->data[trk->pos];

        if (b == 0xFF) {
            /* System/meta events terminate running status in Standard MIDI. */
            trk->running_status = 0;
            trk->pos++;
            if (trk->pos >= trk->length) { trk->ended = true; break; }
            uint8_t meta_type = trk->data[trk->pos++];
            uint32_t meta_len = read_vlq(trk->data, &trk->pos, trk->length);
            if (meta_len == 0xFFFFFFFFU || meta_len > trk->length - trk->pos) {
                trk->ended = true;
                break;
            }

            if (meta_type == 0x51 && meta_len == 3 && trk->pos + 3 <= trk->length) {
                mp->tempo = ((uint32_t)trk->data[trk->pos] << 16) |
                            ((uint32_t)trk->data[trk->pos+1] << 8) |
                            trk->data[trk->pos+2];
                recalc_tick_rate(mp);
            } else if (meta_type == 0x2F) {
                trk->ended = true;
                break;
            }
            trk->pos += meta_len;
        } else if (b == 0xF0 || b == 0xF7) {
            trk->running_status = 0;
            trk->pos++;
            uint32_t sysex_len = read_vlq(trk->data, &trk->pos, trk->length);
            if (sysex_len == 0xFFFFFFFFU || sysex_len > trk->length - trk->pos) {
                trk->ended = true;
                break;
            }
            trk->pos += sysex_len;
        } else {
            uint8_t status;
            if (b & 0x80) {
                status = b;
                trk->running_status = status;
                trk->pos++;
            } else {
                status = trk->running_status;
            }

            uint8_t type = status & 0xF0;
            uint8_t param[2] = {0, 0};
            int params_needed = (type == MIDI_PROGRAM || type == MIDI_PRESSURE) ? 1 : 2;

            int params_read = 0;
            for (int i = 0; i < params_needed && trk->pos < trk->length; i++) {
                param[i] = trk->data[trk->pos++];
                params_read++;
            }

            process_event(mp, status, param, params_read);
        }

        if (trk->pos < trk->length && !trk->ended) {
            uint32_t delta = read_vlq(trk->data, &trk->pos, trk->length);
            if (delta == 0xFFFFFFFFU) {
                trk->ended = true;
                break;
            }
            trk->next_tick = mp->current_tick + delta;
        }
    }
}

bool midi_load(MIDIPlayer *player, const uint8_t *data, size_t size) {
    if (!player || !data) return false;
    memset(player, 0, sizeof(*player));
    player->file_data = data;
    player->file_size = size;
    player->tempo = 500000;
    player->sample_rate = 22050;

    if (size < 14 || memcmp(data, "MThd", 4) != 0) return false;

    uint32_t hdr_len = read_be32(data + 4);
    if (hdr_len < 6 || hdr_len > size - 8) return false;

    uint16_t num_tracks = read_be16(data + 10);
    player->ticks_per_beat = read_be16(data + 12);
    if (player->ticks_per_beat == 0) return false;

    if (num_tracks == 0 || num_tracks > MIDI_MAX_TRACKS) return false;

    uint32_t pos = 8 + hdr_len;
    for (int i = 0; i < num_tracks; i++) {
        if (size - pos < 8U || memcmp(data + pos, "MTrk", 4) != 0)
            return false;
        uint32_t trk_len = read_be32(data + pos + 4);
        pos += 8;
        if ((uint64_t)pos + trk_len > size) return false;

        player->tracks[i].data = data + pos;
        player->tracks[i].length = trk_len;
        player->tracks[i].pos = 0;
        player->tracks[i].next_tick = 0;
        player->tracks[i].running_status = 0;
        player->tracks[i].ended = false;

        uint32_t tpos = 0;
        player->tracks[i].next_tick = read_vlq(player->tracks[i].data, &tpos,
                                                player->tracks[i].length);
        if (player->tracks[i].next_tick == 0xFFFFFFFFU) return false;
        player->tracks[i].pos = tpos;
        player->num_tracks++;
        pos += trk_len;
    }

    for (int i = 0; i < MIDI_MAX_CHANNELS; i++) {
        player->channels[i].volume = 100;
    }
    player->master_volume = 1.0f;

    opl2_init(&player->opl);
    opl2_write(&player->opl, 0x01, 0x20);
    recalc_tick_rate(player);

    return player->num_tracks == num_tracks;
}

void midi_play(MIDIPlayer *player, bool loop) {
    if (!player) return;
    player->playing = true;
    player->looping = loop;
    player->current_tick = 0;
    player->tick_frac = 0;
    player->hq_previous_sample = 0;

    for (int i = 0; i < player->num_tracks; i++) {
        player->tracks[i].pos = 0;
        player->tracks[i].next_tick = 0;
        player->tracks[i].running_status = 0;
        player->tracks[i].ended = false;

        uint32_t tpos = 0;
        player->tracks[i].next_tick = read_vlq(player->tracks[i].data, &tpos,
                                                player->tracks[i].length);
        if (player->tracks[i].next_tick == 0xFFFFFFFFU) {
            player->tracks[i].ended = true;
            continue;
        }
        player->tracks[i].pos = tpos;
    }

    opl2_init(&player->opl);
    opl2_write(&player->opl, 0x01, 0x20);
    memset(player->voices, 0, sizeof(player->voices));
}

void midi_stop(MIDIPlayer *player) {
    if (!player) return;
    player->playing = false;
    player->hq_previous_sample = 0;
    for (int i = 0; i < OPL2_VOICE_COUNT; i++) {
        opl2_note_off(&player->opl, i);
        player->voices[i].active = false;
    }
}

void midi_set_volume(MIDIPlayer *player, float vol) {
    if (!player || !isfinite(vol)) return;
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    player->master_volume = vol;
    for (int i = 0; i < OPL2_VOICE_COUNT; i++) {
        if (!player->voices[i].active) continue;
        int channel = player->voices[i].midi_channel;
        opl2_set_volume(&player->opl, i, effective_channel_volume(player, channel));
    }
}

void midi_set_sample_rate(MIDIPlayer *player, int sample_rate) {
    if (!player || sample_rate <= 0) return;
    player->sample_rate = sample_rate;
    recalc_tick_rate(player);
}

void midi_set_high_quality(MIDIPlayer *player, bool enabled) {
    if (!player) return;
    player->high_quality = enabled;
    player->hq_previous_sample = 0;
}

void midi_render(MIDIPlayer *player, int16_t *buffer, int num_samples) {
    if (!player || !buffer || num_samples <= 0) return;
    if (player->samples_per_tick <= 0) {
        memset(buffer, 0, (size_t)num_samples * sizeof(*buffer));
        return;
    }
    if (!player->playing) {
        memset(buffer, 0, num_samples * sizeof(int16_t));
        return;
    }
    if (player->tick_frac < 0 || player->tick_frac >= player->samples_per_tick) {
        /* A stale/corrupt public state must not make remaining negative and
         * prevent the render loop from consuming samples. */
        player->tick_frac = 0;
    }

    int pos = 0;
    int16_t raw[256];
    while (pos < num_samples) {
        int remaining = player->samples_per_tick - player->tick_frac;
        int to_render = num_samples - pos;
        if (to_render > remaining) to_render = remaining;

        int chunk = to_render > (int)(sizeof(raw) / sizeof(raw[0]))
            ? (int)(sizeof(raw) / sizeof(raw[0])) : to_render;
        opl2_render(&player->opl, raw, chunk);
        if (player->high_quality) {
            for (int i = 0; i < chunk; ++i) {
                /* A short causal low-pass removes quantization stair steps
                 * from the software OPL2 output without changing its timing. */
                int32_t filtered = ((int32_t)player->hq_previous_sample +
                                    (int32_t)raw[i] * 3) / 4;
                if (filtered > INT16_MAX) filtered = INT16_MAX;
                if (filtered < INT16_MIN) filtered = INT16_MIN;
                buffer[pos + i] = (int16_t)filtered;
                player->hq_previous_sample = raw[i];
            }
        } else {
            memcpy(buffer + pos, raw, (size_t)chunk * sizeof(raw[0]));
        }
        pos += chunk;
        player->tick_frac += chunk;

        if (player->tick_frac >= player->samples_per_tick) {
            player->tick_frac = 0;
            player->current_tick++;

            for (int i = 0; i < OPL2_VOICE_COUNT; i++) {
                if (player->voices[i].active) player->voices[i].age++;
            }

            for (int t = 0; t < player->num_tracks; t++) {
                process_track_tick(player, &player->tracks[t]);
            }

            bool all_ended = true;
            for (int t = 0; t < player->num_tracks; t++) {
                if (!player->tracks[t].ended) { all_ended = false; break; }
            }
            if (all_ended) {
                if (player->looping) {
                    midi_play(player, true);
                } else {
                    player->playing = false;
                    memset(buffer + pos, 0, (num_samples - pos) * sizeof(int16_t));
                    return;
                }
            }
        }
    }
}
