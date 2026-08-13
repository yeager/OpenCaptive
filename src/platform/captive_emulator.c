#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#if defined(__APPLE__)
extern char *mkdtemp(char *template_name);
#endif
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

bool captive_emulator_session_start(const char *data_path,
                                    CaptiveEmulatorSession *session) {
    (void)data_path;
    if (session) memset(session, 0, sizeof(*session));
    return false;
}

bool captive_emulator_session_send_scan(CaptiveEmulatorSession *session,
                                         uint8_t scan_code) {
    (void)session;
    (void)scan_code;
    return false;
}

bool captive_emulator_session_wait(CaptiveEmulatorSession *session,
                                   unsigned ticks) {
    (void)session;
    (void)ticks;
    return false;
}

bool captive_emulator_session_send_mouse_motion(CaptiveEmulatorSession *session,
                                                int dx, int dy) {
    (void)session; (void)dx; (void)dy;
    return false;
}

bool captive_emulator_session_send_mouse_button(CaptiveEmulatorSession *session,
                                                bool pressed) {
    (void)session; (void)pressed;
    return false;
}

bool captive_emulator_session_dump_ready(const CaptiveEmulatorSession *session) {
    (void)session;
    return false;
}

void captive_emulator_session_stop(CaptiveEmulatorSession *session) {
    (void)session;
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

static bool build_sequence_path(char *out, size_t out_size) {
    const char *base = SDL_GetBasePath();
    int written = 0;
    if (base) {
        written = snprintf(out, out_size, "%stools/captive_dosbox_sequence.expect", base);
        bool found = written > 0 && (size_t)written < out_size && file_exists(out);
        if (!found) {
            written = snprintf(out, out_size,
                               "%s../Resources/tools/captive_dosbox_sequence.expect", base);
            found = written > 0 && (size_t)written < out_size && file_exists(out);
        }
        if (!found) {
            written = snprintf(out, out_size,
                               "%sassets/captive/captive_dosbox_sequence.expect", base);
            found = written > 0 && (size_t)written < out_size && file_exists(out);
        }
        if (!found) {
            written = snprintf(out, out_size,
                               "%s../Resources/assets/captive/captive_dosbox_sequence.expect", base);
            found = written > 0 && (size_t)written < out_size && file_exists(out);
        }
        SDL_free((void *)base);
        if (found) return true;
    }
    written = snprintf(out, out_size, "tools/captive_dosbox_sequence.expect");
    if (written > 0 && (size_t)written < out_size && file_exists(out)) return true;
    written = snprintf(out, out_size, "assets/captive/captive_dosbox_sequence.expect");
    return written > 0 && (size_t)written < out_size && file_exists(out);
}

static const char *find_expect_binary(void) {
    if (file_exists("/usr/bin/expect")) return "/usr/bin/expect";
    if (file_exists("/opt/homebrew/bin/expect")) return "/opt/homebrew/bin/expect";
    return "expect";
}

bool captive_emulator_session_start(const char *data_path,
                                    CaptiveEmulatorSession *session) {
    char sequence[1024];
    char template_path[] = "/tmp/opencaptive-live.XXXXXX";
    char *output_dir;
    pid_t child;
    if (!data_path || !session || path_has_quote(data_path) ||
        !build_sequence_path(sequence, sizeof(sequence))) return false;
    memset(session, 0, sizeof(*session));
    session->process_id = -1;
    output_dir = mkdtemp(template_path);
    if (!output_dir) return false;
    snprintf(session->output_dir, sizeof(session->output_dir), "%s", output_dir);
    snprintf(session->dump_path, sizeof(session->dump_path), "%s/MEMDUMP.BIN", output_dir);
    snprintf(session->fifo_path, sizeof(session->fifo_path), "%s/commands", output_dir);
    if (mkfifo(session->fifo_path, 0600) != 0) {
        rmdir(output_dir);
        return false;
    }
    char *const args[] = {
        (char *)find_expect_binary(), sequence, (char *)data_path,
        /* CAPPO needs the complete original startup window after the
         * INTRO/FILEPLAY handoff. Twelve ticks can still leave the first
         * droid/start screen visible, so navigation bytes would be consumed
         * before Mission 0001 owns the input queue. DOSBox-X verification
         * reaches the authentic CAPTIVE MISSION 0001 navigation/map surface
         * at 240 ticks. */
        (char *)"-", (char *)"240", session->output_dir, (char *)"120",
        session->fifo_path, NULL
    };
    child = fork();
    if (child < 0) {
        unlink(session->fifo_path);
        rmdir(output_dir);
        return false;
    }
    if (child == 0) {
        execvp(args[0], args);
        _exit(127);
    }
    session->process_id = (int)child;
    /* Keep one read/write descriptor while the expect harness finishes its
     * authentic INTRO/FILEPLAY handoff. Opening O_WRONLY here deadlocks: the
     * harness opens the command FIFO only after its first real CAPPO dump. */
    session->command_fd = open(session->fifo_path, O_RDWR | O_NONBLOCK);
    if (session->command_fd < 0) {
        kill(child, SIGTERM);
        waitpid(child, NULL, 0);
        unlink(session->fifo_path);
        rmdir(output_dir);
        memset(session, 0, sizeof(*session));
        session->process_id = -1;
        return false;
    }
    int flags = fcntl(session->command_fd, F_GETFL, 0);
    if (flags >= 0) (void)fcntl(session->command_fd, F_SETFL, flags & ~O_NONBLOCK);
    return true;
}

bool captive_emulator_session_send_scan(CaptiveEmulatorSession *session,
                                         uint8_t scan_code) {
    char command[4];
    ssize_t written;
    if (!session || session->process_id <= 0 || session->command_fd < 0) return false;
    snprintf(command, sizeof(command), "%02X\n", scan_code);
    written = write(session->command_fd, command, 3);
    return written == 3;
}

static bool send_fifo_command(CaptiveEmulatorSession *session,
                              const char *command);

bool captive_emulator_session_wait(CaptiveEmulatorSession *session,
                                   unsigned ticks) {
    char command[32];
    int length;
    if (ticks < 1 || ticks > 2000) return false;
    length = snprintf(command, sizeof(command), "WAIT:%u\n", ticks);
    return length > 0 && (size_t)length < sizeof(command) &&
           send_fifo_command(session, command);
}

static bool send_fifo_command(CaptiveEmulatorSession *session,
                              const char *command) {
    size_t length;
    ssize_t written;
    if (!session || session->process_id <= 0 || session->command_fd < 0 ||
        !command) return false;
    length = strlen(command);
    written = write(session->command_fd, command, length);
    return written == (ssize_t)length;
}

bool captive_emulator_session_send_mouse_motion(CaptiveEmulatorSession *session,
                                                int dx, int dy) {
    char command[48];
    bool ok = true;
    /* DOSBox-X's integration device accepts signed relative motion. Split
     * large SDL deltas so the original INT 33 accumulator sees every unit. */
    while (dx != 0) {
        int step = dx > 127 ? 127 : (dx < -127 ? -127 : dx);
        int length = snprintf(command, sizeof(command), "MOUSE_DX:%d\n", step);
        if (length <= 0 || (size_t)length >= sizeof(command) ||
            !send_fifo_command(session, command)) return false;
        dx -= step;
    }
    while (dy != 0) {
        int step = dy > 127 ? 127 : (dy < -127 ? -127 : dy);
        int length = snprintf(command, sizeof(command), "MOUSE_DY:%d\n", step);
        if (length <= 0 || (size_t)length >= sizeof(command) ||
            !send_fifo_command(session, command)) return false;
        dy -= step;
    }
    return ok;
}

bool captive_emulator_session_send_mouse_button(CaptiveEmulatorSession *session,
                                                bool pressed) {
    return send_fifo_command(session, pressed ? "MOUSE_DOWN\n" : "MOUSE_UP\n");
}

bool captive_emulator_session_dump_ready(const CaptiveEmulatorSession *session) {
    struct stat st;
    return session && stat(session->dump_path, &st) == 0 &&
           st.st_size == 1048576;
}

void captive_emulator_session_stop(CaptiveEmulatorSession *session) {
    if (!session) return;
    if (session->process_id > 0) {
        int status;
        if (session->command_fd >= 0) {
            (void)write(session->command_fd, "quit\n", 5);
            close(session->command_fd);
            session->command_fd = -1;
        }
        kill((pid_t)session->process_id, SIGTERM);
        waitpid((pid_t)session->process_id, &status, 0);
    }
    if (session->dump_path[0]) unlink(session->dump_path);
    if (session->fifo_path[0]) unlink(session->fifo_path);
    if (session->output_dir[0]) rmdir(session->output_dir);
    memset(session, 0, sizeof(*session));
    session->process_id = -1;
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
