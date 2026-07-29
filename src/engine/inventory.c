#include "inventory.h"
#include <string.h>

// Item database populated from research data
// Weapons: {id, category, name, dmg_min, dmg_max, def, range, tier, ammo, ammo_max, price, weight, skill}
static const Item weapon_defs[] = {
    // Melee (range 1)
    { 1, ITEM_WEAPON_MELEE, "KNUCKLE-DUSTER",  10,  15, 0, 1, 0, 0,  0,   100, 0.4f, 1},
    { 2, ITEM_WEAPON_MELEE, "BATTLE-GLOVE",    20,  30, 0, 1, 1, 0,  0,   200, 0.6f, 2},
    { 3, ITEM_WEAPON_MELEE, "WAR-BLADE",       30,  60, 0, 1, 2, 0,  0,   400, 0.9f, 3},
    { 4, ITEM_WEAPON_MELEE, "LIGHT-BLADE",     50, 100, 0, 2, 3, 0,  0,   800, 4.3f, 4},
    { 5, ITEM_WEAPON_MELEE, "FIRE-AXE",        80, 150, 0, 2, 4, 0,  0,  1200, 6.1f, 5},

    // Handguns (range 6)
    {10, ITEM_WEAPON_HANDGUN, "20MM-PISTOL",   100, 120, 0, 6, 0, 30, 30,  999, 1.5f, 1},
    {11, ITEM_WEAPON_HANDGUN, "45MM-PISTOL",   140, 180, 0, 6, 1, 34, 34, 1500, 2.0f, 2},
    {12, ITEM_WEAPON_HANDGUN, "50MM-PISTOL",   180, 250, 0, 6, 2, 40, 40, 2000, 2.5f, 3},

    // Rifles (range 15)
    {20, ITEM_WEAPON_RIFLE, "45MM-RIFLE",      300, 400, 0, 15, 0, 34, 34, 2000, 3.0f, 1},
    {21, ITEM_WEAPON_RIFLE, "CARTRIDGE-RIFLE", 350, 450, 0, 15, 1, 42, 42, 2500, 3.2f, 2},
    {22, ITEM_WEAPON_RIFLE, "50MM-RIFLE",      400, 490, 0, 15, 2, 40, 40, 3000, 3.5f, 3},

    // Automatics (range 12)
    {30, ITEM_WEAPON_AUTO, "20MM-AUTO",        300, 380, 0, 12, 0, 39, 39, 2500, 2.2f, 1},
    {31, ITEM_WEAPON_AUTO, "45MM-AUTO",        350, 450, 0, 12, 1, 46, 46, 3000, 2.3f, 2},
    {32, ITEM_WEAPON_AUTO, "50MM-AUTO",        400, 570, 0, 12, 2, 50, 50, 3500, 2.4f, 3},
    {33, ITEM_WEAPON_AUTO, "BOOSTER",          450, 600, 0, 12, 3, 50, 50, 4001, 2.5f, 4},

    // Lasers (range 30)
    {40, ITEM_WEAPON_LASER, "HAND-LASER",      900,1100, 0, 30, 0, 20, 20, 3000, 2.9f, 1},
    {41, ITEM_WEAPON_LASER, "ION-PULSE",      1200,1500, 0, 30, 1, 25, 25, 4000, 3.2f, 2},

    // Cannons (range 50)
    {50, ITEM_WEAPON_CANNON, "MONO-CANNON",   1500,1800, 0, 50, 0, 20, 20, 4000, 16.0f, 1},
    {51, ITEM_WEAPON_CANNON, "A51-LAUNCHER",  1800,2000, 0, 50, 1, 15, 15, 5000, 19.0f, 2},
    {52, ITEM_WEAPON_CANNON, "TWIN-CANNON",   2000,2200, 0, 50, 2, 20, 20, 6000, 20.2f, 3},

    // Sprayguns (range 45)
    {60, ITEM_WEAPON_SPRAY, "AIROSOLL",        100, 300, 0, 45, 0, 21, 21, 2000, 4.0f, 1},
    {61, ITEM_WEAPON_SPRAY, "ACID-DISPERSER",  300, 500, 0, 45, 1, 10, 10, 3000, 4.5f, 2},
    {62, ITEM_WEAPON_SPRAY, "FLAME-THROWER",   500, 700, 0, 45, 2, 10, 10, 4000, 5.0f, 3},
};

static const Item armor_defs[] = {
    {70, ITEM_ARMOR_HEAD,  "HELMET-1",      0, 0, 5, 0, 0, 0, 0, 200, 1.0f, 0},
    {71, ITEM_ARMOR_HEAD,  "HELMET-2",      0, 0,10, 0, 1, 0, 0, 400, 1.2f, 0},
    {72, ITEM_ARMOR_CHEST, "CHESTPLATE-1",  0, 0, 8, 0, 0, 0, 0, 300, 2.0f, 0},
    {73, ITEM_ARMOR_CHEST, "CHESTPLATE-2",  0, 0,16, 0, 1, 0, 0, 600, 2.5f, 0},
    {74, ITEM_ARMOR_ARM,   "ARM-GUARD-1",   0, 0, 4, 0, 0, 0, 0, 150, 0.8f, 0},
    {75, ITEM_ARMOR_ARM,   "ARM-GUARD-2",   0, 0, 8, 0, 1, 0, 0, 300, 1.0f, 0},
    {76, ITEM_ARMOR_LEG,   "LEG-GUARD-1",   0, 0, 4, 0, 0, 0, 0, 150, 1.0f, 0},
    {77, ITEM_ARMOR_LEG,   "LEG-GUARD-2",   0, 0, 8, 0, 1, 0, 0, 300, 1.2f, 0},
    {78, ITEM_ARMOR_FOOT,  "BOOTS-1",       0, 0, 3, 0, 0, 0, 0, 100, 0.6f, 0},
    {79, ITEM_ARMOR_FOOT,  "BOOTS-2",       0, 0, 6, 0, 1, 0, 0, 200, 0.8f, 0},
    {80, ITEM_ARMOR_HAND,  "GLOVES-1",      0, 0, 2, 0, 0, 0, 0,  80, 0.3f, 0},
    {81, ITEM_ARMOR_HAND,  "GLOVES-2",      0, 0, 4, 0, 1, 0, 0, 160, 0.4f, 0},
};

static const Item misc_defs[] = {
    {90, ITEM_BATTERY,    "BATTERY",     0, 0, 0, 0, 0, 0, 0,  50, 2.3f, 0},
    {91, ITEM_MAP,        "MAP",         0, 0, 0, 0, 0, 0, 0, 500, 1.3f, 0},
    {92, ITEM_CLIPBOARD,  "CLIPBOARD",   0, 0, 0, 0, 0, 0, 0, 100, 0.2f, 0},
    {93, ITEM_DROID_CHIP, "DROID-CHIP",  0, 0, 0, 0, 0, 0, 0, 200, 1.0f, 0},
    {94, ITEM_CAMERA,     "CAMERA",      0, 0, 0, 0, 0, 0, 0, 300, 29.3f, 0},
    {95, ITEM_MINE,       "MINE",        0, 0, 0, 0, 0, 0, 0, 150, 19.2f, 0},
    {96, ITEM_DIE,        "DIE",         0, 0, 0, 0, 0, 0, 0,  20, 0.2f, 0},
    {97, ITEM_DEVSCAPE,   "DEV-SCAPE",   0, 0, 0, 0, 0, 0, 0, 900, 2.4f, 0},
    {98, ITEM_OPTIC,      "OPTIC",       0, 0, 0, 0, 0, 0, 0, 600, 1.4f, 0},
    {99, ITEM_PROBE,      "PLANET-PROBE",0, 0, 0, 0, 0, 0, 0,1000, 92.5f, 0},

    // Ammo types
    {100, ITEM_AMMO, "20MM-CLIP",    0, 0, 0, 0, 0, 30, 30,  50, 0.2f, 0},
    {101, ITEM_AMMO, "45MM-CLIP",    0, 0, 0, 0, 0, 34, 34,  60, 0.3f, 0},
    {102, ITEM_AMMO, "50MM-CLIP",    0, 0, 0, 0, 0, 40, 40,  70, 0.4f, 0},
    {103, ITEM_AMMO, "CARTRIDGES",   0, 0, 0, 0, 0, 42, 42,  65, 0.3f, 0},
    {104, ITEM_AMMO, "LASER-PACK",   0, 0, 0, 0, 0, 20, 20, 100, 3.1f, 0},
    {105, ITEM_AMMO, "SONIC-PACK",   0, 0, 0, 0, 0, 25, 25, 120, 3.3f, 0},
    {106, ITEM_AMMO, "SHELLS",       0, 0, 0, 0, 0, 20, 20, 150, 16.0f, 0},
    {107, ITEM_AMMO, "A51-MISSILES", 0, 0, 0, 0, 0, 15, 15, 200, 11.0f, 0},
    {108, ITEM_AMMO, "GAS-CANISTER", 0, 0, 0, 0, 0, 21, 21,  80, 19.0f, 0},
};

void item_db_init(ItemDatabase *db) {
    memset(db, 0, sizeof(*db));

    int n = 0;
    for (int i = 0; i < (int)(sizeof(weapon_defs)/sizeof(weapon_defs[0])); i++)
        db->defs[n++] = weapon_defs[i];
    for (int i = 0; i < (int)(sizeof(armor_defs)/sizeof(armor_defs[0])); i++)
        db->defs[n++] = armor_defs[i];
    for (int i = 0; i < (int)(sizeof(misc_defs)/sizeof(misc_defs[0])); i++)
        db->defs[n++] = misc_defs[i];
    db->num_defs = n;
}

const Item *item_db_get(const ItemDatabase *db, uint8_t id) {
    for (int i = 0; i < db->num_defs; i++) {
        if (db->defs[i].id == id) return &db->defs[i];
    }
    return NULL;
}

int item_db_find_by_category(const ItemDatabase *db, ItemCategory cat,
                              const Item **out, int max_out) {
    int found = 0;
    for (int i = 0; i < db->num_defs && found < max_out; i++) {
        if (db->defs[i].category == cat)
            out[found++] = &db->defs[i];
    }
    return found;
}
