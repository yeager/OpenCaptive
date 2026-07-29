#include "midi_player.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
        value = (value << 7) | (b & 0x7F);
        if (!(b & 0x80)) break;
    }
    return value;
}

// Note frequency table (A4 = 440 Hz)
static float note_freq(uint8_t note) {
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

bool midi_load(MIDIPlayer *player, const uint8_t *data, size_t size) {
    memset(player, 0, sizeof(*player));
    player->file_data = data;
    player->file_size = size;
    player->volume = 0.5f;
    player->tempo = 500000; // 120 BPM default

    if (size < 14) return false;
    if (memcmp(data, "MThd", 4) != 0) return false;

    uint32_t hdr_len = read_be32(data + 4);
    if (hdr_len < 6) return false;

    uint16_t format = read_be16(data + 8);
    uint16_t num_tracks = read_be16(data + 10);
    player->ticks_per_beat = read_be16(data + 12);

    if (num_tracks > MIDI_MAX_TRACKS) num_tracks = MIDI_MAX_TRACKS;
    (void)format;

    uint32_t pos = 8 + hdr_len;
    for (int i = 0; i < num_tracks && pos + 8 <= size; i++) {
        if (memcmp(data + pos, "MTrk", 4) != 0) break;
        uint32_t trk_len = read_be32(data + pos + 4);
        pos += 8;

        player->tracks[i].data = data + pos;
        player->tracks[i].length = trk_len;
        player->tracks[i].pos = 0;
        player->tracks[i].next_tick = 0;
        player->tracks[i].running_status = 0;
        player->tracks[i].ended = false;

        // Read first delta time
        uint32_t tpos = 0;
        player->tracks[i].next_tick = read_vlq(player->tracks[i].data, &tpos,
                                                player->tracks[i].length);
        player->tracks[i].pos = tpos;
        player->num_tracks++;

        pos += trk_len;
    }

    // Initialize channels
    for (int i = 0; i < MIDI_MAX_CHANNELS; i++) {
        player->channels[i].volume = 100;
        player->channels[i].pan = 64;
        player->channels[i].pitch_bend = 0x2000;
    }

    return player->num_tracks > 0;
}

static MIDIVoice *find_free_voice(MIDIPlayer *player) {
    for (int i = 0; i < 32; i++) {
        if (!player->voices[i].active) return &player->voices[i];
    }
    // Steal oldest releasing voice
    for (int i = 0; i < 32; i++) {
        if (player->voices[i].releasing) return &player->voices[i];
    }
    return &player->voices[0]; // steal first
}

static void process_event(MIDIPlayer *player, uint8_t status, const uint8_t *data,
                          uint32_t data_len) {
    uint8_t channel = status & 0x0F;
    uint8_t type = status & 0xF0;

    switch (type) {
        case MIDI_NOTE_ON:
            if (data_len >= 2 && data[1] > 0) {
                MIDIVoice *v = find_free_voice(player);
                v->note = data[0];
                v->freq = note_freq(data[0]);
                v->velocity = data[1] / 127.0f;
                v->phase = 0.0f;
                v->env = 1.0f;
                v->env_rate = 0.0f;
                v->active = true;
                v->releasing = false;
            } else if (data_len >= 1) {
                // Note on with velocity 0 = note off
                for (int i = 0; i < 32; i++) {
                    if (player->voices[i].active && player->voices[i].note == data[0]) {
                        player->voices[i].releasing = true;
                        player->voices[i].env_rate = 0.005f;
                    }
                }
            }
            break;

        case MIDI_NOTE_OFF:
            if (data_len >= 1) {
                for (int i = 0; i < 32; i++) {
                    if (player->voices[i].active && player->voices[i].note == data[0]) {
                        player->voices[i].releasing = true;
                        player->voices[i].env_rate = 0.005f;
                    }
                }
            }
            break;

        case MIDI_CONTROL:
            if (data_len >= 2) {
                switch (data[0]) {
                    case 7: player->channels[channel].volume = data[1]; break;
                    case 10: player->channels[channel].pan = data[1]; break;
                    case 123: // All notes off
                        for (int i = 0; i < 32; i++) {
                            player->voices[i].releasing = true;
                            player->voices[i].env_rate = 0.01f;
                        }
                        break;
                }
            }
            break;

        case MIDI_PROGRAM:
            if (data_len >= 1) {
                player->channels[channel].program = data[0];
            }
            break;

        case MIDI_PITCH_BEND:
            if (data_len >= 2) {
                player->channels[channel].pitch_bend = data[0] | (data[1] << 7);
            }
            break;

        default:
            break;
    }
}

static void process_track_tick(MIDIPlayer *player, MIDITrack *trk) {
    while (!trk->ended && trk->next_tick <= player->current_tick) {
        if (trk->pos >= trk->length) { trk->ended = true; break; }

        uint8_t b = trk->data[trk->pos];

        if (b == 0xFF) {
            // Meta event
            trk->pos++;
            if (trk->pos >= trk->length) { trk->ended = true; break; }
            uint8_t meta_type = trk->data[trk->pos++];
            uint32_t meta_len = read_vlq(trk->data, &trk->pos, trk->length);

            if (meta_type == 0x51 && meta_len == 3 && trk->pos + 3 <= trk->length) {
                player->tempo = ((uint32_t)trk->data[trk->pos] << 16) |
                               ((uint32_t)trk->data[trk->pos+1] << 8) |
                               trk->data[trk->pos+2];
            } else if (meta_type == 0x2F) {
                trk->ended = true;
                break;
            }
            trk->pos += meta_len;
        } else if (b == 0xF0 || b == 0xF7) {
            // SysEx
            trk->pos++;
            uint32_t sysex_len = read_vlq(trk->data, &trk->pos, trk->length);
            trk->pos += sysex_len;
        } else {
            // Channel event
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
            int params_needed;

            switch (type) {
                case MIDI_PROGRAM:
                case MIDI_PRESSURE:
                    params_needed = 1;
                    break;
                default:
                    params_needed = 2;
                    break;
            }

            for (int i = 0; i < params_needed && trk->pos < trk->length; i++) {
                param[i] = trk->data[trk->pos++];
            }

            process_event(player, status, param, params_needed);
        }

        // Read next delta time
        if (trk->pos < trk->length && !trk->ended) {
            uint32_t delta = read_vlq(trk->data, &trk->pos, trk->length);
            trk->next_tick = player->current_tick + delta;
        }
    }
}

void midi_play(MIDIPlayer *player, bool loop) {
    player->playing = true;
    player->looping = loop;
    player->current_tick = 0;
    player->tick_accum = 0.0f;

    // Reset tracks
    for (int i = 0; i < player->num_tracks; i++) {
        player->tracks[i].pos = 0;
        player->tracks[i].next_tick = 0;
        player->tracks[i].running_status = 0;
        player->tracks[i].ended = false;

        uint32_t tpos = 0;
        player->tracks[i].next_tick = read_vlq(player->tracks[i].data, &tpos,
                                                player->tracks[i].length);
        player->tracks[i].pos = tpos;
    }

    memset(player->voices, 0, sizeof(player->voices));
}

void midi_stop(MIDIPlayer *player) {
    player->playing = false;
    memset(player->voices, 0, sizeof(player->voices));
}

void midi_set_volume(MIDIPlayer *player, float vol) {
    player->volume = vol;
}

// Simple square wave synth
static float synth_sample(float phase) {
    return (phase < 0.5f) ? 0.5f : -0.5f;
}

void midi_render(MIDIPlayer *player, int16_t *buffer, int num_samples) {
    if (!player->playing) {
        memset(buffer, 0, num_samples * sizeof(int16_t));
        return;
    }

    float tps = (float)player->ticks_per_beat * 1000000.0f /
                (float)player->tempo / MIDI_SAMPLE_RATE;

    for (int s = 0; s < num_samples; s++) {
        // Advance MIDI tick counter
        player->tick_accum += tps;
        while (player->tick_accum >= 1.0f) {
            player->tick_accum -= 1.0f;
            player->current_tick++;

            for (int t = 0; t < player->num_tracks; t++) {
                process_track_tick(player, &player->tracks[t]);
            }
        }

        // Check if all tracks ended
        bool all_ended = true;
        for (int t = 0; t < player->num_tracks; t++) {
            if (!player->tracks[t].ended) { all_ended = false; break; }
        }
        if (all_ended) {
            if (player->looping) {
                midi_play(player, true);
            } else {
                player->playing = false;
                memset(buffer + s, 0, (num_samples - s) * sizeof(int16_t));
                return;
            }
        }

        // Mix voices
        float mix = 0.0f;
        for (int v = 0; v < 32; v++) {
            MIDIVoice *voice = &player->voices[v];
            if (!voice->active) continue;

            float sample = synth_sample(voice->phase);
            sample *= voice->velocity * voice->env * 0.15f;
            mix += sample;

            voice->phase += voice->freq / MIDI_SAMPLE_RATE;
            if (voice->phase >= 1.0f) voice->phase -= 1.0f;

            if (voice->releasing) {
                voice->env -= voice->env_rate;
                if (voice->env <= 0.0f) {
                    voice->active = false;
                    voice->env = 0.0f;
                }
            }
        }

        mix *= player->volume;
        if (mix > 1.0f) mix = 1.0f;
        if (mix < -1.0f) mix = -1.0f;
        buffer[s] = (int16_t)(mix * 32000.0f);
    }
}
