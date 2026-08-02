#include "inventory.h"
#include <string.h>

/* Item database recovered from the verified DOS executable CAPPO.EXE v1.06.
 * Names and classification bytes extracted from the unpacked binary
 * (SHA-256 fa7d5ca76d26f614476ed41f27cf737084942e9216b20b4605734df9ede9aee4).
 *
 * Original hex prefix bytes: first byte is item flags, second is weapon class.
 * Class mapping from disassembly:
 *   01 = brawling, 07 = close combat (axes/maces), 08 = close combat (swords),
 *   09 = projectile, 0a = firearms, 0b = light/heavy arms,
 *   0c = energy weapon, 0d = heavy auto/cannon/spray,
 *   02 = consumable/utility, 03 = energy cell/ammo
 */
static const Item weapon_defs[] = {
    /* Swords / energy blades (class 0x0c, 0x08) */
    { 1, ITEM_WEAPON_MELEE, "LIGHT SABRE",      40, 80, 0, 1, 5, 0, 0, 1500, 2.0f, 6},
    { 2, ITEM_WEAPON_MELEE, "RAPIER",            15, 30, 0, 1, 2, 0, 0,  400, 1.2f, 2},
    { 3, ITEM_WEAPON_MELEE, "EPEE",              12, 25, 0, 1, 1, 0, 0,  300, 1.0f, 2},
    { 4, ITEM_WEAPON_MELEE, "CUTLASS",           20, 40, 0, 1, 3, 0, 0,  600, 1.5f, 2},

    /* Axes and blunt (class 0x07) */
    { 5, ITEM_WEAPON_MELEE, "AXE",               18, 35, 0, 1, 2, 0, 0,  350, 2.5f, 2},
    { 6, ITEM_WEAPON_MELEE, "BATTLE AXE",        25, 50, 0, 1, 3, 0, 0,  700, 3.5f, 2},
    { 7, ITEM_WEAPON_MELEE, "HALBERD",           30, 55, 0, 2, 4, 0, 0,  900, 4.0f, 2},
    { 8, ITEM_WEAPON_MELEE, "HAMMER",            22, 45, 0, 1, 3, 0, 0,  500, 3.0f, 2},
    { 9, ITEM_WEAPON_MELEE, "MACE",              20, 42, 0, 1, 2, 0, 0,  450, 2.8f, 2},

    /* Projectile (class 0x09) */
    {10, ITEM_WEAPON_HANDGUN, "ARROW",            8, 15, 0, 6, 0, 20, 20, 100, 0.1f, 3},
    {11, ITEM_WEAPON_HANDGUN, "DART",             5, 12, 0, 5, 0, 15, 15,  60, 0.1f, 3},
    {12, ITEM_WEAPON_HANDGUN, "THROWING KNIFE",  10, 20, 0, 4, 1, 10, 10, 150, 0.3f, 3},
    {13, ITEM_WEAPON_HANDGUN, "THROWING STAR",   12, 22, 0, 5, 1, 12, 12, 180, 0.2f, 3},
    {14, ITEM_WEAPON_HANDGUN, "GRENADE",         30, 60, 0, 4, 2,  5,  5, 400, 0.5f, 3},

    /* Firearms (class 0x0a) */
    {20, ITEM_WEAPON_HANDGUN, "PISTOL",          15, 25, 0, 6, 1, 30, 30, 500, 1.0f, 4},
    {21, ITEM_WEAPON_HANDGUN, "LIGHT PISTOL",    12, 20, 0, 6, 0, 25, 25, 350, 0.8f, 4},
    {22, ITEM_WEAPON_HANDGUN, "HEAVY PISTOL",    20, 35, 0, 6, 2, 30, 30, 700, 1.3f, 4},
    {23, ITEM_WEAPON_HANDGUN, "TWIN PISTOL",     25, 40, 0, 6, 3, 40, 40, 900, 1.5f, 4},

    /* Rifles / light arms (class 0x0b) */
    {30, ITEM_WEAPON_RIFLE, "RIFLE",             25, 40, 0, 10, 1, 30, 30,  800, 2.5f, 5},
    {31, ITEM_WEAPON_RIFLE, "ASSAULT RIFLE",     30, 50, 0, 10, 2, 40, 40, 1200, 3.0f, 6},
    {32, ITEM_WEAPON_RIFLE, "SNIPER RIFLE",      40, 60, 0, 15, 3, 20, 20, 1500, 3.5f, 5},
    {33, ITEM_WEAPON_RIFLE, "LASER RIFLE",       50, 80, 0, 12, 4, 25, 25, 2000, 2.8f, 6},
    {34, ITEM_WEAPON_LASER, "PLASMA GUN",        60,100, 0, 10, 5, 20, 20, 2500, 3.2f, 10},

    /* Heavy weapons (class 0x0d) */
    {40, ITEM_WEAPON_CANNON, "CANNON",           50, 90, 0, 8, 3, 15, 15, 2000, 8.0f, 7},
    {41, ITEM_WEAPON_CANNON, "TWIN CANNON",      70,120, 0, 8, 4, 20, 20, 3000, 10.0f, 7},
    {42, ITEM_WEAPON_SPRAY,  "LIGHT SPRAYGUN",   25, 45, 0, 5, 2, 30, 30, 1200, 4.0f, 8},
    {43, ITEM_WEAPON_SPRAY,  "HEAVY SPRAYGUN",   40, 70, 0, 5, 3, 25, 25, 1800, 5.0f, 8},
    {44, ITEM_WEAPON_SPRAY,  "FLAME THROWER",    45, 80, 0, 4, 4, 20, 20, 2200, 5.5f, 8},
    {45, ITEM_WEAPON_CANNON, "ROCKET LAUNCHER",  80,150, 0, 12, 5, 10, 10, 3500, 12.0f, 9},
};

static const Item consumable_defs[] = {
    /* Energy cells (class 0x03) */
    {50, ITEM_AMMO,    "SMALL ENERGY CELL",   0, 0, 0, 0, 0, 10, 10,  50, 0.5f, 0},
    {51, ITEM_AMMO,    "MEDIUM ENERGY CELL",  0, 0, 0, 0, 0, 25, 25, 100, 1.0f, 0},
    {52, ITEM_AMMO,    "LARGE ENERGY CELL",   0, 0, 0, 0, 0, 50, 50, 200, 2.0f, 0},

    /* Clips (class 0x01) */
    {53, ITEM_AMMO,    "SMALL CLIP",          0, 0, 0, 0, 0, 15, 15,  30, 0.2f, 0},
    {54, ITEM_AMMO,    "MEDIUM CLIP",         0, 0, 0, 0, 0, 30, 30,  60, 0.4f, 0},
    {55, ITEM_AMMO,    "LARGE CLIP",          0, 0, 0, 0, 0, 50, 50, 100, 0.6f, 0},

    /* Utility items (class 0x02) */
    {60, ITEM_DEVSCAPE,   "SMALL MEDIPACK",   0, 0, 0, 0, 0, 0, 0, 200, 0.5f, 0},
    {61, ITEM_DEVSCAPE,   "LARGE MEDIPACK",   0, 0, 0, 0, 0, 0, 0, 500, 1.0f, 0},
    {62, ITEM_ARMOR_HEAD, "SMALL SHIELD",     0, 0, 5, 0, 0, 0, 0, 300, 1.5f, 0},
    {63, ITEM_ARMOR_CHEST,"LARGE SHIELD",     0, 0,10, 0, 1, 0, 0, 600, 3.0f, 0},
    {64, ITEM_OPTIC,      "OPTICAL DEVICE",   0, 0, 0, 0, 0, 0, 0, 400, 0.8f, 0},
    {65, ITEM_MAP,        "HOLO MAP",         0, 0, 0, 0, 0, 0, 0, 500, 0.5f, 0},
};

void item_db_init(ItemDatabase *db) {
    memset(db, 0, sizeof(*db));
    int n = 0;
    for (int i = 0; i < (int)(sizeof(weapon_defs)/sizeof(weapon_defs[0])); i++)
        db->defs[n++] = weapon_defs[i];
    for (int i = 0; i < (int)(sizeof(consumable_defs)/sizeof(consumable_defs[0])); i++)
        db->defs[n++] = consumable_defs[i];
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
