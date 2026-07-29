#ifndef LIBERATION_DATA_H
#define LIBERATION_DATA_H

#include "data_vfs.h"
#include "iso9660_reader.h"
#include "liberation_anim.h"
#include <stdbool.h>

typedef struct {
    uint8_t *disc_data;
    size_t disc_size;
    ISOImage iso;
    bool verified;
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
    LIBERATION_RESOURCE_COUNT,
} LiberationResource;

bool liberation_data_open(LiberationData *data, const DataVFS *vfs);
void liberation_data_close(LiberationData *data);
uint8_t *liberation_data_read(const LiberationData *data,
                              LiberationResource resource, size_t *out_size);

#endif
