#include "liberation_cte.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

bool cte_table_parse(CteTable *table, const uint8_t *data, size_t size) {
    if (!table || !data || !size) return false;
    memset(table, 0, sizeof(*table));
    table->raw = data;
    table->raw_size = size;

    /* Walk the buffer tracking bracket depth.  A '[' at depth 0 opens a
     * section; its content runs to the matching ']'.  The section's id token
     * sits three bytes before the '[' when the middle byte is the 0x00 frame
     * separator (id, 0x00, len, '['), which is how every section after the
     * leading preamble is framed. */
    int depth = 0;
    size_t i = 0;
    while (i < size) {
        if (data[i] == '[') {
            if (depth == 0) {
                /* find matching ']' */
                int d = 1;
                size_t j = i + 1;
                while (j < size && d > 0) {
                    if (data[j] == '[') d++;
                    else if (data[j] == ']') d--;
                    if (d > 0) j++;
                }
                if (d != 0) break; /* unbalanced: stop */
                if (i >= 3 && data[i - 2] == 0x00 &&
                    table->section_count < CTE_MAX_SECTIONS) {
                    CteSection *s = &table->sections[table->section_count++];
                    s->id = data[i - 3];
                    s->content = data + i + 1;
                    s->length = j - (i + 1);
                }
                i = j + 1;
                depth = 0;
                continue;
            }
            depth++;
        } else if (data[i] == ']') {
            if (depth > 0) depth--;
        }
        i++;
    }
    return table->section_count > 0;
}

const CteSection *cte_section_find(const CteTable *table, uint16_t id) {
    if (!table) return NULL;
    for (unsigned i = 0; i < table->section_count; i++)
        if (table->sections[i].id == id)
            return &table->sections[i];
    return NULL;
}

/* --- Bytecode interpreter --------------------------------------------------
 *
 * A read-only expander: it emits the authentic text a section produces for a
 * given state, evaluating conditionals and case switches, and performs none of
 * the script's side effects.  Opcodes it does not emit text for are skipped by
 * consuming their operand bytes.
 */

#define CTE_MAX_RECURSION 48U

static uint32_t cte_prng(uint32_t *s) {
    *s = *s * 0x5E5u + 0x29u;
    return *s >> 8;
}

void cte_state_init(CteState *state, uint32_t seed) {
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->prng_state = seed;
}

void cte_state_set(CteState *state, const char *name, int value) {
    if (!state || !name) return;
    for (unsigned i = 0; i < state->count; i++) {
        if (strncmp(state->vars[i].name, name, sizeof(state->vars[i].name)) == 0) {
            state->vars[i].value = value;
            return;
        }
    }
    if (state->count >= CTE_MAX_VARS) return;
    snprintf(state->vars[state->count].name, sizeof(state->vars[0].name), "%s", name);
    state->vars[state->count].value = value;
    state->count++;
}

int cte_state_get(const CteState *state, const char *name) {
    if (!state || !name) return 0;
    for (unsigned i = 0; i < state->count; i++)
        if (strncmp(state->vars[i].name, name, sizeof(state->vars[i].name)) == 0)
            return state->vars[i].value;
    return 0;  /* unset flags read as 0 (the initial-interaction state) */
}

typedef struct { char *out; size_t pos, cap; } CteOut;

static void cte_emit(CteOut *o, char c) {
    if (o->pos + 1 < o->cap) o->out[o->pos++] = c;
}

/* Read a script variable name: a run of letters. */
static void read_var(const char *s, size_t len, size_t *p, char *name, size_t nsz) {
    size_t k = 0;
    while (*p < len && isalpha((unsigned char)s[*p])) {
        if (k + 1 < nsz) name[k++] = s[*p];
        (*p)++;
    }
    name[k] = '\0';
}

static int read_int(const char *s, size_t len, size_t *p) {
    int sign = 1, v = 0;
    if (*p < len && s[*p] == '-') { sign = -1; (*p)++; }
    while (*p < len && isdigit((unsigned char)s[*p])) {
        v = v * 10 + (s[*p] - '0');
        (*p)++;
    }
    return sign * v;
}

/* Evaluate one term: <~?var><op><int>.  A leading '~' negates the term.
 * op is '=', '<' or '>'. */
static bool eval_term(const char *s, size_t len, size_t *p, const CteState *st) {
    bool negate = false;
    if (*p < len && s[*p] == '~') { negate = true; (*p)++; }
    char name[8];
    read_var(s, len, p, name, sizeof(name));
    char op = 0;
    if (*p < len && (s[*p] == '=' || s[*p] == '<' || s[*p] == '>')) op = s[(*p)++];
    int rhs = read_int(s, len, p);
    int lhs = cte_state_get(st, name);
    bool r;
    switch (op) {
        case '=': r = (lhs == rhs); break;
        case '<': r = (lhs < rhs);  break;
        case '>': r = (lhs > rhs);  break;
        default:  r = (lhs != 0);   break;  /* bare variable: truthy */
    }
    return negate ? !r : r;
}

/* Evaluate a condition that follows ^XI, up to (but not consuming) the '['.
 * Supports a parenthesised list combined with '!' (OR) or '&' (AND), or a
 * single bare term. */
static bool eval_condition(const char *s, size_t len, size_t *p, const CteState *st) {
    if (*p < len && s[*p] == '(') {
        (*p)++;
        bool acc = false;
        bool have = false;
        char combiner = 0;  /* '!'=OR, '&'=AND */
        while (*p < len && s[*p] != ')') {
            bool t = eval_term(s, len, p, st);
            if (!have) { acc = t; have = true; }
            else if (combiner == '&') acc = acc && t;
            else acc = acc || t;  /* default and '!' both OR here */
            if (*p < len && (s[*p] == '!' || s[*p] == '&')) combiner = s[(*p)++];
            else break;
        }
        if (*p < len && s[*p] == ')') (*p)++;
        return acc;
    }
    return eval_term(s, len, p, st);
}

/* Skip a bracket-balanced group starting at s[*p]=='['; leaves *p after ']'. */
static void skip_group(const char *s, size_t len, size_t *p) {
    if (*p >= len || s[*p] != '[') return;
    int depth = 0;
    while (*p < len) {
        if (s[*p] == '[') depth++;
        else if (s[*p] == ']') { depth--; if (depth == 0) { (*p)++; return; } }
        (*p)++;
    }
}

/* Find the top-level branch boundaries of a "[a|b|c]" group starting at
 * s[*p]=='['.  Fills starts[]/ends[] and returns the branch count; advances
 * *p past the closing ']'. */
static unsigned split_branches(const char *s, size_t len, size_t *p,
                               size_t *starts, size_t *ends, unsigned maxb) {
    if (*p >= len || s[*p] != '[') return 0;
    (*p)++;
    unsigned n = 0;
    starts[0] = *p; n = 1;
    int depth = 0;
    while (*p < len) {
        char c = s[*p];
        if (c == '[') depth++;
        else if (c == ']') {
            if (depth == 0) { ends[n - 1] = *p; (*p)++; return n; }
            depth--;
        } else if (c == '|' && depth == 0) {
            ends[n - 1] = *p;
            if (n < maxb) starts[n++] = *p + 1;
        }
        (*p)++;
    }
    ends[n - 1] = *p;
    return n;
}

static void cte_process(const CteTable *table, const char *s, size_t len,
                        CteState *st, CteOut *o, unsigned depth);

static void expand_branch(const CteTable *table, const char *s,
                          size_t start, size_t end, CteState *st,
                          CteOut *o, unsigned depth) {
    if (start <= end && depth < CTE_MAX_RECURSION)
        cte_process(table, s + start, end - start, st, o, depth + 1);
}

/* Consume a side-effect opcode's operand: a variable/target token (letters)
 * and an optional signed integer, then an optional bracket group (e.g. the
 * `[*]` of ^XF85[*]).  This deliberately stops at the first byte that is not
 * part of the operand so surrounding narrative text is preserved. */
static void skip_effect(const char *s, size_t len, size_t *p) {
    while (*p < len && isalpha((unsigned char)s[*p])) (*p)++;
    if (*p < len && (s[*p] == '-' || s[*p] == '+')) (*p)++;
    while (*p < len && isdigit((unsigned char)s[*p])) (*p)++;
    if (*p < len && s[*p] == '[') skip_group(s, len, p);
}

static void cte_process(const CteTable *table, const char *s, size_t len,
                        CteState *st, CteOut *o, unsigned depth) {
    size_t i = 0;
    size_t starts[40], ends[40];
    while (i < len) {
        char c = s[i];
        if (c == '^' && i + 1 < len) {
            i++;
            char op = s[i];
            if (op == '^') { cte_emit(o, '\n'); i++; continue; }
            if (op == ';') {  /* comment to end of line */
                while (i < len && s[i] != '\n' && s[i] != '\r') i++;
                while (i < len && (s[i] == '\n' || s[i] == '\r')) i++;
                continue;
            }
            if (op == 'O') {                    /* ^O<n>[a|b|...] random pick */
                i++;
                while (i < len && isdigit((unsigned char)s[i])) i++;
                unsigned n = split_branches(s, len, &i, starts, ends, 40);
                if (n > 0) {
                    unsigned pick = (unsigned)(cte_prng(&st->prng_state) % n);
                    expand_branch(table, s, starts[pick], ends[pick], st, o, depth);
                }
                continue;
            }
            if (op == 'X' && i + 1 < len) {
                i++;
                char x = s[i];
                if (x == 'I') {              /* ^XI<cond>[then|else] */
                    i++;
                    bool t = eval_condition(s, len, &i, st);
                    unsigned n = split_branches(s, len, &i, starts, ends, 40);
                    unsigned pick = t ? 0 : 1;
                    if (pick < n) expand_branch(table, s, starts[pick], ends[pick], st, o, depth);
                    continue;
                }
                if (x == 'C' && i + 1 < len) {  /* ^XC<var>[c0|c1|...] */
                    i++;
                    char vn[4] = { s[i], 0, 0, 0 };
                    i++;
                    /* ^XCA uses the case_value convention; here we switch on the
                     * named variable's value. */
                    int idx = cte_state_get(st, vn);
                    unsigned n = split_branches(s, len, &i, starts, ends, 40);
                    if (idx >= 0 && (unsigned)idx < n)
                        expand_branch(table, s, starts[idx], ends[idx], st, o, depth);
                    continue;
                }
                if (x == 'P') { i++; continue; }  /* person-name insert: no
                                                     authentic name source yet,
                                                     so emit nothing */
                /* Any other ^X opcode is a side effect / unsupported call:
                 * consume its operand and emit nothing. */
                i++;
                skip_effect(s, len, &i);
                continue;
            }
            if (op == 'F') {   /* ^F<op><operand>: formatting/flag side effect */
                i++;
                skip_effect(s, len, &i);
                continue;
            }
            /* Unknown ^-escape: drop the marker. */
            i++;
            continue;
        }
        if (c == '\r') { i++; if (i < len && s[i] == '\n') i++; cte_emit(o, ' '); continue; }
        if (c == '\n') { cte_emit(o, ' '); i++; continue; }
        cte_emit(o, c);
        i++;
    }
}

bool cte_expand(const CteTable *table, const CteSection *section,
                CteState *state, char *out, size_t out_size) {
    if (!table || !section || !state || !out || out_size < 2) return false;
    if (state->prng_state == 0) state->prng_state = 1;  /* seed if uninitialised */
    CteOut o = { out, 0, out_size };
    cte_process(table, (const char *)section->content, section->length,
                state, &o, 0);
    out[o.pos] = '\0';
    return true;
}
