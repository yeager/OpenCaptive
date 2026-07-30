#include "liberation_data.h"
#include "rnc_decoder.h"
#include "sha256.h"
#include <stdlib.h>
#include <string.h>

/* Liberation: Captive II CD32 (Europe, Rev 3), data track 1. */
static const char cd32_track_sha256[] =
    "f807b1385c0996d54ed10afab271a7dd31d2c6dc6a18f13196ad2a79a0af8a80";
static const char presentation_bundle_sha256[] =
    "1d3a335d254c0eae919a712dd73bd41b24ed897bf145ed118ccf2277baa7a35f";
#define CITY_ANIM_RNC_OFFSET 386824U
#define INTRO_ANIM_RNC_OFFSET 590U
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
    [LIBERATION_RESOURCE_MISSION_MENU] =
        "d6bb0dd9c578beb8e84ddf9f458f0be43ec158b2b261491d023e972d2812c2d2",
};

static uint32_t read_be32(const uint8_t *data) {
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | data[3];
}

static void load_optional_presentation_frame(const uint8_t *bundle, size_t bundle_size,
                                             size_t offset, LiberationAnimFrame *frame,
                                             LiberationAnimScript *script) {
    if (!bundle || bundle_size < offset + 12U) return;
    const uint8_t *rnc = bundle + offset;
    uint32_t raw_size = read_be32(rnc + 4U);
    uint32_t packed_size = read_be32(rnc + 8U);
    if (memcmp(rnc, "RNC\2", 4) != 0 || raw_size == 0U ||
        raw_size > 16U * 1024U * 1024U ||
        packed_size > bundle_size - offset - 12U) return;
    uint8_t *form = malloc(raw_size);
    if (!form) return;
    if (rnc_decode(rnc, (int)(packed_size + 12U), form, (int)raw_size) == (int)raw_size) {
        liberation_anim_decode_first_frame(form, raw_size, frame);
        liberation_anim_extract_script(form, raw_size, script);
    }
    free(form);
}

static void load_optional_presentation_frames(LiberationData *data) {
    size_t bundle_size = 0;
    uint8_t *bundle = iso_read_file_sha256(&data->iso, presentation_bundle_sha256,
                                            &bundle_size);
    if (!bundle) return;
    load_optional_presentation_frame(bundle, bundle_size, INTRO_ANIM_RNC_OFFSET,
                                     &data->intro_frame, &data->intro_script);
    load_optional_presentation_frame(bundle, bundle_size, CITY_ANIM_RNC_OFFSET,
                                     &data->city_frame, &data->city_script);
    free(bundle);
}

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
    load_optional_presentation_frames(data);
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
    liberation_anim_frame_free(&data->city_frame);
    liberation_anim_frame_free(&data->intro_frame);
    liberation_anim_script_free(&data->city_script);
    liberation_anim_script_free(&data->intro_script);
    memset(data, 0, sizeof(*data));
}
