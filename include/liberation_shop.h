#ifndef LIBERATION_SHOP_H
#define LIBERATION_SHOP_H

#include <stdbool.h>
#include <stdint.h>
#include "liberation_citygen.h"
#include "liberation_dialogue.h"
#include "inventory.h"

#define LIB_SHOP_MAX_ITEMS 32
#define SHOP_MAX_NAME 64

typedef struct {
    char name[32];
    uint16_t price;
    uint16_t item_type;
    uint8_t  quantity;
    uint8_t  quality;
} LibShopItem;

typedef struct {
    char shop_name[SHOP_MAX_NAME];
    BuildingType building_type;
    LibShopItem items[LIB_SHOP_MAX_ITEMS];
    unsigned item_count;
    uint16_t prng_seed;
    DialogueTree dialogue;
} LibShopState;

void lib_shop_init(LibShopState *shop, const CityBuilding *building, uint16_t seed);
/* item_db supplies the authentic Captive item names for the real item IDs the
 * shop stocks; pass NULL only where no database is available (the item then
 * carries its numeric id as a factual label rather than an invented name). */
void lib_shop_generate_inventory(LibShopState *shop, const ItemDatabase *item_db);
bool lib_shop_buy_item(LibShopState *shop, unsigned item_idx, uint32_t *gold);
bool lib_shop_sell_item(LibShopState *shop, const char *name, uint16_t price,
                    uint16_t type, uint32_t *gold);

void lib_shop_generate_bar_menu(LibShopState *shop);
void lib_shop_generate_dialogue(LibShopState *shop, const char *npc_name);

#endif
