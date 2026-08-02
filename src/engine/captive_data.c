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
    uint32_t s = *state;
    s = s * 0x5e5 + 0x29;
    s = (s >> 4) | (s << 28);
    s ^= 0x0800;
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
