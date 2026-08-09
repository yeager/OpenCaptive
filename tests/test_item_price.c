#include "item_price.h"
#include <stdio.h>

static int failures = 0;
#define ASSERT(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } \
} while(0)

static void test_variant_table(void) {
    ASSERT(item_variants[0].type_code == 0x30, "tier 0 is ranged");
    ASSERT(item_variants[1].type_code == 0x21, "tier 1 is melee");
    ASSERT(item_variants[5].price == 140, "melee tier 5 price");
    ASSERT(item_variants[6].price == 140, "ranged tier 1 price");
    ASSERT(item_variants[17].price == 2000, "ranged tier 12 price");
}

static void test_price_early_game(void) {
    uint16_t p = item_compute_price(0, 3, 0, 0, 50);
    ASSERT(p == 0x80, "early game returns base price 128");
}

static void test_price_scales(void) {
    uint16_t p1 = item_compute_price(1, 10, 5, 2, 50);
    uint16_t p2 = item_compute_price(2, 10, 5, 2, 50);
    ASSERT(p2 > p1, "price scales with game level");
}

static void test_price_difficulty(void) {
    uint16_t p1 = item_compute_price(1, 10, 5, 2, 10);
    uint16_t p2 = item_compute_price(1, 10, 5, 2, 90);
    ASSERT(p2 > p1, "higher difficulty = higher price");
}

static void test_availability_grade_zero(void) {
    ASSERT(item_check_availability(0, 50, 1) == 1, "grade 0 always available");
}

static void test_availability_returns_nonzero(void) {
    /* `r > 0 || r == 0` is true for every uint8_t, so the old assertion could
     * not fail.  Assert the property that actually matters: the routine is
     * pure, so identical inputs must yield identical output, and it must not
     * vary with an unrelated call in between. */
    uint8_t r = item_check_availability(1, 50, 1);
    uint8_t other = item_check_availability(2, 90, 3);
    (void)other;
    ASSERT(item_check_availability(1, 50, 1) == r,
           "availability is deterministic for identical inputs");
}

int main(void) {
    test_variant_table();
    test_price_early_game();
    test_price_scales();
    test_price_difficulty();
    test_availability_grade_zero();
    test_availability_returns_nonzero();
    if (failures) {
        printf("%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All item_price tests passed\n");
    return 0;
}
