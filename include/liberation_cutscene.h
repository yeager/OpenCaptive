#ifndef LIBERATION_CUTSCENE_H
#define LIBERATION_CUTSCENE_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int frame;
    int total_frames;
    uint32_t seed;
    int mission;
    bool done;
} CutsceneState;

void cutscene_init(CutsceneState *cs, int mission, uint32_t seed);
void cutscene_tick(CutsceneState *cs);
void cutscene_render(const CutsceneState *cs, uint32_t *fb, int w, int h);

void endgame_init(CutsceneState *cs);
void endgame_render(const CutsceneState *cs, uint32_t *fb, int w, int h);

#endif
