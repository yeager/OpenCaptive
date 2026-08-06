#ifndef CAPTIVE_SCENE_ASSETS_H
#define CAPTIVE_SCENE_ASSETS_H

/* Every source surface observed in Captive's first-person renderer is named
 * by content identity.  Array position is deliberately not a media filename
 * or archive-entry contract. */
#define CAPTIVE_VIEW_SOURCE_COUNT 23
#define CAPTIVE_BOOT_SOURCE_COUNT 2
#define CAPTIVE_REQUIRED_SOURCE_COUNT \
    (CAPTIVE_BOOT_SOURCE_COUNT + CAPTIVE_VIEW_SOURCE_COUNT)

#include "data_vfs.h"

extern const char *const captive_view_source_hashes[CAPTIVE_VIEW_SOURCE_COUNT];
extern const char *const captive_boot_source_hashes[CAPTIVE_BOOT_SOURCE_COUNT];

bool captive_scene_assets_available(const DataVFS *vfs);
bool captive_data_available(const DataVFS *vfs);
const char *captive_required_source_hash(size_t index);

#endif
