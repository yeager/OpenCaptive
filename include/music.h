#ifndef MUSIC_H
#define MUSIC_H

#include "midi_player.h"
#include "sound.h"
#include "data_vfs.h"
#include <stdbool.h>

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
} MusicTrack;

typedef struct {
    MIDIPlayer  player;
    SoundSystem *sound;
    MusicTrack  current_track;
    const DataVFS *vfs;
    bool        enabled;
} MusicSystem;

bool music_init(MusicSystem *mus, SoundSystem *snd, const DataVFS *vfs);
void music_play(MusicSystem *mus, MusicTrack track);
void music_stop(MusicSystem *mus);
void music_update(MusicSystem *mus);
void music_shutdown(MusicSystem *mus);

#endif
