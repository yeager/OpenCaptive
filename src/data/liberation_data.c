#include "liberation_data.h"
#include "sha256.h"
#include <stdlib.h>
#include <string.h>

/* Liberation: Captive II CD32 (Europe, Rev 3), data track 1. */
static const char cd32_track_sha256[] =
    "f807b1385c0996d54ed10afab271a7dd31d2c6dc6a18f13196ad2a79a0af8a80";
static const char game_binary_sha256[] =
    "9767c95dd0d134441d0de8906f9fbb8749f5b435e79a2ae5a021daf5b3337963";

bool liberation_data_open(LiberationData *data, const DataVFS *vfs) {
    memset(data, 0, sizeof(*data));
    data->disc_data = vfs_find_sha256(vfs, cd32_track_sha256, &data->disc_size);
    if (!data->disc_data || !iso_open_raw(&data->iso, data->disc_data, data->disc_size)) {
        liberation_data_close(data);
        return false;
    }

    ISOEntry entries[256];
    int count = iso_list_root(&data->iso, entries, 256);
    for (int i = 0; i < count; i++) {
        if (entries[i].is_dir) continue;
        uint8_t *file = iso_read_file(&data->iso, entries[i].lba, entries[i].size);
        if (!file) continue;
        uint8_t digest[32];
        sha256_digest(file, entries[i].size, digest);
        free(file);
        if (sha256_matches_hex(digest, game_binary_sha256)) {
            data->verified = true;
            return true;
        }
    }
    liberation_data_close(data);
    return false;
}

void liberation_data_close(LiberationData *data) {
    if (!data) return;
    free(data->disc_data);
    memset(data, 0, sizeof(*data));
}
