# Internationalization

OpenCaptive supports 19 languages with full UI translation coverage using a gettext-compatible PO file system.

## Supported Languages

| Code | Language |
|---|---|
| en | English |
| sv | Svenska |
| cs | Cestina |
| da | Dansk |
| de | Deutsch |
| es | Espanol |
| fi | Suomi |
| fr | Francais |
| hu | Magyar |
| it | Italiano |
| ja | Japanese |
| ko | Korean |
| nl | Nederlands |
| no | Norsk |
| pl | Polski |
| pt | Portugues |
| ro | Romana |
| ru | Russian |
| zh | Chinese |

## Architecture

### Translation Macro

All translatable strings are wrapped with the `_()` macro:

```c
#define _(s) i18n_get(s)
```

This calls `i18n_get()`, which performs a linear search through the loaded translation table (up to 512 entries of 256 characters each).

### Data Structures

```c
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
```

### API

```c
void        i18n_init(const char *lang_override);
void        i18n_free(void);
const char *i18n_get(const char *msgid);
const char *i18n_get_lang(void);
```

## Language Detection

At startup, `i18n_init` determines the language in this order:

1. **CLI override**: `--lang <code>` command-line argument (passed as `lang_override`).
2. **SDL3 auto-detection**: `SDL_GetPreferredLocales()` queries the operating system locale.
3. **Fallback**: English (no PO file loaded; `i18n_get` returns the msgid verbatim).

## PO File Format

Translation files use the standard gettext `.po` format, stored in the `po/` directory:

- **Source files**: `po/<lang>.po` (e.g., `po/sv.po`, `po/ja.po`)
- **Template**: `po/messages.pot` (POT template for generating new translations)
- **Compiled**: `po/<lang>.mo` (binary format compiled from `.po` files)

The loader searches for PO files first relative to the SDL base path, then the current working directory. It handles escape sequences (`\n`, `\t`, `\\`, `\"`) and multi-line msgid/msgstr via continuation strings.

## Translation Coverage

All user-facing text is translatable:

- Start menu labels and descriptions
- Settings panel items
- Building interaction dialogue
- Shop interface
- NPC dialogue
- Combat messages
- All HUD and status text

## Font Rendering

### Bitmap Font

The built-in bitmap font uses 5-column, 7-row glyphs covering A-Z, a-z, 0-9, and common punctuation. It includes:

- **UTF-8 decoding**: multi-byte UTF-8 sequences are decoded to Unicode codepoints.
- **Accented character fallback**: Unicode accented characters are mapped to their ASCII base character for rendering (e.g., e with accent maps to plain e).

### TTF Font

The start menu uses DejaVu Sans Mono Bold TTF rendering at 36pt, 18pt, and 14pt sizes, which supports the full Unicode range needed for all 19 languages.

## Settings Integration

The language selector in the Settings panel cycles through all 19 languages using Left/Right arrow keys. The `lang_index` field in `StartMenu` tracks the current selection. Changing the language reloads the translation table.

## Adding a New Language

1. Copy `po/messages.pot` to `po/<code>.po`.
2. Translate all `msgstr` entries.
3. Compile to `po/<code>.mo`.
4. Add the language code to the `LANG_COUNT` array in `start_menu.c`.

## Source Files

- Header: `include/i18n.h`
- Implementation: `src/data/i18n.c`
- Translations: `po/*.po`
- Template: `po/messages.pot`
