#ifndef PNG_LOADER_H
#define PNG_LOADER_H

/* Shared declaration for the stb_image-backed PNG loader defined in
 * src/data/stb_image_impl.c.  start_menu.c previously carried a hand-copied
 * `extern` prototype in a different translation unit, so a change to either
 * side would have become silent undefined behaviour at the call sites. */
unsigned char *load_png_file(const char *path, int *w, int *h);

#endif
