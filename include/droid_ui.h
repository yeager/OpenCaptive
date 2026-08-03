#ifndef DROID_UI_H
#define DROID_UI_H

#include "game_state.h"
#include "inventory.h"
#include <stdbool.h>

typedef enum {
    DROID_UI_EQUIP,
    DROID_UI_INVENTORY,
} DroidUIMode;

typedef struct {
    int droid_idx;
    DroidUIMode ui_mode;
    int cursor;
    bool active;
} DroidUIState;

void droid_ui_init(DroidUIState *ui, int droid_idx);
void droid_recalc_weapon_damage(Droid *d, const ItemDatabase *db);
void droid_ui_render(const DroidUIState *ui, const GameState *gs,
                     const ItemDatabase *db, uint32_t *pixels, int width, int height);
bool droid_ui_handle_key(DroidUIState *ui, GameState *gs, const ItemDatabase *db, int key);

#endif
