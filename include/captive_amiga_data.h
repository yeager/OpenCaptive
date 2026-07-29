#ifndef CAPTIVE_AMIGA_DATA_H
#define CAPTIVE_AMIGA_DATA_H

#include "data_vfs.h"
#include <stdbool.h>

/* Verifies the original Amiga Captive media through its disk-image hash and
 * the hashes of reconstructed OFS files. This does not select data by names. */
bool captive_amiga_data_verify(const DataVFS *vfs);

#endif
