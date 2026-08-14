/* Tests for the CTE (CITY_TEXT) section parser — the first step of decoding
 * Liberation's authentic interaction script.  A fixture pins the binary
 * section framing; an optional real-data check parses the actual CTE. */
#include "liberation_cte.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Two sections in the real frame: <id> 0x00 <len> [ content ]. */
static const uint8_t fixture[] = {
    0x41, 0x00, 0x08, '[', 'H', 'e', 'l', 'l', 'o', ']',
    0x42, 0x00, 0x0a, '[', 'W', 'o', 'r', 'l', 'd', '!', ']',
};

static void test_parse_fixture(void) {
    CteTable t;
    assert(cte_table_parse(&t, fixture, sizeof(fixture)));
    assert(t.section_count == 2);

    const CteSection *a = cte_section_find(&t, 0x41);
    assert(a && a->length == 5 && memcmp(a->content, "Hello", 5) == 0);
    const CteSection *b = cte_section_find(&t, 0x42);
    assert(b && b->length == 6 && memcmp(b->content, "World!", 6) == 0);
    assert(cte_section_find(&t, 0x99) == NULL);
}

static void test_rejects_empty(void) {
    CteTable t;
    assert(!cte_table_parse(&t, NULL, 0));
    const uint8_t none[] = { 'n', 'o', 'b', 'r', 'a', 'c', 'k', 'e', 't' };
    assert(!cte_table_parse(&t, none, sizeof(none)));
}

/* Nested brackets inside a section must not be treated as new sections. */
static void test_nested_brackets(void) {
    static const uint8_t nested[] = {
        0x50, 0x00, 0x0c, '[', 'a', '[', 'b', ']', 'c', ']',
    };
    CteTable t;
    assert(cte_table_parse(&t, nested, sizeof(nested)));
    assert(t.section_count == 1);
    const CteSection *s = cte_section_find(&t, 0x50);
    assert(s && s->length == 5 && memcmp(s->content, "a[b]c", 5) == 0);
}

/* Parse the real CTE when a raw dump is provided via OPENCAPTIVE_TEST_CTE. */
static void test_real_cte_if_available(void) {
    const char *path = getenv("OPENCAPTIVE_TEST_CTE");
    if (!path) { printf("SKIP: real_cte (set OPENCAPTIVE_TEST_CTE)\n"); return; }
    FILE *f = fopen(path, "rb");
    if (!f) { printf("SKIP: real_cte (%s unreadable)\n", path); return; }
    static uint8_t buf[262144];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    CteTable t;
    assert(cte_table_parse(&t, buf, n));
    assert(t.section_count >= 40);
    printf("PASS: real_cte (%u sections)\n", t.section_count);
}

int main(void) {
    test_parse_fixture();
    test_rejects_empty();
    test_nested_brackets();
    test_real_cte_if_available();
    printf("All liberation_cte tests passed\n");
    return 0;
}
