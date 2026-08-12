#ifndef CAPTIVE_EMULATOR_H
#define CAPTIVE_EMULATOR_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int process_id;
    int command_fd;
    char output_dir[1024];
    char dump_path[1200];
    char fifo_path[1200];
} CaptiveEmulatorSession;

/* Launch the supplied original CAPTIVE.BAT chain in the isolated DOSBox-X
 * profile. This path never copies, patches or generates game data. */
bool captive_emulator_launch(const char *data_path);

/* Start the authentic CAPPO chain and expose a raw XT-scan FIFO. The
 * resulting MEMDUMP.BIN is written by DOSBox-X and may be polled by the
 * caller; no compatibility state is produced. */
bool captive_emulator_session_start(const char *data_path,
                                    CaptiveEmulatorSession *session);
bool captive_emulator_session_send_scan(CaptiveEmulatorSession *session,
                                         uint8_t scan_code);
bool captive_emulator_session_dump_ready(const CaptiveEmulatorSession *session);
void captive_emulator_session_stop(CaptiveEmulatorSession *session);

#endif
