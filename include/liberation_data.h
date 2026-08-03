#ifndef LIBERATION_DATA_H
#define LIBERATION_DATA_H

#include "data_vfs.h"
#include "iso9660_reader.h"
#include "liberation_anim.h"
#include <stdbool.h>

typedef enum {
    LIBERATION_SOURCE_NONE,
    LIBERATION_SOURCE_CD32,
    LIBERATION_SOURCE_AMIGA_ADF,
} LiberationSource;

typedef struct {
    uint8_t *disc_data;
    size_t disc_size;
    ISOImage iso;
    bool verified;
    LiberationSource source;
    const DataVFS *vfs;
    LiberationAnimFrame city_frame;
    LiberationAnimFrame intro_frame;
    LiberationAnimScript city_script;
    LiberationAnimScript intro_script;
} LiberationData;

typedef enum {
    LIBERATION_RESOURCE_GAME_BINARY,
    LIBERATION_RESOURCE_CITY_GENERATOR,
    LIBERATION_RESOURCE_PLOT_GENERATOR,
    LIBERATION_RESOURCE_PLOT_TEXT,
    LIBERATION_RESOURCE_CITY_TEXT,
    LIBERATION_RESOURCE_DIALOGUE_TEXT,
    /* 320x109 AMOS mission-selection composition, sprite index zero. */
    LIBERATION_RESOURCE_MISSION_MENU,
    LIBERATION_RESOURCE_COUNT,
} LiberationResource;

bool liberation_data_open(LiberationData *data, const DataVFS *vfs);
void liberation_data_close(LiberationData *data);
uint8_t *liberation_data_read(const LiberationData *data,
                              LiberationResource resource, size_t *out_size);

/* String tables from the verified CD32 executable.
 * All tables are direct transcriptions from the binary. */
#define LIBERATION_CITY_SYLLABLE_COUNT 32
extern const char *const liberation_city_syllables[LIBERATION_CITY_SYLLABLE_COUNT];

#define LIBERATION_STREET_TYPE_COUNT 20
extern const char *const liberation_street_types[LIBERATION_STREET_TYPE_COUNT];

#define LIBERATION_FIRST_NAME_COUNT 35
extern const char *const liberation_first_names[LIBERATION_FIRST_NAME_COUNT];

#define LIBERATION_LAST_NAME_COUNT 32
extern const char *const liberation_last_names[LIBERATION_LAST_NAME_COUNT];

#define LIBERATION_NPC_TITLE_COUNT 8
extern const char *const liberation_npc_titles[LIBERATION_NPC_TITLE_COUNT];

#define LIBERATION_SHOP_TYPE_COUNT 9
extern const char *const liberation_shop_types[LIBERATION_SHOP_TYPE_COUNT];

#define LIBERATION_BAR_TYPE_COUNT 12
extern const char *const liberation_bar_types[LIBERATION_BAR_TYPE_COUNT];

#define LIBERATION_BUSINESS_TYPE_COUNT 11
extern const char *const liberation_business_types[LIBERATION_BUSINESS_TYPE_COUNT];

#define LIBERATION_INDUSTRIAL_TYPE_COUNT 12
extern const char *const liberation_industrial_types[LIBERATION_INDUSTRIAL_TYPE_COUNT];

#endif
