#ifndef LIBERATION_DATA_H
#define LIBERATION_DATA_H

#include "data_vfs.h"
#include "iso9660_reader.h"
#include <stdbool.h>

typedef struct {
    uint8_t *disc_data;
    size_t disc_size;
    ISOImage iso;
    bool verified;
} LiberationData;

bool liberation_data_open(LiberationData *data, const DataVFS *vfs);
void liberation_data_close(LiberationData *data);

#endif
