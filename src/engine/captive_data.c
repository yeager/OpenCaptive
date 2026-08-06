#include "captive_data.h"
#include <string.h>

const char *const captive_material_names[CAPTIVE_MATERIAL_COUNT] = {
    "SHIT", "", "", "", "HUMAN",
    "TINDRON", "COPPATOR", "BRONZITE", "IRONIDE", "CROMIZE",
};

const char *const captive_skill_names[CAPTIVE_SKILL_COUNT] = {
    "ROBOTICS", "BRAWLING", "SWORDS", "HANDGUNS", "RIFLES",
    "AUTOMATICS", "LASERS", "CANNONS", "SPAYGUNS", "EXPERIENCE",
};

const char *const captive_device_names[CAPTIVE_DEVICE_COUNT] = {
    "AG-SCAN", "ROOT-FINDER", "MAPPER", "RADAR",
    "MAGNA-SCAN", "BODY-SCAN", "VISION-CORRECTOR", "VISOR",
    "ANTI-GRAV", "SHIELD", "FIRE-SHIELD", "GREASER",
};

const char *const captive_bodypart_names[CAPTIVE_BODYPART_COUNT] = {
    "HEAD", "CHEST", "ARM", "LEG", "FOOT", "HAND",
};

const char *const captive_name_syllables[CAPTIVE_SYLLABLE_COUNT] = {
    "VI",   "RUP",  "YUL",  "SCO",  "PHY",  "RAT",
    "QUE",  "CHA",  "SY",   "POC",  "E",    "EX",
    "DE",   "LAP",  "EL",   "MID",  "SO",   "SI",
    "LE",   "NE",   "SIC",  "THA",  "ENE",  "INS",
    "OO",   "ES",   "GIN",  "CEP",  "LTE",  "PE",
    "DER",  "S",    "DON",  "S",    "ING",  "ST",
    "Y",    "ED",   "BERY", "SY",   "LUME", "TON",
    "FAR",  "HAM",  "KAL",  "APE",  "BEE",  "INK",
};

uint32_t captive_prng(uint32_t *state) {
    if (!state) return 0;
    uint16_t s = (uint16_t)*state;
    s = (uint16_t)(s * 0x5e5 + 0x29);
    s = (s >> 3) | (s << 13);
    s ^= 0x0800;
    *state = s;
    return s;
}

uint32_t captive_combat_prng(uint32_t *state) {
    if (!state) return 0;
    uint16_t s = (uint16_t)*state;
    s = (uint16_t)(s * 0x5e5 + 0x29);
    s = (s >> 2) | (s << 14);
    *state = s;
    return s;
}

void captive_generate_name(uint32_t *prng_state, char *buf, int bufsize) {
    if (!buf || bufsize < 1) return;
    buf[0] = '\0';

    int syllable_count = 2 + (captive_prng(prng_state) % 2);
    int pos = 0;

    for (int i = 0; i < syllable_count; i++) {
        int idx = captive_prng(prng_state) % CAPTIVE_SYLLABLE_COUNT;
        const char *syl = captive_name_syllables[idx];
        int len = (int)strlen(syl);
        if (pos + len >= bufsize - 1) break;
        memcpy(buf + pos, syl, len);
        pos += len;
    }
    buf[pos] = '\0';
}

const char *const captive_messages[CAPTIVE_MSG_COUNT] = {
    "DESTROY THE GENERATORS.",
    "TOP-LEFT, BOTTOM-LEFT, TOP-RIGHT, BOTTOM-RIGHT.",
    "PUT PLANET-PROBE IN HOLAMAP.",
    "BASE HAS BEEN DESTROYED!",
    "TRILL HAS BEEN RESCUED!",
    "DROID'S HAVE FAILED!",
    "TRILL HAS BEEN LEFT TO DIE.",
    "DROID'S ARE TO BE WASTED",
    "TRILL HAS BE KILLED!",
    "DROIDS BRUTALLY OUTWITTED",
    "PRESS MOUSE TO CONTINUE",
    "LET BATTLE COMMENCE",
    "CAPTIVE MISSION 0001",
    "LEGEND OF TRILL:",
    "A SMALL PARTY OF FOUR",
    "SWAN NOT YET IN ORBIT",
    "LANDER NOT IN DOCK",
    "LANDING SUCCESSFUL",
    "DOCKING COMPLETE",
    "LANDER LAUNCHED",
    "FLIGHT PATH SET",
    "ARRIVED AT DESTINATION",
    "PROBE REVEALS A BASE",
    "PROBE LAUNCHED",
    "MESSAGE FROM RATT",
};

const char *const captive_shop_messages[CAPTIVE_SHOP_MSG_COUNT] = {
    "WELCOME STRANGER TO MY",
    "'HUMBLE SHOP",
    "HOW MAY I BE OF ASSISTANCE",
    "CALL AGAIN LATER",
    "THIS WILL COST YOU",
    "FOR NEXT LEVEL IN",
    "YOU DO NOT HAVE ENOUGH!",
    "ACCEPT",
    "I DON'T STOCK THIS OBJECT",
    "MODEL OF",
    "PLEASE REMOVE",
    "GRADE OF THE",
    "REPAIR",
};

const char *const captive_music_categories[CAPTIVE_MUSIC_CATEGORY_COUNT] = {
    "BATT", "COMPROOM", "ESCAPED", "FCBASE", "FINAL2",
    "GENBASE", "HOLOMAP", "LONGNT", "MAIN2", "RUNNING",
    "SHOPKEEP", "TRAPPED", "VCBASE", "W",
};

static const char *const graphics_files[] = {
    "GAMESCRN.PL5", "ROOFS.PL5",
    "WALLA.PL5", "WALLB.PL5", "WALLC.PL5", "WALLD.PL5", "WALLE.PL5",
    "DOORS1.PL5", "DOORS2.PL5",
    "SHOP1.PL5", "SHOP2.PL5",
    "ICONS.PL5", "OBJECTS.PL5",
    "ALIEN1.PL5", "ALIEN2.PL5", "ALIEN3.PL5",
    "ALIEN4.PL5", "ALIEN5.PL5", "ALIEN6.PL5",
    "ANIM1.PL5", "ANIM2.PL5", "ANIM3.PL5",
    "KEYBOARD.PL5",
};
const char *const *captive_graphics_files_ptr = graphics_files;
const int captive_graphics_file_count = sizeof(graphics_files) / sizeof(graphics_files[0]);

/* Planet name generation tables from the DOS executable.
 * The original builds names like "STATION ZQTR" from these character sets. */
static const char planet_consonants[] = "Z";
static const char planet_vowels1[]    = "QUP";
static const char planet_vowels2[]    = "TRSLM";
static const char planet_vowels3[]    = "B";
static const char planet_suffix1[]    = "GRL";
static const char planet_suffix2[]    = "S";

void captive_generate_planet_name(uint32_t *prng_state, char *buf, int bufsize) {
    if (!buf || bufsize < 1) return;
    const char *prefix = "STATION ";
    int pos = 0;
    for (int i = 0; prefix[i] && pos < bufsize - 1; i++)
        buf[pos++] = prefix[i];

    int suffix_len = 3 + (captive_prng(prng_state) % 3);
    for (int i = 0; i < suffix_len && pos < bufsize - 1; i++) {
        const char *set;
        int set_len;
        switch (i % 4) {
        case 0: set = planet_consonants; set_len = (int)sizeof(planet_consonants) - 1; break;
        case 1: set = planet_vowels1;    set_len = (int)sizeof(planet_vowels1) - 1;    break;
        case 2: set = planet_vowels2;    set_len = (int)sizeof(planet_vowels2) - 1;    break;
        default: set = planet_vowels3;   set_len = (int)sizeof(planet_vowels3) - 1;    break;
        }
        if (set_len > 0)
            buf[pos++] = set[captive_prng(prng_state) % set_len];
    }
    if (captive_prng(prng_state) % 2 && pos < bufsize - 1) {
        int s1_len = (int)sizeof(planet_suffix1) - 1;
        if (s1_len > 0)
            buf[pos++] = planet_suffix1[captive_prng(prng_state) % s1_len];
    }
    if (captive_prng(prng_state) % 3 == 0 && pos < bufsize - 1) {
        int s2_len = (int)sizeof(planet_suffix2) - 1;
        if (s2_len > 0)
            buf[pos++] = planet_suffix2[captive_prng(prng_state) % s2_len];
    }
    buf[pos] = '\0';
}
