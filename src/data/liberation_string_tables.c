#include "liberation_data.h"

/* String tables extracted from the verified CD32 executable
 * (SHA-256 db61f7e39fd31ac19b82216ea963711728d25518454fae42fd89c5bab52f2215). */

const char *const liberation_city_syllables[LIBERATION_CITY_SYLLABLE_COUNT] = {
    "gold", "grun", "braun", "wein", "eisen", "ein", "bieder", "jaeger",
    "tell", "fern", "hahn", "stein", "mann", "berg", "er", "mensch",
    "tod", "fisch", "rhein", "kurz", "laben", "weg", "wehr", "kropf",
    "esel", "erz", "fach", "fuchs", "schild", "ort", "paar", "platz",
};

const char *const liberation_street_types[LIBERATION_STREET_TYPE_COUNT] = {
    "Street", "Avenue", "Road", "Boulevard", "Lane",
    "Park", "Gardens", "Gate", "Crescent", "Mews",
    "Terrace", "Drive", "Alley", "Approach", "Arcade",
    "Buildings", "Corner", "Quadrant", "Passage", "Promenade",
};

const char *const liberation_first_names[LIBERATION_FIRST_NAME_COUNT] = {
    "Max", "Joe", "Jack", "Jim", "Ed", "Ann", "June", "Fred",
    "Kiku", "Art", "John", "Barbara", "Steve", "Rick", "Mark",
    "Barry", "Deb", "Malc", "Tone", "Pete", "Alison", "Maggi",
    "Claire", "Kim", "Cass", "Lynne", "Mel", "Gill", "Cath",
    "Ivor", "Huw", "Gwynn", "Geraint", "Narish", "Rajiv",
};

const char *const liberation_last_names[LIBERATION_LAST_NAME_COUNT] = {
    "featherstonehaugh", "patel", "smythe", "singh", "meridew",
    "sagan", "murray", "cherryh", "chambers", "gaiman",
    "cleese", "simon", "burkinshaw", "forward", "rector",
    "alexy", "jacobs", "crowther", "macleod", "leary",
    "crowley", "gibson", "floyd", "whittle", "pitt",
    "heath", "goodley", "chapman", "palin", "gilliam",
    "hake", "turbot",
};

const char *const liberation_npc_titles[LIBERATION_NPC_TITLE_COUNT] = {
    "Governor", "Councillor", "Investment Counsellor", "Engineer",
    "Entrepreneur", "Pilot", "Trader", "Merchant",
};

const char *const liberation_shop_types[LIBERATION_SHOP_TYPE_COUNT] = {
    "Emporium", "Hardware", "Pawnshop", "Gunsmith", "Jewellers",
    "Store", "Market", "Newsagent", "TriDee Rental",
};

const char *const liberation_bar_types[LIBERATION_BAR_TYPE_COUNT] = {
    "Bar", "Boozerama", "Diner", "PopShop", "Pub", "Vinyard",
    "Beer Hall", "Pool Rooms", "Speakeasy", "Gin Palace",
    "cha no mise", "biru no mise",
};

const char *const liberation_business_types[LIBERATION_BUSINESS_TYPE_COUNT] = {
    "Software", "Insurance", "Assurance", "Bank", "Accountants",
    "Actuarial Services", "Credit Control", "Mercantile",
    "Import/Export", "Productions", "Expediters",
};

const char *const liberation_industrial_types[LIBERATION_INDUSTRIAL_TYPE_COUNT] = {
    "Engineering", "Ceramics", "Steel", "Aluminium Corp.",
    "Laser Systems", "TeleCommunications", "Plastics", "Mouldings",
    "Metals a.g.", "Optik g.m.b.h", "ParaGravitics", "Noble Gasses",
};
