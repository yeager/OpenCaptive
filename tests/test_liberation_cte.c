/* Tests for the CTE (CITY_TEXT) section parser — the first step of decoding
 * Liberation's authentic interaction script.  A fixture pins the binary
 * section framing; an optional real-data check parses the actual CTE. */
#include "liberation_cte.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Two sections in the real frame: 0xD7 <id-hi> <id-lo> 0x00 <len> [ content ].
 * The id is 16-bit big-endian; <len> is a non-load-bearing hint (content is
 * bracket-delimited).  0x4141 == 16705, 0x4242 == 16962. */
static const uint8_t fixture[] = {
    0xD7, 0x41, 0x41, 0x00, 0x08, '[', 'H', 'e', 'l', 'l', 'o', ']',
    0xD7, 0x42, 0x42, 0x00, 0x0a, '[', 'W', 'o', 'r', 'l', 'd', '!', ']',
};

static void test_parse_fixture(void) {
    CteTable t;
    assert(cte_table_parse(&t, fixture, sizeof(fixture)));
    assert(t.section_count == 2);

    const CteSection *a = cte_section_find(&t, 0x4141);
    assert(a && a->length == 5 && memcmp(a->content, "Hello", 5) == 0);
    const CteSection *b = cte_section_find(&t, 0x4242);
    assert(b && b->length == 6 && memcmp(b->content, "World!", 6) == 0);
    assert(cte_section_find(&t, 0x9999) == NULL);
}

static void test_rejects_empty(void) {
    CteTable t;
    assert(!cte_table_parse(&t, NULL, 0));
    const uint8_t none[] = { 'n', 'o', 'b', 'r', 'a', 'c', 'k', 'e', 't' };
    assert(!cte_table_parse(&t, none, sizeof(none)));
}

/* Nested brackets inside a section must not truncate its content. */
static void test_nested_brackets(void) {
    static const uint8_t nested[] = {
        0xD7, 0x00, 0x50, 0x00, 0x0c, '[', 'a', '[', 'b', ']', 'c', ']',
    };
    CteTable t;
    assert(cte_table_parse(&t, nested, sizeof(nested)));
    assert(t.section_count == 1);
    const CteSection *s = cte_section_find(&t, 0x0050);
    assert(s && s->length == 5 && memcmp(s->content, "a[b]c", 5) == 0);
}

/* Build a one-section table around a bytecode body and expand it. */
static void expand_body(const uint8_t *body, size_t body_len, CteState *st,
                        char *out, size_t out_size) {
    uint8_t buf[512];
    assert(body_len + 7 <= sizeof(buf));
    buf[0] = 0xD7; buf[1] = 0x00; buf[2] = 0x41; buf[3] = 0x00;
    buf[4] = (uint8_t)(body_len + 3);
    buf[5] = '[';
    memcpy(buf + 6, body, body_len);
    buf[6 + body_len] = ']';
    CteTable t;
    assert(cte_table_parse(&t, buf, body_len + 7));
    const CteSection *s = cte_section_find(&t, 0x0041);
    assert(s);
    assert(cte_expand(&t, s, st, out, out_size));
}

#define EXPAND(lit, st, out) expand_body((const uint8_t *)(lit), sizeof(lit) - 1, (st), (out), sizeof(out))

static void test_plain_text(void) {
    CteState st; cte_state_init(&st, 1);
    char out[256];
    EXPAND("Good day, citizen.", &st, out);
    assert(strcmp(out, "Good day, citizen.") == 0);
}

static void test_newline_and_comment(void) {
    CteState st; cte_state_init(&st, 1);
    char out[256];
    EXPAND("A^^B", &st, out);
    assert(strcmp(out, "A\nB") == 0);
}

/* ^XI<cond>[then|else]: pick branch by evaluating the condition on state. */
static void test_conditional_true_false(void) {
    char out[256];
    CteState st; cte_state_init(&st, 1);
    cte_state_set(&st, "E", 0);
    EXPAND("^XIE=0[no record|on record]", &st, out);
    assert(strcmp(out, "no record") == 0);

    cte_state_set(&st, "E", 4);
    EXPAND("^XIE=0[no record|on record]", &st, out);
    assert(strcmp(out, "on record") == 0);
}

/* Compound OR condition (a!b): true if either term holds. */
static void test_conditional_compound_or(void) {
    char out[256];
    CteState st; cte_state_init(&st, 1);
    cte_state_set(&st, "mc", 16);
    EXPAND("^XI(mc=16!mc=18)[match|nomatch]", &st, out);
    assert(strcmp(out, "match") == 0);
    cte_state_set(&st, "mc", 5);
    EXPAND("^XI(mc=16!mc=18)[match|nomatch]", &st, out);
    assert(strcmp(out, "nomatch") == 0);
}

/* Compound AND condition (a&b): true only if both terms hold. */
static void test_conditional_compound_and(void) {
    char out[256];
    CteState st; cte_state_init(&st, 1);
    cte_state_set(&st, "g", 0);
    cte_state_set(&st, "v", 0);
    EXPAND("^XI(g=0&v<1)[both|not]", &st, out);
    assert(strcmp(out, "both") == 0);
    cte_state_set(&st, "v", 5);
    EXPAND("^XI(g=0&v<1)[both|not]", &st, out);
    assert(strcmp(out, "not") == 0);
}

/* ^XC<var>[c0|c1|c2]: switch on the variable's integer value. */
static void test_case_switch(void) {
    char out[256];
    CteState st; cte_state_init(&st, 1);
    cte_state_set(&st, "H", 2);
    EXPAND("^XCH[zero|one|two|three]", &st, out);
    assert(strcmp(out, "two") == 0);
}

/* Side-effect opcodes emit nothing but must not corrupt surrounding text. */
static void test_skips_side_effects(void) {
    char out[256];
    CteState st; cte_state_init(&st, 1);
    EXPAND("Pay ^X=g100 now.", &st, out);
    assert(strcmp(out, "Pay  now.") == 0);
}

/* ^XS<id> inlines a called section's text and continues; ^XG<id> inlines then
 * ends the current section (goto). Build a two-section table by hand. */
static void test_call_inlining(void) {
    /* section 100: "A^XS200B"  ; section 200: "MID" */
    static const uint8_t buf[] = {
        0xD7, 0x00, 0x64, 0x00, 0x0b, '[', 'A', '^', 'X', 'S', '2', '0', '0', 'B', ']',
        0xD7, 0x00, 0xC8, 0x00, 0x06, '[', 'M', 'I', 'D', ']',
    };
    CteTable t;
    assert(cte_table_parse(&t, buf, sizeof(buf)));
    assert(cte_section_find(&t, 100) && cte_section_find(&t, 200));
    CteState st; cte_state_init(&st, 1);
    char out[256];
    assert(cte_expand(&t, cte_section_find(&t, 100), &st, out, sizeof(out)));
    assert(strcmp(out, "AMIDB") == 0);

    /* goto: text after ^XG is dead. section 101: "A^XG200B" -> "AMID" */
    static const uint8_t buf2[] = {
        0xD7, 0x00, 0x65, 0x00, 0x0b, '[', 'A', '^', 'X', 'G', '2', '0', '0', 'B', ']',
        0xD7, 0x00, 0xC8, 0x00, 0x06, '[', 'M', 'I', 'D', ']',
    };
    assert(cte_table_parse(&t, buf2, sizeof(buf2)));
    assert(cte_expand(&t, cte_section_find(&t, 101), &st, out, sizeof(out)));
    assert(strcmp(out, "AMID") == 0);
}

/* A cyclic call (A->A) must terminate and not overflow. */
static void test_call_cycle_safe(void) {
    static const uint8_t buf[] = {
        0xD7, 0x00, 0x64, 0x00, 0x0b, '[', 'X', '^', 'X', 'S', '1', '0', '0', 'Y', ']',
    };
    CteTable t;
    assert(cte_table_parse(&t, buf, sizeof(buf)));
    CteState st; cte_state_init(&st, 1);
    char out[256];
    assert(cte_expand(&t, cte_section_find(&t, 100), &st, out, sizeof(out)));
    assert(strcmp(out, "XY") == 0);  /* self-call refused, no runaway */
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

    /* Expand every authentic section with an empty (initial) state: the
     * interpreter must terminate, stay within the output buffer, and never
     * leave a raw opcode marker ('^') in the emitted text. */
    unsigned expanded = 0;
    for (unsigned i = 0; i < t.section_count; i++) {
        CteState st; cte_state_init(&st, 1);
        char out[8192];
        assert(cte_expand(&t, &t.sections[i], &st, out, sizeof(out)));
        assert(strchr(out, '^') == NULL);   /* no unparsed opcode escaped */
        assert(strchr(out, '[') == NULL);   /* no raw branch group leaked */
        expanded++;
    }
    printf("PASS: real_cte (%u sections, %u expanded)\n",
           t.section_count, expanded);
}

int main(void) {
    test_parse_fixture();
    test_rejects_empty();
    test_nested_brackets();
    test_plain_text();
    test_newline_and_comment();
    test_conditional_true_false();
    test_conditional_compound_or();
    test_conditional_compound_and();
    test_case_switch();
    test_skips_side_effects();
    test_call_inlining();
    test_call_cycle_safe();
    test_real_cte_if_available();
    printf("All liberation_cte tests passed\n");
    return 0;
}
