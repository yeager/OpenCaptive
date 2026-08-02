#ifndef LIBERATION_SHOP_H
#define LIBERATION_SHOP_H

#include <stdbool.h>
#include <stdint.h>
#include "liberation_citygen.h"
#include "liberation_dialogue.h"

#define SHOP_MAX_ITEMS 32
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
    LibShopItem items[SHOP_MAX_ITEMS];
    unsigned item_count;
    uint16_t prng_seed;
    DialogueTree dialogue;
} LibShopState;

void lib_shop_init(LibShopState *shop, const CityBuilding *building, uint16_t seed);
void lib_shop_generate_inventory(LibShopState *shop);
bool lib_shop_buy_item(LibShopState *shop, unsigned item_idx, uint32_t *gold);
bool lib_shop_sell_item(LibShopState *shop, const char *name, uint16_t price,
                    uint16_t type, uint32_t *gold);

void lib_shop_generate_bar_menu(LibShopState *shop);
void lib_shop_generate_dialogue(LibShopState *shop, const char *npc_name);

#endif
