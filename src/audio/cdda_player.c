#include "cdda_player.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void cdda_init(CDDAPlayer *cd, SoundSystem *snd) {
    if (!cd) return;
    memset(cd, 0, sizeof(*cd));
    cd->sound = snd;
    cd->current_track = -1;
    cd->volume = 1.0f;
    cd->stream_id = -1;
}

static bool parse_cue_track_offsets(const char *cue_path,
                                     uint32_t *offsets, unsigned *count) {
    FILE *f = fopen(cue_path, "r");
    if (!f) return false;
    char line[256];
    unsigned cur_track = 0;
    *count = 0;
    while (fgets(line, sizeof(line), f)) {
        unsigned tn;
        if (sscanf(line, " TRACK %u AUDIO", &tn) == 1) {
            cur_track = tn;
        }
        unsigned mm, ss, ff;
        if (sscanf(line, " INDEX 01 %u:%u:%u", &mm, &ss, &ff) == 3) {
            if (cur_track > 0 && cur_track <= CDDA_MAX_TRACKS + 1) {
                uint32_t sector = mm * 60 * 75 + ss * 75 + ff;
                offsets[cur_track - 1] = sector * CDDA_SECTOR_SIZE;
                if (cur_track > *count) *count = cur_track;
            }
        }
    }
    fclose(f);
    return *count > 1;
}

bool cdda_load_bin_cue(CDDAPlayer *cd, const char *bin_path, const char *cue_path) {
    if (!cd || !bin_path || !cue_path) return false;

    uint32_t offsets[CDDA_MAX_TRACKS + 1];
    unsigned total_tracks = 0;
    memset(offsets, 0, sizeof(offsets));

    if (!parse_cue_track_offsets(cue_path, offsets, &total_tracks))
        return false;

    FILE *f = fopen(bin_path, "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);

    unsigned audio_count = 0;
    for (unsigned i = 1; i < total_tracks && audio_count < CDDA_MAX_TRACKS; i++) {
        uint32_t start = offsets[i];
        uint32_t end = (i + 1 < total_tracks) ? offsets[i + 1] : (uint32_t)file_size;
        if (end <= start || start >= (uint32_t)file_size) continue;
        uint32_t size = end - start;

        uint8_t *data = malloc(size);
        if (!data) continue;
        fseek(f, start, SEEK_SET);
        if (fread(data, 1, size, f) != size) {
            free(data);
            continue;
        }

        cd->track_data[audio_count] = data;
        cd->track_size[audio_count] = size;
        audio_count++;
    }

    fclose(f);
    cd->track_count = audio_count;
    return audio_count > 0;
}

bool cdda_load_track_file(CDDAPlayer *cd, unsigned track_num, const char *path) {
    if (!cd || track_num >= CDDA_MAX_TRACKS || !path) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size <= 0) { fclose(f); return false; }
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc((size_t)size);
    if (!data) { fclose(f); return false; }
    if (fread(data, 1, (size_t)size, f) != (size_t)size) {
        free(data); fclose(f); return false;
    }
    fclose(f);
    free(cd->track_data[track_num]);
    cd->track_data[track_num] = data;
    cd->track_size[track_num] = (uint32_t)size;
    if (track_num >= cd->track_count)
        cd->track_count = track_num + 1;
    return true;
}

bool cdda_load_track_raw(CDDAPlayer *cd, unsigned track_num,
                         const uint8_t *data, uint32_t size) {
    if (!cd || track_num >= CDDA_MAX_TRACKS || !data || size == 0)
        return false;
    free(cd->track_data[track_num]);
    cd->track_data[track_num] = malloc(size);
    if (!cd->track_data[track_num]) return false;
    memcpy(cd->track_data[track_num], data, size);
    cd->track_size[track_num] = size;
    if (track_num >= cd->track_count)
        cd->track_count = track_num + 1;
    return true;
}

void cdda_play(CDDAPlayer *cd, unsigned track_num, bool loop) {
    if (!cd || !cd->sound || track_num >= cd->track_count ||
        !cd->track_data[track_num]) return;
    cdda_stop(cd);
    cd->current_track = (int)track_num;
    cd->position = 0;
    cd->playing = true;
    cd->looping = loop;
}

void cdda_stop(CDDAPlayer *cd) {
    if (!cd) return;
    cd->playing = false;
    cd->current_track = -1;
    cd->position = 0;
}

void cdda_set_volume(CDDAPlayer *cd, float volume) {
    if (!cd) return;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    cd->volume = volume;
}

void cdda_update(CDDAPlayer *cd) {
    if (!cd || !cd->sound || !cd->playing || cd->current_track < 0)
        return;

    unsigned t = (unsigned)cd->current_track;
    if (t >= cd->track_count || !cd->track_data[t]) {
        cd->playing = false;
        return;
    }

    uint32_t chunk = 4096;
    if (cd->position + chunk > cd->track_size[t]) {
        chunk = cd->track_size[t] - cd->position;
    }

    if (chunk >= 4) {
        const int16_t *pcm = (const int16_t *)(cd->track_data[t] + cd->position);
        uint32_t samples = chunk / 4;
        int sid = sound_load_raw(cd->sound, (const int8_t *)pcm,
                                 samples * 4, CDDA_SAMPLE_RATE);
        if (sid >= 0)
            sound_play(cd->sound, sid, cd->volume, 1.0f);
    }

    cd->position += chunk;
    if (cd->position >= cd->track_size[t]) {
        if (cd->looping)
            cd->position = 0;
        else
            cd->playing = false;
    }
}

void cdda_shutdown(CDDAPlayer *cd) {
    if (!cd) return;
    cdda_stop(cd);
    for (unsigned i = 0; i < CDDA_MAX_TRACKS; i++) {
        free(cd->track_data[i]);
        cd->track_data[i] = NULL;
    }
    cd->track_count = 0;
}
