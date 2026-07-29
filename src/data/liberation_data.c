#include "liberation_data.h"
#include "sha256.h"
#include <stdlib.h>
#include <string.h>

/* Liberation: Captive II CD32 (Europe, Rev 3), data track 1. */
static const char cd32_track_sha256[] =
    "f807b1385c0996d54ed10afab271a7dd31d2c6dc6a18f13196ad2a79a0af8a80";
static const char *const resource_sha256[LIBERATION_RESOURCE_COUNT] = {
    [LIBERATION_RESOURCE_GAME_BINARY] =
        "db61f7e39fd31ac19b82216ea963711728d25518454fae42fd89c5bab52f2215",
    [LIBERATION_RESOURCE_CITY_GENERATOR] =
        "e54540c3bf8dfaf569380a135ac039f1438e9efb85cf6d5e3e487e25d4c7c13e",
    [LIBERATION_RESOURCE_PLOT_GENERATOR] =
        "bc9c922801661eb66024d0bcf822c03e38ffea7f3576693e0512692ccf6d6705",
    [LIBERATION_RESOURCE_PLOT_TEXT] =
        "884d4124fa1ab600a4f7dd889df160779eda8c62e13af1d0280ac9aad681818c",
    [LIBERATION_RESOURCE_CITY_TEXT] =
        "99f7bd75794a7b4f3e94eeef9c61b756da938d862bb83339b140c18d02eb79c5",
    [LIBERATION_RESOURCE_DIALOGUE_TEXT] =
        "e154d250c1acdbed66835bb356a699efdb6f9f8b5e6d586ca07080414610a94c",
};

bool liberation_data_open(LiberationData *data, const DataVFS *vfs) {
    memset(data, 0, sizeof(*data));
    data->disc_data = vfs_find_sha256(vfs, cd32_track_sha256, &data->disc_size);
    if (!data->disc_data || !iso_open_raw(&data->iso, data->disc_data, data->disc_size)) {
        liberation_data_close(data);
        return false;
    }

    for (int i = 0; i < LIBERATION_RESOURCE_COUNT; i++) {
        uint8_t *file = iso_read_file_sha256(&data->iso, resource_sha256[i], NULL);
        if (!file) {
            liberation_data_close(data);
            return false;
        }
        free(file);
    }
    data->verified = true;
    return true;
}

uint8_t *liberation_data_read(const LiberationData *data,
                              LiberationResource resource, size_t *out_size) {
    if (out_size) *out_size = 0;
    if (!data || !data->verified || resource < 0 ||
        resource >= LIBERATION_RESOURCE_COUNT) return NULL;
    return iso_read_file_sha256(&data->iso, resource_sha256[resource], out_size);
}

void liberation_data_close(LiberationData *data) {
    if (!data) return;
    free(data->disc_data);
    memset(data, 0, sizeof(*data));
}
