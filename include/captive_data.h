#ifndef CAPTIVE_DATA_H
#define CAPTIVE_DATA_H

#include <stdint.h>

/* Data recovered from the verified DOS executable CAPPO.EXE v1.06 (Oct 7 1992).
 * Unpacked SHA-256: fa7d5ca76d26f614476ed41f27cf737084942e9216b20b4605734df9ede9aee4
 *
 * All tables below are direct transcriptions from the binary's string and data
 * sections. No values are invented or interpolated. */

/* Droid material grades, ascending quality. Indices 1-3 are blank in the
 * original executable (reserved/unused material tiers). */
#define CAPTIVE_MATERIAL_COUNT 10
extern const char *const captive_material_names[CAPTIVE_MATERIAL_COUNT];

/* Combat skills from the original skill table. */
#define CAPTIVE_SKILL_COUNT 10
extern const char *const captive_skill_names[CAPTIVE_SKILL_COUNT];

/* Shop device names (12 purchasable devices). */
#define CAPTIVE_DEVICE_COUNT 12
extern const char *const captive_device_names[CAPTIVE_DEVICE_COUNT];

/* Body part names (6 slots). */
#define CAPTIVE_BODYPART_COUNT 6
extern const char *const captive_bodypart_names[CAPTIVE_BODYPART_COUNT];

/* Name generation syllable table: 48 syllables in 4-char slots. */
#define CAPTIVE_SYLLABLE_COUNT 48
extern const char *const captive_name_syllables[CAPTIVE_SYLLABLE_COUNT];

/* Main PRNG recovered from 0x8A8E: multiply by 0x5E5, add 0x29, ror 3, xor 0x0800. */
uint32_t captive_prng(uint32_t *state);

/* Combat PRNG variant from 0x9815: multiply by 0x5E5, add 0x29, ror 2 (no XOR). */
uint32_t captive_combat_prng(uint32_t *state);

/* Generate a droid name from the syllable table using the original algorithm.
 * Writes a NUL-terminated string of 2-3 syllables into buf (max bufsize). */
void captive_generate_name(uint32_t *prng_state, char *buf, int bufsize);

/* Game message strings from the DOS executable. */
#define CAPTIVE_MSG_COUNT 25
extern const char *const captive_messages[CAPTIVE_MSG_COUNT];

/* Indices into captive_messages[].  These are the original strings, so screens
 * that have one must use it rather than inventing replacement wording. */
#define CAPTIVE_MSG_DESTROY_GENERATORS 0
#define CAPTIVE_MSG_BASE_DESTROYED     3
#define CAPTIVE_MSG_TRILL_RESCUED      4
#define CAPTIVE_MSG_DROIDS_FAILED      5
#define CAPTIVE_MSG_PRESS_TO_CONTINUE 10
#define CAPTIVE_MSG_LET_BATTLE        11

/* Shop dialogue strings from the DOS executable. */
#define CAPTIVE_SHOP_MSG_COUNT 13
extern const char *const captive_shop_messages[CAPTIVE_SHOP_MSG_COUNT];

/* Music track categories from the DOS executable. */
#define CAPTIVE_MUSIC_CATEGORY_COUNT 14
extern const char *const captive_music_categories[CAPTIVE_MUSIC_CATEGORY_COUNT];

/* Graphics file names from the DOS executable (23 PL5 files). */
extern const char *const *captive_graphics_files_ptr;
extern const int captive_graphics_file_count;

/* Planet/station name generation from the DOS executable.
 * Format: "STATION " + generated suffix from consonant/vowel tables. */
void captive_generate_planet_name(uint32_t *prng_state, char *buf, int bufsize);

#endif
