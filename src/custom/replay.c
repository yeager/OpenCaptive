#include "custom_features.h"
#include <stdio.h>
#include <string.h>

#define REPLAY_MAGIC 0x4F435250 /* "OCRP" */
#define REPLAY_VERSION 1

void replay_init(ReplaySystem *rs) {
    memset(rs, 0, sizeof(*rs));
}

void replay_record_input(ReplaySystem *rs, uint32_t tick, uint8_t action, uint8_t param) {
    if (!rs->recording || rs->count >= REPLAY_MAX_INPUTS) return;
    rs->inputs[rs->count].tick = tick;
    rs->inputs[rs->count].action = action;
    rs->inputs[rs->count].param = param;
    rs->count++;
}

bool replay_save(const ReplaySystem *rs, const char *path) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return false;

    uint32_t magic = REPLAY_MAGIC;
    uint32_t version = REPLAY_VERSION;
    uint32_t count = (uint32_t)rs->count;

    fwrite(&magic, 4, 1, fp);
    fwrite(&version, 4, 1, fp);
    fwrite(&rs->seed, 4, 1, fp);
    fwrite(&count, 4, 1, fp);

    for (int i = 0; i < rs->count; i++) {
        fwrite(&rs->inputs[i].tick, 4, 1, fp);
        fwrite(&rs->inputs[i].action, 1, 1, fp);
        fwrite(&rs->inputs[i].param, 1, 1, fp);
    }

    fclose(fp);
    return true;
}

bool replay_load(ReplaySystem *rs, const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;

    uint32_t magic, version, count;
    if (fread(&magic, 4, 1, fp) != 1 || magic != REPLAY_MAGIC) {
        fclose(fp);
        return false;
    }
    if (fread(&version, 4, 1, fp) != 1 || version != REPLAY_VERSION) {
        fclose(fp);
        return false;
    }
    fread(&rs->seed, 4, 1, fp);
    fread(&count, 4, 1, fp);

    if (count > REPLAY_MAX_INPUTS) count = REPLAY_MAX_INPUTS;
    rs->count = (int)count;
    rs->playback_pos = 0;
    rs->playing = true;
    rs->recording = false;

    for (int i = 0; i < rs->count; i++) {
        fread(&rs->inputs[i].tick, 4, 1, fp);
        fread(&rs->inputs[i].action, 1, 1, fp);
        fread(&rs->inputs[i].param, 1, 1, fp);
    }

    fclose(fp);
    return true;
}

const ReplayInput *replay_next(ReplaySystem *rs, uint32_t tick) {
    if (!rs->playing || rs->playback_pos >= rs->count) return NULL;
    if (rs->inputs[rs->playback_pos].tick <= tick) {
        return &rs->inputs[rs->playback_pos++];
    }
    return NULL;
}
