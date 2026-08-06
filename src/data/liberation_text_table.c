#include "liberation_text_table.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#define LIB_TEXT_MAX_RECURSION 64U

static uint32_t text_prng(uint32_t *state) {
    *state = *state * 0x5E5 + 0x29;
    return *state >> 8;
}

bool lib_text_table_parse(LibTextTable *table, const uint8_t *data, size_t size) {
    if (!table || !data || !size) return false;
    memset(table, 0, sizeof(*table));
    table->raw = data;
    table->raw_size = size;

    const char *text = (const char *)data;
    size_t i = 0;

    while (i < size && table->section_count < LIB_TEXT_MAX_SECTIONS) {
        if (text[i] == '|' && i + 1 < size && isdigit((unsigned char)text[i + 1])) {
            unsigned id = 0;
            size_t j = i + 1;
            while (j < size && isdigit((unsigned char)text[j])) {
                unsigned digit = (unsigned)(text[j] - '0');
                if (id > (UINT_MAX - digit) / 10U) return false;
                id = id * 10U + digit;
                j++;
            }
            if (j < size && text[j] == '[') {
                j++;
                size_t start = j;
                int depth = 1;
                while (j < size && depth > 0) {
                    if (text[j] == '[') depth++;
                    else if (text[j] == ']') depth--;
                    if (depth > 0) j++;
                }
                if (depth != 0) break;
                LibTextSection *sec = &table->sections[table->section_count++];
                sec->id = id;
                sec->start = text + start;
                sec->length = j - start;
                i = j + 1;
                continue;
            }
        }
        i++;
    }
    return table->section_count > 0;
}

const LibTextSection *lib_text_table_find(const LibTextTable *table, unsigned id) {
    if (!table) return NULL;
    for (unsigned i = 0; i < table->section_count; i++) {
        if (table->sections[i].id == id)
            return &table->sections[i];
    }
    return NULL;
}

void lib_text_state_init(LibTextState *state, uint32_t seed) {
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->prng_state = seed;
}

typedef struct {
    char *out;
    size_t pos;
    size_t cap;
} OutBuf;

static void emit(OutBuf *buf, char c) {
    if (buf->pos + 1 < buf->cap)
        buf->out[buf->pos++] = c;
}

static void emit_str(OutBuf *buf, const char *s) {
    while (*s) emit(buf, *s++);
}

static int parse_int(const char *s, size_t len, size_t *pos) {
    int sign = 1, val = 0;
    if (*pos < len && s[*pos] == '-') { sign = -1; (*pos)++; }
    while (*pos < len && isdigit((unsigned char)s[*pos])) {
        int digit = s[*pos] - '0';
        if (val > (INT_MAX - digit) / 10) {
            val = INT_MAX;
        } else {
            val = val * 10 + digit;
        }
        (*pos)++;
    }
    if (sign < 0 && val == INT_MAX) return INT_MIN;
    return sign < 0 ? -val : val;
}

static void expand_range(const char *s, size_t len, size_t *pos,
                         LibTextState *state, OutBuf *buf) {
    size_t p = *pos;
    int lo = parse_int(s, len, &p);
    if (p < len && s[p] == '-') { p++; }
    int hi = parse_int(s, len, &p);
    if (hi < lo) hi = lo;
    int64_t range = (int64_t)hi - (int64_t)lo + 1;
    int val = lo;
    if (range > 0) {
        int64_t offset = (int64_t)(text_prng(&state->prng_state) %
                                   (uint32_t)(range > UINT32_MAX ? UINT32_MAX : range));
        val = (int)((int64_t)lo + offset);
    }
    char num[16];
    snprintf(num, sizeof(num), "%d", val);
    emit_str(buf, num);
    *pos = p;
}

static void process_text(const char *s, size_t len, LibTextState *state,
                         int case_value, OutBuf *buf, unsigned depth);

static void process_random_option(const char *s, size_t len, size_t *pos,
                                  LibTextState *state, OutBuf *buf,
                                  unsigned recursion_depth) {
    size_t p = *pos;
    if (p >= len || s[p] != '[') { *pos = p; return; }
    p++;

    size_t options[32];
    unsigned count = 0;
    options[0] = p;
    count = 1;

    int depth = 0;
    size_t scan = p;
    while (scan < len) {
        if (s[scan] == '[') depth++;
        else if (s[scan] == ']') {
            if (depth == 0) break;
            depth--;
        } else if (s[scan] == '|' && depth == 0 && count < 32) {
            options[count++] = scan + 1;
        }
        scan++;
    }

    unsigned pick = text_prng(&state->prng_state) % count;

    size_t opt_start = options[pick];
    size_t opt_end;
    if (pick + 1 < count)
        opt_end = options[pick + 1] - 1;
    else
        opt_end = scan;

    if (opt_start <= opt_end && recursion_depth < LIB_TEXT_MAX_RECURSION)
        process_text(s + opt_start, opt_end - opt_start, state, 0, buf,
                     recursion_depth + 1U);

    *pos = (scan < len) ? scan + 1 : scan;
}

static void process_case(const char *s, size_t len, size_t *pos,
                         int case_value, LibTextState *state, OutBuf *buf,
                         unsigned recursion_depth) {
    size_t p = *pos;
    if (p >= len || s[p] != '[') { *pos = p; return; }
    p++;

    int current_case = 0;
    size_t case_start = p;
    int depth = 0;
    bool found = false;

    while (p < len) {
        if (s[p] == '[') depth++;
        else if (s[p] == ']') {
            if (depth == 0) {
                if (current_case == case_value && !found) {
                    if (recursion_depth < LIB_TEXT_MAX_RECURSION)
                        process_text(s + case_start, p - case_start, state, 0,
                                     buf, recursion_depth + 1U);
                    found = true;
                }
                p++;
                break;
            }
            depth--;
        } else if (s[p] == '|' && depth == 0) {
            if (current_case == case_value && !found) {
                if (recursion_depth < LIB_TEXT_MAX_RECURSION)
                    process_text(s + case_start, p - case_start, state, 0, buf,
                                 recursion_depth + 1U);
                found = true;
            }
            current_case++;
            case_start = p + 1;
        }
        p++;
    }
    *pos = p;
}

static void process_text(const char *s, size_t len, LibTextState *state,
                         int case_value, OutBuf *buf, unsigned depth) {
    size_t i = 0;
    while (i < len) {
        if (s[i] == '^' && i + 1 < len) {
            i++;
            switch (s[i]) {
            case ';':
                while (i < len && s[i] != '\n' && s[i] != '\r') i++;
                while (i < len && (s[i] == '\n' || s[i] == '\r')) i++;
                break;
            case '^':
                emit(buf, '\n');
                i++;
                break;
            case 'O': {
                i++;
                while (i < len && isdigit((unsigned char)s[i])) i++;
                process_random_option(s, len, &i, state, buf, depth);
                break;
            }
            case 'N': {
                i++;
                expand_range(s, len, &i, state, buf);
                break;
            }
            case 'X': {
                i++;
                if (i < len && s[i] == 'C' && i + 1 < len && s[i + 1] == 'A') {
                    i += 2;
                    process_case(s, len, &i, case_value, state, buf, depth);
                } else if (i < len && s[i] == 'O') {
                    i++;
                    while (i < len && isdigit((unsigned char)s[i])) i++;
                    process_random_option(s, len, &i, state, buf, depth);
                } else {
                    while (i < len && s[i] != '\r' && s[i] != '\n'
                           && s[i] != '[' && s[i] != ']' && s[i] != '|') i++;
                }
                break;
            }
            case 'S': {
                i++;
                while (i < len && s[i] != '\r' && s[i] != '\n') i++;
                while (i < len && (s[i] == '\r' || s[i] == '\n')) i++;
                break;
            }
            case 'J':
                i++;
                emit_str(buf, "trader");
                break;
            case 'P':
                i++;
                if (state->person_name)
                    emit_str(buf, state->person_name);
                else
                    emit_str(buf, "someone");
                break;
            case 'W':
            case 'Z':
            case 'A':
                i++;
                break;
            case 'F': {
                i++;
                while (i < len && s[i] != '\r' && s[i] != '\n'
                       && s[i] != '[' && s[i] != ']' && s[i] != '|') i++;
                break;
            }
            case 'I':
            case 'L':
            case 'E':
            case 'G': {
                i++;
                while (i < len && s[i] != '\r' && s[i] != '\n'
                       && s[i] != '[' && s[i] != ']' && s[i] != '|') i++;
                break;
            }
            default:
                emit(buf, '^');
                break;
            }
        } else if (s[i] == '\r') {
            i++;
            if (i < len && s[i] == '\n') i++;
            emit(buf, ' ');
        } else if (s[i] == '\n') {
            emit(buf, ' ');
            i++;
        } else {
            emit(buf, s[i]);
            i++;
        }
    }
}

bool lib_text_expand(const LibTextTable *table, const LibTextSection *section,
                     LibTextState *state, int case_value,
                     char *out, size_t out_size) {
    if (!table || !section || !state || !out || out_size < 2) return false;
    if (!table->raw || section->length > table->raw_size) return false;
    uintptr_t raw_start = (uintptr_t)table->raw;
    uintptr_t section_start = (uintptr_t)section->start;
    if (section_start < raw_start ||
        section_start - raw_start > table->raw_size - section->length)
        return false;
    OutBuf buf = { out, 0, out_size };
    process_text(section->start, section->length, state, case_value, &buf, 0U);
    out[buf.pos] = '\0';
    return buf.pos > 0;
}

bool lib_text_expand_id(const LibTextTable *table, unsigned section_id,
                        LibTextState *state, int case_value,
                        char *out, size_t out_size) {
    const LibTextSection *sec = lib_text_table_find(table, section_id);
    if (!sec) return false;
    return lib_text_expand(table, sec, state, case_value, out, out_size);
}
