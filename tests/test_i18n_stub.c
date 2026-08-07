#include "i18n.h"

/* droid_ui rendering tests exercise the pixel renderer, not catalog loading.
 * Keeping this target independent of SDL's locale/path runtime also makes the
 * test deterministic on Windows Release runners. */
const char *i18n_get(const char *msgid) {
    return msgid ? msgid : "";
}
