#ifndef INVENTORY_H
#define INVENTORY_H

#include <stdint.h>
#include <stdbool.h>

// Item categories
typedef enum {
    ITEM_NONE = 0,
    ITEM_WEAPON_MELEE,
    ITEM_WEAPON_HANDGUN,
    ITEM_WEAPON_RIFLE,
    ITEM_WEAPON_AUTO,
    ITEM_WEAPON_LASER,
    ITEM_WEAPON_CANNON,
    ITEM_WEAPON_SPRAY,
    ITEM_AMMO,
    ITEM_ARMOR_HEAD,
    ITEM_ARMOR_CHEST,
    ITEM_ARMOR_ARM,
    ITEM_ARMOR_LEG,
    ITEM_ARMOR_FOOT,
    ITEM_ARMOR_HAND,
    ITEM_DEVSCAPE,
    ITEM_OPTIC,
    ITEM_BATTERY,
    ITEM_GOLD,
    ITEM_KEY,
    ITEM_MAP,
    ITEM_CLIPBOARD,
    ITEM_DROID_CHIP,
    ITEM_CAMERA,
    ITEM_PROBE,
    ITEM_MINE,
    ITEM_DIE,
    ITEM_EXPLOSIVE,
    ITEM_CATEGORY_COUNT,
} ItemCategory;

typedef struct {
    uint8_t  id;
    ItemCategory category;
    char     name[24];
    int16_t  damage_min;
    int16_t  damage_max;
    int16_t  defense;
    uint8_t  range;
    uint8_t  tier;        // 0-7 quality tier
    uint16_t ammo;
    uint16_t ammo_max;
    uint16_t price;
    float    weight;
    uint8_t  skill_req;   // skill points needed
} Item;

#define MAX_ITEM_DEFS 128

typedef struct {
    Item defs[MAX_ITEM_DEFS];
    int  num_defs;
} ItemDatabase;

void item_db_init(ItemDatabase *db);
const Item *item_db_get(const ItemDatabase *db, uint8_t id);
int  item_db_find_by_category(const ItemDatabase *db, ItemCategory cat,
                               const Item **out, int max_out);

#endif
