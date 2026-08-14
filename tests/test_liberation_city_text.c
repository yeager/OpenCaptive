/* Tests for the authentic CTE clue provider that the building interaction uses
 * in place of invented rumor text. */
#include "liberation_city_text.h"
#include "liberation_cte.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Clue 0 = sections 300 (0x012c) and 316 (0x013c), the two halves. */
static const uint8_t fixture[] = {
    0xD7, 0x01, 0x2c, 0x00, 0x10, '[', 'T', 'i', 'g', 'e', 'r', 's', '.', '.', '.', ']',
    0xD7, 0x01, 0x3c, 0x00, 0x11, '[', ' ', '.', '.', '.', 'a', 'n', 'd', ' ', 'h', 'o', 'r', 's', 'e', 's', ']',
};

static void test_no_table_returns_false(void) {
    liberation_city_text_set(NULL);
    char out[256] = "untouched";
    assert(!liberation_city_text_clue(0, out, sizeof(out)));
    assert(strcmp(out, "untouched") == 0);  /* caller keeps its own text */
}

static void test_fixture_clue(void) {
    CteTable t;
    assert(cte_table_parse(&t, fixture, sizeof(fixture)));
    liberation_city_text_set(&t);
    char out[256];
    assert(liberation_city_text_clue(0, out, sizeof(out)));
    assert(strcmp(out, "Tigers... ...and horses") == 0);
    /* seed selects among the 16 clues modulo 16; clue 0 repeats at 16. */
    char out2[256];
    assert(liberation_city_text_clue(16, out2, sizeof(out2)));
    assert(strcmp(out2, out) == 0);
    liberation_city_text_set(NULL);
}

static void test_real_clue_if_available(void) {
    const char *path = getenv("OPENCAPTIVE_TEST_CTE");
    if (!path) { printf("SKIP: real clue (set OPENCAPTIVE_TEST_CTE)\n"); return; }
    FILE *f = fopen(path, "rb");
    if (!f) { printf("SKIP: real clue (unreadable)\n"); return; }
    static uint8_t buf[262144];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    CteTable t;
    assert(cte_table_parse(&t, buf, n));
    liberation_city_text_set(&t);
    /* Every one of the 16 authentic clues must expand to non-trivial text. */
    for (unsigned i = 0; i < 16; i++) {
        char out[512];
        assert(liberation_city_text_clue(i, out, sizeof(out)));
        assert(strlen(out) > 8);
        assert(strchr(out, '^') == NULL && strchr(out, '[') == NULL);
    }
    printf("PASS: real clue (16 authentic quotes)\n");
    liberation_city_text_set(NULL);
}

int main(void) {
    test_no_table_returns_false();
    test_fixture_clue();
    test_real_clue_if_available();
    printf("All liberation_city_text tests passed\n");
    return 0;
}
