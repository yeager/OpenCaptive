#include "inventory.h"
#include <string.h>

/* Item database recovered from CAPPO.EXE v1.06 (unpacked).
 * SHA-256: fa7d5ca76d26f614476ed41f27cf737084942e9216b20b4605734df9ede9aee4
 * Item table at offset 0x1a090. Record format: 00 type_code [grade] NAME 0x20.
 * Type codes: 0x27/0x65=body part, 0x08=consumable, 0x21=equipment,
 *             0x30=ranged weapon, 0x60=explosive, 0x10=ammo, 0x00=misc, 0x20=chip.
 * Numeric stats (damage, price, weight) are provisional — computed procedurally
 * in the original from type_code + material grade. */
static const Item item_defs[] = {
    /* Body parts — type 0x27 (ARM uses 0x65), all grade 0x05 */
    { 1, ITEM_ARMOR_HEAD,  "HEAD",             0x27, 0,0,0,0,0,0,0, 100, 1.0f, 0},
    { 2, ITEM_ARMOR_CHEST, "CHEST",            0x27, 0,0,0,0,0,0,0, 100, 1.0f, 0},
    { 3, ITEM_ARMOR_ARM,   "ARM",              0x65, 0,0,0,0,0,0,0,  80, 1.0f, 0},
    { 4, ITEM_ARMOR_LEG,   "LEG",              0x27, 0,0,0,0,0,0,0,  80, 1.0f, 0},
    { 5, ITEM_ARMOR_FOOT,  "FOOT",             0x27, 0,0,0,0,0,0,0,  60, 1.0f, 0},
    { 6, ITEM_ARMOR_HAND,  "HAND",             0x27, 0,0,0,0,0,0,0,  60, 1.0f, 0},

    /* Currency and consumable */
    { 7, ITEM_GOLD,        "GOLD",             0x27, 0,0,0,0,0,0,0,   1, 0.1f, 0},
    { 8, ITEM_BATTERY,     "BATTERY",          0x08, 0,0,0,0,0,0,0,  50, 0.5f, 0},
    { 9, ITEM_EXPLOSIVE,   "EXPLOSIVES",       0x60, 0,0,0,0,0,0,0, 200, 1.0f, 0},
    {10, ITEM_DEVSCAPE,    "DEV-SCAPE",        0x00, 0,0,0,0,0,0,0, 300, 0.5f, 0},

    /* Optics — type 0x21 */
    {11, ITEM_OPTIC,       "OPTIC",            0x21, 0,0,0,0,0,0,0, 200, 0.3f, 0},
    {12, ITEM_CAMERA,      "CAMERA",           0x21, 0,0,0,0,0,0,0, 150, 0.3f, 0},

    /* Melee weapons — KNUCLE-DUSTER is type 0x00, rest are 0x21 */
    {13, ITEM_WEAPON_MELEE, "KNUCLE-DUSTER",   0x00, 0,0,0,0,0,0,0,  50, 0.5f, 1},
    {14, ITEM_WEAPON_MELEE, "BATTLE-GLOVE",    0x21, 0,0,0,0,0,0,0, 150, 0.5f, 1},
    {15, ITEM_WEAPON_MELEE, "WAR-BLADE",       0x21, 0,0,0,0,0,0,0, 300, 1.0f, 2},
    {16, ITEM_WEAPON_MELEE, "LIGHT-BLADE",     0x21, 0,0,0,0,0,0,0, 500, 0.8f, 2},
    {17, ITEM_WEAPON_MELEE, "FIRE-AXE",        0x21, 0,0,0,0,0,0,0, 400, 2.0f, 2},

    /* Handguns — PISTOL is type 0x21, COLT/MAGNUM are 0x30 */
    {18, ITEM_WEAPON_HANDGUN, "PISTOL",        0x21, 0,0,0,0,0,0,0, 400, 1.0f, 4},
    {19, ITEM_WEAPON_HANDGUN, "COLT",          0x30, 0,0,0,0,0,0,0, 600, 1.2f, 4},
    {20, ITEM_WEAPON_HANDGUN, "MAGNUM",        0x30, 0,0,0,0,0,0,0, 800, 1.5f, 4},

    /* Rifles — all type 0x30 */
    {21, ITEM_WEAPON_RIFLE, "RIFLE",           0x30, 0,0,0,0,0,0,0, 700, 2.5f, 5},
    {22, ITEM_WEAPON_RIFLE, "SHOTGUN",         0x30, 0,0,0,0,0,0,0, 900, 3.0f, 5},
    {23, ITEM_WEAPON_RIFLE, "HUNTER",          0x30, 0,0,0,0,0,0,0,1100, 3.5f, 5},

    /* Automatics — all type 0x30 */
    {24, ITEM_WEAPON_AUTO,  "UZIE",            0x30, 0,0,0,0,0,0,0, 800, 2.0f, 6},
    {25, ITEM_WEAPON_AUTO,  "RAPEDO",          0x30, 0,0,0,0,0,0,0,1200, 2.5f, 6},
    {26, ITEM_WEAPON_AUTO,  "BOOSTER",         0x30, 0,0,0,0,0,0,0,1500, 3.0f, 6},

    /* Energy weapons — all type 0x30 */
    {27, ITEM_WEAPON_LASER, "HAND-LASER",      0x30, 0,0,0,0,0,0,0,1500, 1.5f, 7},
    {28, ITEM_WEAPON_LASER, "LYTE-ZAPPER",     0x30, 0,0,0,0,0,0,0,2000, 2.0f, 8},
    {29, ITEM_WEAPON_LASER, "ION-PULSE",       0x30, 0,0,0,0,0,0,0,2500, 2.5f, 9},

    /* Heavy weapons — all type 0x30 */
    {30, ITEM_WEAPON_CANNON, "MONO-CANNON",    0x30, 0,0,0,0,0,0,0,2000, 8.0f, 7},
    {31, ITEM_WEAPON_CANNON, "A51-LAUNCHER",   0x30, 0,0,0,0,0,0,0,3000,10.0f, 8},
    {32, ITEM_WEAPON_CANNON, "TWIN-CANNON",    0x30, 0,0,0,0,0,0,0,3500,10.0f, 7},
    {33, ITEM_WEAPON_SPRAY,  "AIROSOLL",       0x30, 0,0,0,0,0,0,0,1200, 4.0f, 8},
    {34, ITEM_WEAPON_SPRAY,  "ACID-DISPERSER", 0x30, 0,0,0,0,0,0,0,1800, 5.0f, 8},
    {35, ITEM_WEAPON_SPRAY,  "FLAME-THROWER",  0x30, 0,0,0,0,0,0,0,2200, 5.5f, 8},

    /* Weapon variant prefixes — type 0x30 */
    {36, ITEM_WEAPON_AUTO,   "A12-",           0x30, 0,0,0,0,0,0,0,1000, 3.0f, 6},
    {37, ITEM_WEAPON_LASER,  "L22-",           0x30, 0,0,0,0,0,0,0,1800, 2.5f, 7},
    {38, ITEM_WEAPON_CANNON, "X42-2-",         0x30, 0,0,0,0,0,0,0,2800, 8.0f, 8},

    /* Ammo — type 0x10 */
    {40, ITEM_AMMO, "CARTRIDGES",              0x10, 0,0,0,0,0,0,0,  30, 0.2f, 0},
    {41, ITEM_AMMO, "A51 MISSILES",            0x10, 0,0,0,0,0,0,0, 200, 1.0f, 0},
    {42, ITEM_AMMO, "SHELLS",                  0x10, 0,0,0,0,0,0,0,  40, 0.3f, 0},
    {43, ITEM_AMMO, "LASER PACK",              0x10, 0,0,0,0,0,0,0, 100, 0.5f, 0},
    {44, ITEM_AMMO, "SONIC PACK",              0x10, 0,0,0,0,0,0,0, 120, 0.5f, 0},
    {45, ITEM_AMMO, "POISON",                  0x10, 0,0,0,0,0,0,0,  80, 0.3f, 0},
    {46, ITEM_AMMO, "ACID",                    0x10, 0,0,0,0,0,0,0,  90, 0.3f, 0},
    {47, ITEM_AMMO, "FLAMBOS",                 0x10, 0,0,0,0,0,0,0, 100, 0.4f, 0},

    /* Utility items — type 0x00 unless noted */
    {48, ITEM_PROBE,     "PLANET PROBE",       0x10, 0,0,0,0,0,0,0, 500, 0.5f, 0},
    {49, ITEM_CLIPBOARD, "CLIPBOARD",          0x00, 0,0,0,0,0,0,0, 100, 0.3f, 0},
    {50, ITEM_MINE,      "MINE",               0x00, 0,0,0,0,0,0,0, 300, 1.0f, 0},
    {51, ITEM_DIE,       "DIE",                0x00, 0,0,0,0,0,0,0,  10, 0.1f, 0},
    {52, ITEM_AMMO,      "BALL",               0x00, 0,0,0,0,0,0,0,  20, 0.2f, 0},
    {53, ITEM_AMMO,      "SUPER BALL",         0x00, 0,0,0,0,0,0,0,  50, 0.3f, 0},
    {54, ITEM_MAP,       "MAP",                0x00, 0,0,0,0,0,0,0, 200, 0.2f, 0},
    {55, ITEM_DROID_CHIP, "DROID CHIP",        0x20, 0,0,0,0,0,0,0, 400, 0.1f, 0},
    {56, ITEM_MAP,       "MESSAGE FROM RATT",  0x00, 0,0,0,0,0,0,0,   0, 0.1f, 0},
    {57, ITEM_KEY,       "KEY",                0x00, 0,0,0,0,0,0,0,  50, 0.1f, 0},
};

const UpgradeTier upgrade_tiers[UPGRADE_TIER_COUNT] = {
    {0x30, "1.9"},   {0x21, "2"},     {0x21, "3C"},    {0x21, "5"},
    {0x21, "7"},     {0x21, "14A"},   {0x30, "14B"},   {0x30, "27"},
    {0x30, "23"},    {0x30, "33.1"},  {0x30, "56"},    {0x30, "78"},
    {0x30, "99"},    {0x30, "111"},   {0x30, "141"},   {0x30, "165.8"},
    {0x30, "180"},   {0x30, "200"},   {0x30, "211"},   {0x30, "231"},
    {0x30, "A12"},   {0x30, "L22"},   {0x30, "X42"},
};

const MeleeDamageEntry melee_damage[MELEE_DAMAGE_COUNT] = {
    {0x04, 0x21}, {0x04, 0x2A}, {0x04, 0x33}, {0x04, 0x3C},
    {0x4C, 0x21}, {0x4C, 0x2A}, {0x4C, 0x33}, {0x4C, 0x3C},
    {0x15, 0x21}, {0x12, 0x2A}, {0x0E, 0x33}, {0x0F, 0x3C}, {0x13, 0x45},
    {0x5A, 0x21}, {0x5A, 0x2A}, {0x57, 0x33}, {0x52, 0x3C}, {0x59, 0x45},
};

const RangedDamageEntry ranged_damage[RANGED_DAMAGE_COUNT] = {
    {72, 45, 1}, {88, 45, 1}, {104, 45, 1}, {120, 45, 0},
    {72, 35, 1}, {88, 35, 1}, {104, 35, 1}, {120, 35, 0},
    {72, 25, 1}, {88, 25, 1}, {104, 25, 1}, {120, 25, 0},
    {72, 15, 1}, {88, 15, 1}, {104, 15, 1}, {120, 15, 0},
    {72,  5, 0}, {88,  5, 0}, {104,  5, 0}, {120,  5, 0},
};

void item_db_init(ItemDatabase *db) {
    memset(db, 0, sizeof(*db));
    int n = sizeof(item_defs) / sizeof(item_defs[0]);
    if (n > MAX_ITEM_DEFS) n = MAX_ITEM_DEFS;
    memcpy(db->defs, item_defs, n * sizeof(Item));
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
