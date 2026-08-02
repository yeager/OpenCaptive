#ifndef MUSIC_H
#define MUSIC_H

#include "midi_player.h"
#include "sound.h"
#include "data_vfs.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MUSIC_NONE,
    MUSIC_TITLE,
    MUSIC_BASE,
    MUSIC_BATTLE,
    MUSIC_SHOP,
    MUSIC_HOLOMAP,
    MUSIC_ESCAPE,
    MUSIC_FINAL,
    MUSIC_TRAPPED,
    MUSIC_FCBASE,
    MUSIC_VCBASE,
    MUSIC_LONGNT,
    MUSIC_W,
    MUSIC_COMPROOM,
    MUSIC_RUNNING,
    MUSIC_TRACK_COUNT,
} MusicTrack;

typedef struct {
    MIDIPlayer  player;
    SoundSystem *sound;
    MusicTrack  current_track;
    MusicTrack  requested_track;
    const DataVFS *vfs;
    uint8_t     *owned_data;
    uint32_t    prng_state;
    bool        enabled;
} MusicSystem;

bool music_init(MusicSystem *mus, SoundSystem *snd, const DataVFS *vfs);
void music_set_enabled(MusicSystem *mus, bool enabled);
void music_play(MusicSystem *mus, MusicTrack track);
void music_stop(MusicSystem *mus);
void music_update(MusicSystem *mus);
void music_shutdown(MusicSystem *mus);

#endif
