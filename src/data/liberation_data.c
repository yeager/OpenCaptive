#include "liberation_data.h"
#include "rnc_decoder.h"
#include "sha256.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define LIBERATION_DATA_MAGIC UINT32_C(0x4C444154) /* "LDAT" */

/* Liberation: Captive II CD32 (Europe, Rev 3), data track 1. */
static const char cd32_track_sha256[] =
    "f807b1385c0996d54ed10afab271a7dd31d2c6dc6a18f13196ad2a79a0af8a80";
static const char presentation_bundle_sha256[] =
    "1d3a335d254c0eae919a712dd73bd41b24ed897bf145ed118ccf2277baa7a35f";
/* These identify decompressed FORM/ANIM payloads, never their position in a
   CD image or the name by which an archive happens to store them. */
static const char intro_presentation_sha256[] =
    "e7e35f1b491fafd95da260abcb1c1c402140601840c5f98ae7282069fd30b269";
static const char city_presentation_sha256[] =
    "b94a450c12428af9a22b8bb8c31fca74cdc2b2bd3be3dc9c7a1eadd7e6576101";
/* CD32 resource content hashes. */
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
    /* 1836 bytes each; verified against the CD32 disc image. */
    [LIBERATION_RESOURCE_FONT_0] =
        "c9a86d8912e1b99198589eb7f96cda948dee36659c6fae819fd2a9c8c3970a20",
    [LIBERATION_RESOURCE_FONT_1] =
        "8856f947e6cc680c58afa81c065fe408e9fc552d90ded2c627b4b243f9915b12",
};

/* Amiga OCS/AGA floppy resource hashes.  CityGen and PlotGen are byte-
   identical across platforms; the game binary, text tables, and mission
   menu differ. */
static const char *const amiga_resource_sha256[LIBERATION_RESOURCE_COUNT] = {
    [LIBERATION_RESOURCE_GAME_BINARY] =
        "c9472ad779b4c48f0210627296ef238b61ec19718421d24808c3319e754cc6de",
    [LIBERATION_RESOURCE_CITY_GENERATOR] =
        "e54540c3bf8dfaf569380a135ac039f1438e9efb85cf6d5e3e487e25d4c7c13e",
    [LIBERATION_RESOURCE_PLOT_GENERATOR] =
        "bc9c922801661eb66024d0bcf822c03e38ffea7f3576693e0512692ccf6d6705",
    [LIBERATION_RESOURCE_PLOT_TEXT] =
        "8134f8b1dc241a92d22963ce12401b7fa82799655db569fcb36b5e01d01a8b34",
    [LIBERATION_RESOURCE_CITY_TEXT] =
        "8d2b0082bcb98df7f64a55a51635da1f146c02beea2a6722b5b57a19561d6734",
    [LIBERATION_RESOURCE_DIALOGUE_TEXT] =
        "4dbdbd20f26b6f4c8cfd637f5b917d71876e1130ef0282ead4c1a0dde41ecced",
    [LIBERATION_RESOURCE_MISSION_MENU] =
        "9664753e362deccbcd1f4a55e4c63ae4a6763de620306373a275fb7052b4e11b",
};

/* Red Book audio tracks 2-11 of the CD32 disc (Europe, Rev 3), in disc order.
 * Verified against the original disc image; track 1 is the data track above. */
static const char *const cdda_track_sha256[LIBERATION_CDDA_TRACK_COUNT] = {
    "ceded68026add084e467f750f40297d46dc7e2ce493fb3f855e956acebd1f59c",
    "527bd7f2d00f7e1f1d33593e697541b70bc215c4cc11b6bbe6ad6cf42b81ea58",
    "64994dc80fa2cf425d0827f919ab3e078cf04cdd7a4b357e7fe01b6439b471ed",
    "196f28c68ba35703530179311fa9f41b2ba749ef402722dcfdb157aaf6a03fac",
    "3fa8f0be60271f4738f448515fc433749622d71066895f7611c46b766efe5040",
    "f4afd5eebd2f7128c6bfeb67212323b5ca27abb6feead6786c203c2d85c00ac0",
    "b5301553fd2a082c2134570af25aec3fffceef816b1c6e90b8c3b0127225aaf3",
    "6708d78f1d990009272eaa31602f917bafb709a31b6bceb3c3f2ef1649782ccc",
    "25be0db8612c832728423c6894388bf0b79c35ba7916ac8c54b0f8f6f18eb524",
    "359ece35a1f05f682bea0b5b6dfa280bd628254ab90271ca1cec5b75e4d766f6",
};

const char *liberation_cdda_track_sha256(unsigned index) {
    if (index >= LIBERATION_CDDA_TRACK_COUNT) return NULL;
    return cdda_track_sha256[index];
}

static uint32_t read_be32(const uint8_t *data) {
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | data[3];
}

static void load_optional_presentation_frame(const uint8_t *rnc, size_t rnc_size,
                                             const char expected_sha256[65],
                                             LiberationAnimFrame *frame,
                                             LiberationAnimScript *script) {
    if (!rnc || rnc_size < 12U || !expected_sha256 || !frame || !script) return;
    uint32_t raw_size = read_be32(rnc + 4U);
    uint32_t packed_size = read_be32(rnc + 8U);
    if (memcmp(rnc, "RNC\2", 4) != 0 || raw_size == 0U ||
        raw_size > 16U * 1024U * 1024U ||
        packed_size > rnc_size - 12U ||
        packed_size > (uint32_t)(INT_MAX - 12)) return;
    uint8_t *form = malloc(raw_size);
    if (!form) return;
    if (rnc_decode(rnc, (int)(packed_size + 12U), form, (int)raw_size) == (int)raw_size) {
        uint8_t digest[32];
        sha256_digest(form, raw_size, digest);
        if (sha256_matches_hex(digest, expected_sha256)) {
            liberation_anim_decode_first_frame(form, raw_size, frame);
            liberation_anim_extract_script(form, raw_size, script);
        }
    }
    free(form);
}

static void load_optional_presentation_frames(LiberationData *data) {
    size_t bundle_size = 0;
    uint8_t *bundle = iso_read_file_sha256(&data->iso, presentation_bundle_sha256,
                                            &bundle_size);
    if (!bundle) return;
    /* The original bundle is a byte stream of RNC2 resources.  Its physical
       offsets vary between dump layouts, so scan it and accept only forms
       whose decompressed SHA-256 equals our content identities. */
    for (size_t offset = 0; offset + 12U <= bundle_size; ++offset) {
        const uint8_t *rnc = bundle + offset;
        if (memcmp(rnc, "RNC\2", 4) != 0) continue;
        uint32_t packed_size = read_be32(rnc + 8U);
        if (packed_size > bundle_size - offset - 12U) continue;
        if (!data->intro_frame.bitplanes)
            load_optional_presentation_frame(rnc, bundle_size - offset,
                                             intro_presentation_sha256,
                                             &data->intro_frame, &data->intro_script);
        if (!data->city_frame.bitplanes)
            load_optional_presentation_frame(rnc, bundle_size - offset,
                                             city_presentation_sha256,
                                             &data->city_frame, &data->city_script);
        /* A valid RNC payload cannot contain a second top-level resource.
           Skip past it rather than examining compression bytes as headers. */
        offset += (size_t)packed_size + 11U;
    }
    free(bundle);
}

/* Decode the authentic UI font once at open.  FONT_0 is an optional resource,
 * so a source without a recovered font hash (the Amiga floppies) simply leaves
 * ui_font_loaded false and callers keep their fallback. */
static void load_optional_ui_font(LiberationData *data) {
    data->ui_font_loaded = false;
    size_t size = 0;
    uint8_t *font = liberation_data_read(data, LIBERATION_RESOURCE_FONT_0, &size);
    if (!font) return;
    if (fnt_open(&data->ui_font, font, size))
        data->ui_font_loaded = true;
    free(font);
}

/* Parse the authentic building/location description table (DTE).  The resource
 * is stored as raw text, so it is parsed directly.  lib_text_table_parse keeps
 * pointers into the buffer, so the decoded data is retained for the table's
 * lifetime and freed in liberation_data_close. */
static void load_descriptions(LiberationData *data) {
    data->descriptions_loaded = false;
    size_t size = 0;
    uint8_t *text = liberation_data_read(data, LIBERATION_RESOURCE_DIALOGUE_TEXT,
                                         &size);
    if (!text) return;
    if (lib_text_table_parse(&data->descriptions, text, size)) {
        data->descriptions_data = text;
        data->descriptions_loaded = true;
    } else {
        free(text);
    }
}

static bool liberation_cd32_available(const DataVFS *vfs) {
    size_t size = 0;
    uint8_t *track = vfs_find_sha256(vfs, cd32_track_sha256, &size);
    if (!track) return false;
    ISOImage iso;
    bool ok = iso_open_raw(&iso, track, size);
    if (ok) {
        for (int i = 0; i < LIBERATION_RESOURCE_REQUIRED_COUNT; ++i) {
            uint8_t *file = iso_read_file_sha256(&iso, resource_sha256[i], NULL);
            if (!file) { ok = false; break; }
            free(file);
        }
    }
    free(track);
    return ok;
}

static bool liberation_amiga_available(const DataVFS *vfs) {
    for (int i = 0; i < LIBERATION_RESOURCE_REQUIRED_COUNT; ++i) {
        uint8_t *file = vfs_find_sha256(vfs, amiga_resource_sha256[i], NULL);
        if (!file) return false;
        free(file);
    }
    return true;
}

unsigned liberation_data_available_sources(const DataVFS *vfs) {
    if (!vfs) return 0;
    unsigned sources = 0;
    if (liberation_cd32_available(vfs)) sources |= 1U << LIBERATION_SOURCE_CD32;
    if (liberation_amiga_available(vfs)) sources |= 1U << LIBERATION_SOURCE_AMIGA_ADF;
    return sources;
}

bool liberation_data_open_source(LiberationData *data, const DataVFS *vfs,
                                 LiberationSource preferred) {
    if (!data) return false;
    if (data->lifecycle_magic == LIBERATION_DATA_MAGIC)
        liberation_data_close(data);
    else
        memset(data, 0, sizeof(*data));
    if (!vfs) {
        return false;
    }
    data->lifecycle_magic = LIBERATION_DATA_MAGIC;
    data->vfs = vfs;

    bool try_cd32 = preferred == LIBERATION_SOURCE_NONE || preferred == LIBERATION_SOURCE_CD32;
    bool try_amiga = preferred == LIBERATION_SOURCE_NONE || preferred == LIBERATION_SOURCE_AMIGA_ADF;

    /* Try the explicitly selected source first. */
    if (try_cd32)
        data->disc_data = vfs_find_sha256(vfs, cd32_track_sha256, &data->disc_size);
    if (try_cd32 && data->disc_data && iso_open_raw(&data->iso, data->disc_data, data->disc_size)) {
        bool all_found = true;
        for (int i = 0; i < LIBERATION_RESOURCE_REQUIRED_COUNT; i++) {
            uint8_t *file = iso_read_file_sha256(&data->iso, resource_sha256[i], NULL);
            if (!file) { all_found = false; break; }
            free(file);
        }
        if (all_found) {
            data->verified = true;
            data->source = LIBERATION_SOURCE_CD32;
            load_optional_presentation_frames(data);
            load_optional_ui_font(data);
            load_descriptions(data);
            return true;
        }
    }

    /* Fall back to Amiga ADF: resources found directly via VFS SHA-256
     * (the VFS now searches inside ADF disk images in ZIP archives) */
    if (data->disc_data) { free(data->disc_data); data->disc_data = NULL; }
    memset(&data->iso, 0, sizeof(data->iso));

    if (!try_amiga) {
        liberation_data_close(data);
        return false;
    }
    int found = 0;
    for (int i = 0; i < LIBERATION_RESOURCE_REQUIRED_COUNT; i++) {
        uint8_t *file = vfs_find_sha256(vfs, amiga_resource_sha256[i], NULL);
        if (file) { found++; free(file); }
    }

    if (found == LIBERATION_RESOURCE_REQUIRED_COUNT) {
        data->verified = true;
        data->source = LIBERATION_SOURCE_AMIGA_ADF;
        load_optional_ui_font(data);
        load_descriptions(data);
        return true;
    }

    liberation_data_close(data);
    return false;
}

bool liberation_data_open(LiberationData *data, const DataVFS *vfs) {
    return liberation_data_open_source(data, vfs, LIBERATION_SOURCE_NONE);
}

uint8_t *liberation_data_read(const LiberationData *data,
                              LiberationResource resource, size_t *out_size) {
    if (out_size) *out_size = 0;
    if (!data || !data->verified || resource < 0 ||
        resource >= LIBERATION_RESOURCE_COUNT) return NULL;
    if (data->source == LIBERATION_SOURCE_AMIGA_ADF && data->vfs)
        return vfs_find_sha256(data->vfs, amiga_resource_sha256[resource], out_size);
    return iso_read_file_sha256(&data->iso, resource_sha256[resource], out_size);
}

void liberation_data_close(LiberationData *data) {
    if (!data) return;
    if (data->lifecycle_magic != LIBERATION_DATA_MAGIC) {
        memset(data, 0, sizeof(*data));
        return;
    }
    free(data->disc_data);
    free(data->descriptions_data);
    liberation_anim_frame_free(&data->city_frame);
    liberation_anim_frame_free(&data->intro_frame);
    liberation_anim_script_free(&data->city_script);
    liberation_anim_script_free(&data->intro_script);
    memset(data, 0, sizeof(*data));
}

/* String tables moved to liberation_string_tables.c */
