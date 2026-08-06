#ifndef SHOP_H
#define SHOP_H

#include "game_state.h"
#include "inventory.h"

#define SHOP_MAX_ITEMS 20
#define SHOP_VISIBLE_ITEMS 12

typedef struct {
    uint8_t item_ids[SHOP_MAX_ITEMS];
    int     num_items;
    int     selected;
    int     gold;    // party gold
    bool    active;
} ShopState;

void shop_init(ShopState *shop, const ItemDatabase *db, int level, uint32_t seed);
int shop_next_selection(const ShopState *shop);
void shop_render(const ShopState *shop, const ItemDatabase *db,
                 uint32_t *pixels, int width, int height);
bool shop_buy(ShopState *shop, const ItemDatabase *db, GameState *gs);
bool shop_repair(ShopState *shop, GameState *gs, int droid_idx);

#endif
