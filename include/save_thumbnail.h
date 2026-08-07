#ifndef SAVE_THUMBNAIL_H
#define SAVE_THUMBNAIL_H

#include <stdbool.h>
#include <stdint.h>

#define SAVE_THUMB_W 80
#define SAVE_THUMB_H 50
#define SAVE_THUMB_PIXELS (SAVE_THUMB_W * SAVE_THUMB_H)

/* Downscales `framebuffer` (fb_width x fb_height) to an 80x50 preview and
 * appends/replaces it as a trailer on an already-written save file. Call
 * after a successful save_game(). Not required for load_game() to accept
 * the file -- saves without a thumbnail remain valid.
 *
 * This is intentionally its own translation unit with no dependency on
 * game_state.h/combat.h/puzzle.h: it is linked into both the full engine
 * (via save_load.c) and the start menu (to preview a save slot), and the
 * latter has no reason to pull in item/combat validation code. */
bool save_game_write_thumbnail(const char *path, const uint32_t *framebuffer,
                               int fb_width, int fb_height);

/* Reads the 80x50 thumbnail trailer written by save_game_write_thumbnail()
 * into out_pixels (must hold SAVE_THUMB_PIXELS uint32_t). Returns false if
 * the file has no thumbnail trailer. */
bool save_read_thumbnail(const char *path, uint32_t *out_pixels);

#endif
