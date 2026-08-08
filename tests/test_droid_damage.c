#include "droid_damage.h"
#include <assert.h>

static void test_shield_absorbs_environmental_damage(void) {
    Droid d = {0};
    d.hp = 100;
    d.shield = 1;
    d.shield_hp = 12;

    droid_apply_environmental_damage(&d, 7);
    assert(d.hp == 100);
    assert(d.shield_hp == 5);

    droid_apply_environmental_damage(&d, 9);
    assert(d.hp == 96);
    assert(d.shield_hp == 0);
}

static void test_environmental_damage_handles_invalid_and_lethal_inputs(void) {
    Droid d = {0};
    d.hp = 10;
    droid_apply_environmental_damage(&d, 0);
    droid_apply_environmental_damage(&d, -4);
    assert(d.hp == 10);

    droid_apply_environmental_damage(&d, 10);
    assert(d.hp == 0);
    droid_apply_environmental_damage(&d, 10);
    assert(d.hp == 0);
}

int main(void) {
    test_shield_absorbs_environmental_damage();
    test_environmental_damage_handles_invalid_and_lethal_inputs();
    return 0;
}
