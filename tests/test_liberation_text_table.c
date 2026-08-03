#include "liberation_text_table.h"
#include "arcd_decoder.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_parse_basic(void) {
    const char *input = "header\r\n|1[hello|world]\r\n|2[foo]\r\n";
    LibTextTable table;
    assert(lib_text_table_parse(&table, (const uint8_t *)input, strlen(input)));
    assert(table.section_count == 2);
    assert(table.sections[0].id == 1);
    assert(table.sections[1].id == 2);
    printf("PASS: parse_basic\n");
}

static void test_find_section(void) {
    const char *input = "|5[alpha]|10[beta]|99[gamma]";
    LibTextTable table;
    assert(lib_text_table_parse(&table, (const uint8_t *)input, strlen(input)));
    assert(table.section_count == 3);
    const LibTextSection *s = lib_text_table_find(&table, 10);
    assert(s && s->id == 10);
    assert(lib_text_table_find(&table, 42) == NULL);
    printf("PASS: find_section\n");
}

static void test_expand_plain_text(void) {
    const char *input = "|1[A plain room with no opcodes.]";
    LibTextTable table;
    assert(lib_text_table_parse(&table, (const uint8_t *)input, strlen(input)));
    LibTextState state;
    lib_text_state_init(&state, 12345);
    char out[512];
    assert(lib_text_expand_id(&table, 1, &state, 0, out, sizeof(out)));
    assert(strcmp(out, "A plain room with no opcodes.") == 0);
    printf("PASS: expand_plain_text\n");
}

static void test_comment_skip(void) {
    const char *input = "|1[^; this is a comment\r\nVisible text.]";
    LibTextTable table;
    assert(lib_text_table_parse(&table, (const uint8_t *)input, strlen(input)));
    LibTextState state;
    lib_text_state_init(&state, 0);
    char out[512];
    assert(lib_text_expand_id(&table, 1, &state, 0, out, sizeof(out)));
    assert(strcmp(out, "Visible text.") == 0);
    printf("PASS: comment_skip\n");
}

static void test_newline_caret(void) {
    const char *input = "|1[Line one.^^Line two.]";
    LibTextTable table;
    assert(lib_text_table_parse(&table, (const uint8_t *)input, strlen(input)));
    LibTextState state;
    lib_text_state_init(&state, 0);
    char out[512];
    assert(lib_text_expand_id(&table, 1, &state, 0, out, sizeof(out)));
    assert(strcmp(out, "Line one.\nLine two.") == 0);
    printf("PASS: newline_caret\n");
}

static void test_random_option(void) {
    const char *input = "|1[^O3[alpha|beta|gamma]]";
    LibTextTable table;
    assert(lib_text_table_parse(&table, (const uint8_t *)input, strlen(input)));

    int counts[3] = {0};
    for (uint32_t seed = 0; seed < 300; seed++) {
        LibTextState state;
        lib_text_state_init(&state, seed);
        char out[512];
        assert(lib_text_expand_id(&table, 1, &state, 0, out, sizeof(out)));
        if (strcmp(out, "alpha") == 0) counts[0]++;
        else if (strcmp(out, "beta") == 0) counts[1]++;
        else if (strcmp(out, "gamma") == 0) counts[2]++;
        else { printf("unexpected: '%s'\n", out); assert(0); }
    }
    assert(counts[0] > 0 && counts[1] > 0 && counts[2] > 0);
    printf("PASS: random_option (%d/%d/%d)\n", counts[0], counts[1], counts[2]);
}

static void test_case_switch(void) {
    const char *input = "|1[^XCA[case zero|case one|case two]]";
    LibTextTable table;
    assert(lib_text_table_parse(&table, (const uint8_t *)input, strlen(input)));

    for (int cv = 0; cv < 3; cv++) {
        LibTextState state;
        lib_text_state_init(&state, 0);
        char out[512];
        assert(lib_text_expand_id(&table, 1, &state, cv, out, sizeof(out)));
        char expected[32];
        snprintf(expected, sizeof(expected), "case %s",
                 cv == 0 ? "zero" : cv == 1 ? "one" : "two");
        assert(strcmp(out, expected) == 0);
    }
    printf("PASS: case_switch\n");
}

static void test_deterministic(void) {
    const char *input = "|1[^O3[a|b|c] ^N1-100]";
    LibTextTable table;
    assert(lib_text_table_parse(&table, (const uint8_t *)input, strlen(input)));

    LibTextState s1, s2;
    lib_text_state_init(&s1, 42);
    lib_text_state_init(&s2, 42);
    char out1[512], out2[512];
    assert(lib_text_expand_id(&table, 1, &s1, 0, out1, sizeof(out1)));
    assert(lib_text_expand_id(&table, 1, &s2, 0, out2, sizeof(out2)));
    assert(strcmp(out1, out2) == 0);
    printf("PASS: deterministic\n");
}

static void test_number_range(void) {
    const char *input = "|1[^N10-20 credits]";
    LibTextTable table;
    assert(lib_text_table_parse(&table, (const uint8_t *)input, strlen(input)));

    for (uint32_t seed = 0; seed < 100; seed++) {
        LibTextState state;
        lib_text_state_init(&state, seed);
        char out[512];
        assert(lib_text_expand_id(&table, 1, &state, 0, out, sizeof(out)));
        int val = 0;
        sscanf(out, "%d", &val);
        assert(val >= 10 && val <= 20);
    }
    printf("PASS: number_range\n");
}

static void test_game_data_dte(void) {
    const char *basedir = getenv("OPENCAPTIVE_TEST_DATA");
    if (!basedir) basedir = getenv("HOME");
    if (!basedir) return;

    char path[512];
    snprintf(path, sizeof(path), "%s/.opencaptive/liberation/disk3/DTE.txt", basedir);
    FILE *f = fopen(path, "rb");
    if (!f) { printf("SKIP: DTE.txt not available\n"); return; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *src = malloc(sz);
    fread(src, 1, sz, f);
    fclose(f);

    size_t dsz = arcd_decompressed_size(src, sz);
    if (!dsz) { free(src); printf("SKIP: not ArcD\n"); return; }
    uint8_t *dec = malloc(dsz);
    int r = arcd_decode(src, sz, dec, dsz);
    free(src);
    assert(r > 0);

    LibTextTable table;
    assert(lib_text_table_parse(&table, dec, r));
    printf("  DTE sections: %u\n", table.section_count);
    assert(table.section_count > 0);

    LibTextState state;
    lib_text_state_init(&state, 1337);
    char out[LIB_TEXT_MAX_OUTPUT];
    for (unsigned i = 0; i < table.section_count; i++) {
        assert(lib_text_expand(&table, &table.sections[i], &state, 0,
                               out, sizeof(out)));
        assert(strlen(out) > 0);
    }

    printf("PASS: game_data_dte (%u sections expanded)\n", table.section_count);
    free(dec);
}

int main(void) {
    test_parse_basic();
    test_find_section();
    test_expand_plain_text();
    test_comment_skip();
    test_newline_caret();
    test_random_option();
    test_case_switch();
    test_deterministic();
    test_number_range();
    test_game_data_dte();
    printf("PASS: all liberation_text_table tests\n");
    return 0;
}
