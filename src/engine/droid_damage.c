#include "droid_damage.h"

void droid_apply_environmental_damage(Droid *droid, int damage) {
    if (!droid || damage <= 0 || droid->hp <= 0) return;

    if (droid->shield != 0 && droid->shield_hp > 0) {
        if (damage <= droid->shield_hp) {
            droid->shield_hp -= (int16_t)damage;
            return;
        }
        damage -= droid->shield_hp;
        droid->shield_hp = 0;
    }

    if (damage >= droid->hp)
        droid->hp = 0;
    else
        droid->hp = (int16_t)(droid->hp - damage);
}
