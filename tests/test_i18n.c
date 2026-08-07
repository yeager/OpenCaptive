#include "i18n.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int tests_run = 0;

#define TEST(name) do { printf("  %s... ", #name); name(); printf("ok\n"); tests_run++; } while(0)

static void test_english_passthrough(void) {
    i18n_init("en");
    assert(strcmp(i18n_get("SETTINGS"), "SETTINGS") == 0);
    assert(strcmp(i18n_get("QUIT"), "QUIT") == 0);
    assert(strcmp(_("ON"), "ON") == 0);
    i18n_free();
}

static void test_null_returns_empty(void) {
    i18n_init("en");
    assert(strcmp(i18n_get(NULL), "") == 0);
    i18n_free();
}

static void test_unknown_lang_passthrough(void) {
    i18n_init("xx");
    assert(strcmp(i18n_get_lang(), "en") == 0);
    assert(strcmp(i18n_get("SETTINGS"), "SETTINGS") == 0);
    i18n_free();
}

static void test_language_path_is_sanitized(void) {
    i18n_init("../sv");
    assert(strcmp(i18n_get_lang(), "en") == 0);
    assert(strcmp(i18n_get("SETTINGS"), "SETTINGS") == 0);
    i18n_free();
}

static void test_regional_language_is_normalized(void) {
    i18n_init("sv-SE");
    assert(strcmp(i18n_get_lang(), "sv") == 0);
    i18n_free();
}

static void test_get_lang(void) {
    i18n_init("sv");
    assert(strcmp(i18n_get_lang(), "sv") == 0);
    i18n_free();

    i18n_init("en");
    assert(strcmp(i18n_get_lang(), "en") == 0);
    i18n_free();
}

static void test_macro_works(void) {
    i18n_init("en");
    const char *s = _("QUIT");
    assert(strcmp(s, "QUIT") == 0);
    i18n_free();
}

static void test_holomap_strings_are_catalogued(void) {
    i18n_init("sv");
    assert(strcmp(_("MISSION COMPLETE!"), "UPPDRAG KLART!") == 0);
    assert(strcmp(_("Next: Mission %d of 10"),
                  "Nasta: Uppdrag %d av 10") == 0);
    assert(strcmp(_("ENTER: Launch    S: Shop"),
                  "ENTER: Starta    S: Butik") == 0);
    i18n_free();

    i18n_init("fr");
    assert(strcmp(_("MISSION COMPLETE!"), "MISSION TERMINEE !") == 0);
    i18n_free();
}

static void test_droid_ui_strings_are_catalogued(void) {
    i18n_init("sv");
    assert(strcmp(_("EQUIPMENT"), "UTRUSTNING") == 0);
    assert(strcmp(_("EMPTY"), "TOM") == 0);
    assert(strcmp(_("TAB:SWITCH ENTER:EQUIP/USE ESC:CLOSE"),
                  "TAB:BYT ENTER:UTRUSTA/ANVAND ESC:STANG") == 0);
    i18n_free();
}

int main(void) {
    printf("i18n tests:\n");
    TEST(test_english_passthrough);
    TEST(test_null_returns_empty);
    TEST(test_unknown_lang_passthrough);
    TEST(test_language_path_is_sanitized);
    TEST(test_regional_language_is_normalized);
    TEST(test_get_lang);
    TEST(test_macro_works);
    TEST(test_holomap_strings_are_catalogued);
    TEST(test_droid_ui_strings_are_catalogued);
    printf("%d tests passed\n", tests_run);
    return 0;
}
