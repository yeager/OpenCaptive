#include "data_vfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The original CD32 disc consists of one MODE1 data track followed by ten
 * Red Book audio tracks.  Track identity is content-only; order is the
 * verified table-of-contents order, never an archive member filename. */
static const char *const track_hashes[] = {
    "f807b1385c0996d54ed10afab271a7dd31d2c6dc6a18f13196ad2a79a0af8a80",
    "ceded68026add084e467f750f40297d46dc7e2ce493fb3f855e956acebd1f59c",
    "527bd7f2d00f7e1f1d33593e697541b70bc215c4cc11b6bbe6ad6cf42b81ea58",
    "64994dc80fa2cf425d0827f919ab3e078cf04cdd7a4b357e7fe01b6439b471ed",
    "196f28c68ba35703530179311fa9f41b2ba749ef402722dcfdb157aaf6a03fac",
    "3fa8f0be60271f4738f448515fc433749622d71066895f7611c46b766efe5040",
    "f4afd5eebd2f7128c6bfeb67212323b5ca27abb6feead6786c203c2d85c00ac0",
    "b5301553fd2a082c2134570af25aec3fffceef816b1c6e90b8c3b0127225aaf3",
    "6708d78f1d990009272eaa31602f917bafb709a31b6bceb3c3f2ef1649782ccc",
    "25be0db8612c832728423c6894388bf0b79c35ba7916ac8c54b0f8f6f18eb524",
    "359ece35a1f05f682bea0b5b6dfa280bd628254ab90271ca1cec5b75e4d766f6",
};

static const size_t track_sizes[] = {
    194286960u, 65663136u, 23011968u, 10593408u, 30914688u, 43257984u,
    32194176u, 37462656u, 47999616u, 36559488u, 36935808u,
};

static bool write_file(const char *path, const uint8_t *data, size_t size) {
    FILE *file = fopen(path, "wb");
    if (!file) return false;
    bool wrote = fwrite(data, 1, size, file) == size;
    return fclose(file) == 0 && wrote;
}

static bool make_path(char *out, size_t out_size, const char *dir,
                      const char *hash, const char *extension) {
    int n = snprintf(out, out_size, "%s/%s%s", dir, hash, extension);
    return n >= 0 && (size_t)n < out_size;
}

static bool write_cue(const char *output_dir) {
    char cue_path[1024];
    if (!make_path(cue_path, sizeof(cue_path), output_dir,
                   "liberation-cd32", ".cue")) return false;
    FILE *cue = fopen(cue_path, "wb");
    if (!cue) return false;

    bool ok = fprintf(cue, "CATALOG 0000000000000\n") > 0;
    for (size_t i = 0; ok && i < sizeof(track_hashes) / sizeof(track_hashes[0]); ++i) {
        ok = fprintf(cue, "FILE \"%s.bin\" BINARY\n", track_hashes[i]) > 0;
        if (i == 0) {
            ok = ok && fprintf(cue, "  TRACK 01 MODE1/2352\n"
                                    "    INDEX 01 00:00:00\n") > 0;
        } else if (i == 1) {
            ok = ok && fprintf(cue, "  TRACK 02 AUDIO\n"
                                    "    INDEX 00 00:00:00\n"
                                    "    INDEX 01 00:02:00\n") > 0;
        } else {
            ok = ok && fprintf(cue, "  TRACK %02zu AUDIO\n"
                                    "    INDEX 01 00:00:00\n", i + 1) > 0;
        }
    }
    return ok && fclose(cue) == 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <data-dir> <existing-output-dir>\n", argv[0]);
        return 2;
    }

    DataVFS vfs;
    if (!vfs_init(&vfs, argv[1])) return 1;
    for (size_t i = 0; i < sizeof(track_hashes) / sizeof(track_hashes[0]); ++i) {
        size_t size = 0;
        uint8_t *data = vfs_find_sha256(&vfs, track_hashes[i], &size);
        char path[1024];
        bool ok = data && size == track_sizes[i] &&
                  make_path(path, sizeof(path), argv[2], track_hashes[i], ".bin") &&
                  write_file(path, data, size);
        free(data);
        if (!ok) {
            fprintf(stderr, "verified CD32 track %zu is unavailable or could not be written\n", i + 1);
            vfs_free(&vfs);
            return 1;
        }
    }
    vfs_free(&vfs);
    if (!write_cue(argv[2])) {
        fprintf(stderr, "could not write hash-addressed CUE\n");
        return 1;
    }
    printf("wrote 11 verified CD32 tracks and liberation-cd32.cue\n");
    return 0;
}
