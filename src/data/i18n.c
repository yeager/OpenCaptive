#include "i18n.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static I18nTable table;
static bool initialized;

static void parse_po_line(const char *line, char *out, size_t out_size) {
    const char *start = strchr(line, '"');
    if (!start) { out[0] = '\0'; return; }
    start++;
    size_t i = 0;
    while (*start && *start != '"' && i < out_size - 1) {
        if (*start == '\\' && *(start + 1)) {
            start++;
            switch (*start) {
                case 'n': out[i++] = '\n'; break;
                case 't': out[i++] = '\t'; break;
                case '\\': out[i++] = '\\'; break;
                case '"': out[i++] = '"'; break;
                default: out[i++] = '\\'; out[i++] = *start; break;
            }
        } else {
            out[i++] = *start;
        }
        start++;
    }
    out[i] = '\0';
}

static void load_po(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[1024];
    char msgid[I18N_MAX_MSGLEN] = "";
    char msgstr[I18N_MAX_MSGLEN] = "";
    enum { NONE, IN_MSGID, IN_MSGSTR } state = NONE;

    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        if (line[0] == '#' || (line[0] == '\0' && state == NONE))
            continue;

        if (strncmp(line, "msgid ", 6) == 0) {
            if (state == IN_MSGSTR && msgid[0] && msgstr[0] &&
                table.count < I18N_MAX_ENTRIES) {
                strncpy(table.entries[table.count].msgid, msgid, I18N_MAX_MSGLEN - 1);
                strncpy(table.entries[table.count].msgstr, msgstr, I18N_MAX_MSGLEN - 1);
                table.count++;
            }
            parse_po_line(line, msgid, sizeof(msgid));
            msgstr[0] = '\0';
            state = IN_MSGID;
        } else if (strncmp(line, "msgstr ", 7) == 0) {
            parse_po_line(line, msgstr, sizeof(msgstr));
            state = IN_MSGSTR;
        } else if (line[0] == '"') {
            char extra[I18N_MAX_MSGLEN];
            parse_po_line(line, extra, sizeof(extra));
            if (state == IN_MSGID) {
                size_t cur = strlen(msgid);
                strncat(msgid, extra, I18N_MAX_MSGLEN - cur - 1);
            } else if (state == IN_MSGSTR) {
                size_t cur = strlen(msgstr);
                strncat(msgstr, extra, I18N_MAX_MSGLEN - cur - 1);
            }
        } else if (line[0] == '\0') {
            if (state == IN_MSGSTR && msgid[0] && msgstr[0] &&
                table.count < I18N_MAX_ENTRIES) {
                strncpy(table.entries[table.count].msgid, msgid, I18N_MAX_MSGLEN - 1);
                strncpy(table.entries[table.count].msgstr, msgstr, I18N_MAX_MSGLEN - 1);
                table.count++;
            }
            msgid[0] = '\0';
            msgstr[0] = '\0';
            state = NONE;
        }
    }

    if (state == IN_MSGSTR && msgid[0] && msgstr[0] &&
        table.count < I18N_MAX_ENTRIES) {
        strncpy(table.entries[table.count].msgid, msgid, I18N_MAX_MSGLEN - 1);
        strncpy(table.entries[table.count].msgstr, msgstr, I18N_MAX_MSGLEN - 1);
        table.count++;
    }

    fclose(f);
}

static const char *detect_language(void) {
    SDL_Locale **locales = SDL_GetPreferredLocales(NULL);
    if (locales && locales[0] && locales[0]->language) {
        static char lang[16];
        snprintf(lang, sizeof(lang), "%s", locales[0]->language);
        SDL_free(locales);
        return lang;
    }
    if (locales) SDL_free(locales);
    return "en";
}

void i18n_init(const char *lang_override) {
    memset(&table, 0, sizeof(table));
    initialized = true;

    const char *lang = lang_override ? lang_override : detect_language();
    snprintf(table.lang, sizeof(table.lang), "%s", lang);

    if (strcmp(lang, "en") == 0)
        return;

    char path[512];
    const char *base = SDL_GetBasePath();
    if (base) {
        snprintf(path, sizeof(path), "%spo/%s.po", base, lang);
        load_po(path);
        if (table.count > 0) return;
    }

    snprintf(path, sizeof(path), "po/%s.po", lang);
    load_po(path);
}

void i18n_free(void) {
    memset(&table, 0, sizeof(table));
    initialized = false;
}

const char *i18n_get(const char *msgid) {
    if (!msgid) return "";
    if (!initialized || table.count == 0) return msgid;
    for (int i = 0; i < table.count; i++) {
        if (strcmp(table.entries[i].msgid, msgid) == 0)
            return table.entries[i].msgstr;
    }
    return msgid;
}

const char *i18n_get_lang(void) {
    return initialized ? table.lang : "en";
}
