#ifndef CAPTIVE_SCENE_ASSETS_H
#define CAPTIVE_SCENE_ASSETS_H

/* Every source surface observed in Captive's first-person renderer is named
 * by content identity.  Array position is deliberately not a media filename
 * or archive-entry contract. */
#define CAPTIVE_VIEW_SOURCE_COUNT 23

extern const char *const captive_view_source_hashes[CAPTIVE_VIEW_SOURCE_COUNT];

#endif
