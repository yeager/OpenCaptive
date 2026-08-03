#ifndef OPENCAPTIVE_I18N_H
#define OPENCAPTIVE_I18N_H

#include <stddef.h>

#define _(s) i18n_get(s)

void i18n_init(const char *lang_override);
void i18n_free(void);
const char *i18n_get(const char *msgid);
const char *i18n_get_lang(void);

#define I18N_MAX_ENTRIES 512
#define I18N_MAX_MSGLEN 256

typedef struct {
    char msgid[I18N_MAX_MSGLEN];
    char msgstr[I18N_MAX_MSGLEN];
} I18nEntry;

typedef struct {
    char lang[16];
    I18nEntry entries[I18N_MAX_ENTRIES];
    int count;
} I18nTable;

#endif
