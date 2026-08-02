#include "inventory.h"
#include <string.h>

/* Item database recovered from the verified DOS executable CAPPO.EXE v1.06.
 * Names extracted from the unpacked binary at offsets 0x019c90-0x019fe4.
 * (SHA-256 fa7d5ca76d26f614476ed41f27cf737084942e9216b20b4605734df9ede9aee4).
 *
 * The original item table stores items as variable-length records with hex
 * prefix bytes encoding flags, weapon class and stats.  The stat values
 * (damage, range, ammo capacity, price, weight) are NOT yet fully decoded
 * from the binary prefix bytes — only names and categories are confirmed.
 * All numeric stats below are placeholders until the full item record
 * format is recovered from the executable. */
static const Item item_defs[] = {
    /* Body parts (offset 0x019c90-0x019cc9) */
    { 1, ITEM_ARMOR_HEAD,  "HEAD",             0,0,0,0,0,0,0, 100, 1.0f, 0},
    { 2, ITEM_ARMOR_CHEST, "CHEST",            0,0,0,0,0,0,0, 100, 1.0f, 0},
    { 3, ITEM_ARMOR_CHEST, "ARM",              0,0,0,0,0,0,0,  80, 1.0f, 0},
    { 4, ITEM_ARMOR_CHEST, "LEG",              0,0,0,0,0,0,0,  80, 1.0f, 0},
    { 5, ITEM_ARMOR_CHEST, "FOOT",             0,0,0,0,0,0,0,  60, 1.0f, 0},
    { 6, ITEM_ARMOR_CHEST, "HAND",             0,0,0,0,0,0,0,  60, 1.0f, 0},

    /* Currency and utility (0x019cd3-0x019cf4) */
    { 7, ITEM_MAP,         "GOLD",             0,0,0,0,0,0,0,   1, 0.1f, 0},
    { 8, ITEM_AMMO,        "BATTERY",          0,0,0,0,0,0,0,  50, 0.5f, 0},
    { 9, ITEM_AMMO,        "EXPLOSIVES",       0,0,0,0,0,0,0, 200, 1.0f, 0},
    {10, ITEM_DEVSCAPE,    "DEV-SCAPE",        0,0,0,0,0,0,0, 300, 0.5f, 0},

    /* Optics (0x019d00-0x019d08) */
    {11, ITEM_OPTIC,       "OPTIC",            0,0,0,0,0,0,0, 200, 0.3f, 0},
    {12, ITEM_OPTIC,       "CAMERA",           0,0,0,0,0,0,0, 150, 0.3f, 0},

    /* Melee weapons (0x019d11-0x019d4a) */
    {13, ITEM_WEAPON_MELEE, "KNUCLE-DUSTER",   0,0,0,0,0,0,0,  50, 0.5f, 1},
    {14, ITEM_WEAPON_MELEE, "BATTLE-GLOVE",    0,0,0,0,0,0,0, 150, 0.5f, 1},
    {15, ITEM_WEAPON_MELEE, "WAR-BLADE",       0,0,0,0,0,0,0, 300, 1.0f, 2},
    {16, ITEM_WEAPON_MELEE, "LIGHT-BLADE",     0,0,0,0,0,0,0, 500, 0.8f, 2},
    {17, ITEM_WEAPON_MELEE, "FIRE-AXE",        0,0,0,0,0,0,0, 400, 2.0f, 2},

    /* Handguns (0x019d55-0x019d6e) */
    {18, ITEM_WEAPON_HANDGUN, "PISTOL",        0,0,0,0,0,0,0, 400, 1.0f, 4},
    {19, ITEM_WEAPON_HANDGUN, "COLT",          0,0,0,0,0,0,0, 600, 1.2f, 4},
    {20, ITEM_WEAPON_HANDGUN, "MAGNUM",        0,0,0,0,0,0,0, 800, 1.5f, 4},

    /* Rifles (0x019d76-0x019d89) */
    {21, ITEM_WEAPON_RIFLE, "RIFLE",           0,0,0,0,0,0,0, 700, 2.5f, 5},
    {22, ITEM_WEAPON_RIFLE, "SHOTGUN",         0,0,0,0,0,0,0, 900, 3.0f, 5},
    {23, ITEM_WEAPON_RIFLE, "HUNTER",          0,0,0,0,0,0,0,1100, 3.5f, 5},

    /* Automatics (0x019d90-0x019da3) */
    {24, ITEM_WEAPON_RIFLE, "UZIE",            0,0,0,0,0,0,0, 800, 2.0f, 6},
    {25, ITEM_WEAPON_RIFLE, "RAPEDO",          0,0,0,0,0,0,0,1200, 2.5f, 6},
    {26, ITEM_WEAPON_RIFLE, "BOOSTER",         0,0,0,0,0,0,0,1500, 3.0f, 6},

    /* Energy weapons (0x019db0-0x019dca) */
    {27, ITEM_WEAPON_LASER, "HAND-LASER",      0,0,0,0,0,0,0,1500, 1.5f, 7},
    {28, ITEM_WEAPON_LASER, "LYTE-ZAPPER",     0,0,0,0,0,0,0,2000, 2.0f, 8},
    {29, ITEM_WEAPON_LASER, "ION-PULSE",       0,0,0,0,0,0,0,2500, 2.5f, 9},

    /* Heavy weapons (0x019dd8-0x019e11) */
    {30, ITEM_WEAPON_CANNON, "MONO-CANNON",    0,0,0,0,0,0,0,2000, 8.0f, 7},
    {31, ITEM_WEAPON_CANNON, "A51-LAUNCHER",   0,0,0,0,0,0,0,3000,10.0f, 8},
    {32, ITEM_WEAPON_CANNON, "TWIN-CANNON",    0,0,0,0,0,0,0,3500,10.0f, 7},
    {33, ITEM_WEAPON_SPRAY,  "AIROSOLL",       0,0,0,0,0,0,0,1200, 4.0f, 8},
    {34, ITEM_WEAPON_SPRAY,  "ACID-DISPERSER", 0,0,0,0,0,0,0,1800, 5.0f, 8},
    {35, ITEM_WEAPON_SPRAY,  "FLAME-THROWER",  0,0,0,0,0,0,0,2200, 5.5f, 8},

    /* Weapon variants (0x019eb2-0x019eca) */
    {36, ITEM_WEAPON_RIFLE, "A12-",            0,0,0,0,0,0,0,1000, 3.0f, 6},
    {37, ITEM_WEAPON_LASER, "L22-",            0,0,0,0,0,0,0,1800, 2.5f, 7},
    {38, ITEM_WEAPON_CANNON,"X42-2-",          0,0,0,0,0,0,0,2800, 8.0f, 8},

    /* Ammo (0x019ed4-0x019f37) */
    {40, ITEM_AMMO, "CARTRIDGES",              0,0,0,0,0,0,0,  30, 0.2f, 0},
    {41, ITEM_AMMO, "A51 MISSILES",            0,0,0,0,0,0,0, 200, 1.0f, 0},
    {42, ITEM_AMMO, "SHELLS",                  0,0,0,0,0,0,0,  40, 0.3f, 0},
    {43, ITEM_AMMO, "LASER PACK",              0,0,0,0,0,0,0, 100, 0.5f, 0},
    {44, ITEM_AMMO, "SONIC PACK",              0,0,0,0,0,0,0, 120, 0.5f, 0},
    {45, ITEM_AMMO, "POISON",                  0,0,0,0,0,0,0,  80, 0.3f, 0},
    {46, ITEM_AMMO, "ACID",                    0,0,0,0,0,0,0,  90, 0.3f, 0},
    {47, ITEM_AMMO, "FLAMBOS",                 0,0,0,0,0,0,0, 100, 0.4f, 0},

    /* Utility items (0x019f52-0x019fcb) */
    {48, ITEM_MAP,     "PLANET PROBE",         0,0,0,0,0,0,0, 500, 0.5f, 0},
    {49, ITEM_DEVSCAPE,"CLIPBOARD",            0,0,0,0,0,0,0, 100, 0.3f, 0},
    {50, ITEM_AMMO,    "MINE",                 0,0,0,0,0,0,0, 300, 1.0f, 0},
    {51, ITEM_MAP,     "DIE",                  0,0,0,0,0,0,0,  10, 0.1f, 0},
    {52, ITEM_AMMO,    "BALL",                 0,0,0,0,0,0,0,  20, 0.2f, 0},
    {53, ITEM_AMMO,    "SUPER BALL",           0,0,0,0,0,0,0,  50, 0.3f, 0},
    {54, ITEM_MAP,     "MAP",                  0,0,0,0,0,0,0, 200, 0.2f, 0},
    {55, ITEM_DEVSCAPE,"DROID CHIP",           0,0,0,0,0,0,0, 400, 0.1f, 0},
    {56, ITEM_MAP,     "MESSAGE FROM RATT",    0,0,0,0,0,0,0,   0, 0.1f, 0},
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
