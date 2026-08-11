#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <unistd.h>
#endif

#include "captive_emulator.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)

bool captive_emulator_launch(const char *data_path) {
    (void)data_path;
    fprintf(stderr, "Authentic Captive emulator launch is not available on Windows yet\n");
    return false;
}

#else

#include <errno.h>

static bool file_exists(const char *path) {
    FILE *file = path ? fopen(path, "rb") : NULL;
    if (!file) return false;
    fclose(file);
    return true;
}

static bool path_has_quote(const char *path) {
    return path && strchr(path, '"') != NULL;
}

static bool build_profile_path(char *out, size_t out_size) {
    const char *base = SDL_GetBasePath();
    if (base) {
        int written = snprintf(out, out_size, "%stools/dosbox-x-captive.conf", base);
        SDL_free((void *)base);
        if (written > 0 && (size_t)written < out_size && file_exists(out)) return true;
        base = SDL_GetBasePath();
        if (base) {
            written = snprintf(out, out_size,
                               "%sassets/captive/dosbox-x-captive.conf", base);
            SDL_free((void *)base);
            if (written > 0 && (size_t)written < out_size && file_exists(out)) return true;
        }
        base = SDL_GetBasePath();
        if (base) {
            written = snprintf(out, out_size,
                               "%s../Resources/tools/dosbox-x-captive.conf", base);
            SDL_free((void *)base);
            if (written > 0 && (size_t)written < out_size && file_exists(out)) return true;
        }
        base = SDL_GetBasePath();
        if (base) {
            written = snprintf(out, out_size,
                               "%s../Resources/assets/captive/dosbox-x-captive.conf", base);
            SDL_free((void *)base);
            if (written > 0 && (size_t)written < out_size && file_exists(out)) return true;
        }
    }
    int written = snprintf(out, out_size, "tools/dosbox-x-captive.conf");
    return written > 0 && (size_t)written < out_size && file_exists(out);
}

static const char *find_dosbox_binary(void) {
    const char *override = getenv("DOSBOX_X_BIN");
    if (override && *override) return override;
    if (file_exists("/opt/homebrew/bin/dosbox-x")) return "/opt/homebrew/bin/dosbox-x";
    if (file_exists("/usr/local/bin/dosbox-x")) return "/usr/local/bin/dosbox-x";
    return "dosbox-x";
}

bool captive_emulator_launch(const char *data_path) {
    char captive_bat[1024];
    char profile[1024];
    char mount_command[1200];
    pid_t child = 0;
    const char *binary;

    if (!data_path || !*data_path || path_has_quote(data_path)) {
        fprintf(stderr, "Authentic Captive launch requires a valid data path\n");
        return false;
    }
    int written = snprintf(captive_bat, sizeof(captive_bat), "%s/CAPTIVE.BAT", data_path);
    if (written <= 0 || (size_t)written >= sizeof(captive_bat) ||
        !file_exists(captive_bat)) {
        fprintf(stderr, "Authentic Captive launch requires CAPTIVE.BAT in: %s\n", data_path);
        return false;
    }
    if (!build_profile_path(profile, sizeof(profile))) {
        fprintf(stderr, "DOSBox-X Captive profile is missing beside OpenCaptive\n");
        return false;
    }
    written = snprintf(mount_command, sizeof(mount_command), "mount c \"%s\"", data_path);
    if (written <= 0 || (size_t)written >= sizeof(mount_command)) return false;

    binary = find_dosbox_binary();
    /* Execute DOSBox-X directly.  macOS `open -a` may reuse an existing
     * application instance and silently discard the new -conf/-c arguments,
     * which can launch an old game or leave the runtime at a blank prompt. */
    char *const direct_args[] = {
        (char *)binary, (char *)"-conf", profile,
        (char *)"-c", mount_command,
        (char *)"-c", (char *)"c:",
        (char *)"-c", (char *)"CAPTIVE.BAT 1", NULL
    };
    char *const *args = direct_args;
    const char *program = binary;
    child = fork();
    if (child < 0) {
        fprintf(stderr, "Unable to start DOSBox-X (%s): %s\n", binary, strerror(errno));
        return false;
    }
    if (child == 0) {
        execvp(program, args);
        _exit(127);
    }
    printf("Started authentic Captive runtime in DOSBox-X (pid %ld)\n", (long)child);
    return true;
}

#endif
