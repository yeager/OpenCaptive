#ifndef CDDA_PLAYER_H
#define CDDA_PLAYER_H

#include "sound.h"
#include <stdbool.h>
#include <stdint.h>

#define CDDA_MAX_TRACKS 11
#define CDDA_SAMPLE_RATE 44100
#define CDDA_CHANNELS 2
#define CDDA_SECTOR_SIZE 2352

typedef struct {
    SoundSystem *sound;
    uint8_t *track_data[CDDA_MAX_TRACKS];
    uint32_t track_size[CDDA_MAX_TRACKS];
    unsigned track_count;
    int current_track;
    uint32_t position;
    bool playing;
    bool looping;
    float volume;
    int stream_id;
} CDDAPlayer;

void cdda_init(CDDAPlayer *cd, SoundSystem *snd);
bool cdda_load_bin_cue(CDDAPlayer *cd, const char *bin_path, const char *cue_path);
bool cdda_load_track_file(CDDAPlayer *cd, unsigned track_num, const char *path);
bool cdda_load_track_raw(CDDAPlayer *cd, unsigned track_num,
                         const uint8_t *data, uint32_t size);
void cdda_play(CDDAPlayer *cd, unsigned track_num, bool loop);
void cdda_stop(CDDAPlayer *cd);
void cdda_set_volume(CDDAPlayer *cd, float volume);
void cdda_update(CDDAPlayer *cd);
void cdda_shutdown(CDDAPlayer *cd);

#endif
