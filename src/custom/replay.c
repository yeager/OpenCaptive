#include "custom_features.h"
#include <stdio.h>
#include <string.h>

#define REPLAY_MAGIC 0x4F435250 /* "OCRP" */
#define REPLAY_VERSION 1

void replay_init(ReplaySystem *rs) {
    if (!rs) return;
    memset(rs, 0, sizeof(*rs));
}

void replay_record_input(ReplaySystem *rs, uint32_t tick, uint8_t action, uint8_t param) {
    if (!rs) return;
    if (!rs->recording || rs->count >= REPLAY_MAX_INPUTS) return;
    rs->inputs[rs->count].tick = tick;
    rs->inputs[rs->count].action = action;
    rs->inputs[rs->count].param = param;
    rs->count++;
}

bool replay_save(const ReplaySystem *rs, const char *path) {
    if (!rs || !path || rs->count < 0 || rs->count > REPLAY_MAX_INPUTS) return false;
    for (int i = 1; i < rs->count; i++) {
        if (rs->inputs[i].tick < rs->inputs[i - 1].tick) return false;
    }
    FILE *fp = fopen(path, "wb");
    if (!fp) return false;

    uint32_t magic = REPLAY_MAGIC;
    uint32_t version = REPLAY_VERSION;
    uint32_t count = (uint32_t)rs->count;

    if (fwrite(&magic, 4, 1, fp) != 1 ||
        fwrite(&version, 4, 1, fp) != 1 ||
        fwrite(&rs->seed, 4, 1, fp) != 1 ||
        fwrite(&count, 4, 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    for (int i = 0; i < rs->count; i++) {
        if (fwrite(&rs->inputs[i].tick, 4, 1, fp) != 1 ||
            fwrite(&rs->inputs[i].action, 1, 1, fp) != 1 ||
            fwrite(&rs->inputs[i].param, 1, 1, fp) != 1) {
            fclose(fp);
            return false;
        }
    }

    return fclose(fp) == 0;
}

bool replay_load(ReplaySystem *rs, const char *path) {
    if (!rs || !path) return false;
    ReplaySystem *destination = rs;
    /* Loading replaces the complete replay state; do not read an
     * uninitialised caller-provided object just to build the rollback copy. */
    ReplaySystem restored = {0};
    rs = &restored;
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
    uint32_t seed;
    if (fread(&seed, 4, 1, fp) != 1 || fread(&count, 4, 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    if (count > REPLAY_MAX_INPUTS) {
        fclose(fp);
        return false;
    }
    long payload_start = ftell(fp);
    if (payload_start < 0 || fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return false;
    }
    long file_end = ftell(fp);
    long long expected_size = (long long)payload_start + (long long)count * 6LL;
    if (file_end < 0 || (long long)file_end != expected_size ||
        fseek(fp, payload_start, SEEK_SET) != 0) {
        fclose(fp);
        return false;
    }
    rs->count = (int)count;
    rs->seed = seed;
    rs->playback_pos = 0;
    rs->playing = true;
    rs->recording = false;

    uint32_t previous_tick = 0;
    for (int i = 0; i < rs->count; i++) {
        if (fread(&rs->inputs[i].tick, 4, 1, fp) != 1 ||
            fread(&rs->inputs[i].action, 1, 1, fp) != 1 ||
            fread(&rs->inputs[i].param, 1, 1, fp) != 1) {
            rs->count = 0;
            rs->playing = false;
            fclose(fp);
            return false;
        }
        if (i > 0 && rs->inputs[i].tick < previous_tick) {
            rs->count = 0;
            rs->playing = false;
            fclose(fp);
            return false;
        }
        previous_tick = rs->inputs[i].tick;
    }

    fclose(fp);
    *destination = restored;
    return true;
}

const ReplayInput *replay_next(ReplaySystem *rs, uint32_t tick) {
    if (!rs || !rs->playing || rs->playback_pos < 0 ||
        rs->count < 0 || rs->count > REPLAY_MAX_INPUTS ||
        rs->playback_pos >= rs->count) return NULL;
    if (rs->inputs[rs->playback_pos].tick <= tick) {
        return &rs->inputs[rs->playback_pos++];
    }
    return NULL;
}
