#include "liberation_save.h"
#include <stdio.h>
#include <string.h>

static bool write_u16(FILE *f, uint16_t v) {
    uint8_t b[2] = { (uint8_t)(v >> 8), (uint8_t)v };
    return f && fwrite(b, 1, 2, f) == 2;
}

static bool write_u32(FILE *f, uint32_t v) {
    uint8_t b[4] = { (uint8_t)(v >> 24), (uint8_t)(v >> 16),
                      (uint8_t)(v >> 8), (uint8_t)v };
    return f && fwrite(b, 1, 4, f) == 4;
}

static bool read_u16(FILE *f, uint16_t *out) {
    uint8_t b[2];
    if (!f || !out || fread(b, 1, 2, f) != 2) return false;
    *out = (uint16_t)((uint16_t)b[0] << 8 | b[1]);
    return true;
}

static bool read_u32(FILE *f, uint32_t *out) {
    uint8_t b[4];
    if (!f || !out || fread(b, 1, 4, f) != 4) return false;
    *out = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
           ((uint32_t)b[2] << 8) | b[3];
    return true;
}

bool lib_save_write(const LibSaveData *data, const char *path) {
    if (!data || !path || data->num_droids > LIB_SAVE_MAX_DROIDS ||
        data->city_x < 0 || data->city_x >= CITYGRID_WIDTH ||
        data->city_y < 0 || data->city_y >= CITYGRID_HEIGHT ||
        data->facing > 3 ||
        data->mission < 1 || data->mission >= LIB_SAVE_MAX_MISSIONS ||
        data->shared_inventory_count > LIB_SAVE_MAX_SHARED_ITEMS ||
        data->generators_destroyed > data->generators_total ||
        data->reputation < -100 || data->reputation > 100) return false;
    for (int i = 0; i < data->num_droids; i++) {
        const LibSaveDroid *d = &data->droids[i];
        if (d->hp < 0 || d->hp_max < 0 || d->hp > d->hp_max ||
            d->energy < 0 || d->energy_max < 0 || d->energy > d->energy_max)
            return false;
    }
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    bool ok = fwrite(LIB_SAVE_MAGIC, 1, 4, f) == 4;
    ok = write_u16(f, LIB_SAVE_VERSION) && ok;
    ok = write_u16(f, data->seed_hi) && ok;
    ok = write_u16(f, data->seed_lo) && ok;
    ok = write_u16(f, data->difficulty) && ok;
    ok = write_u16(f, data->mission) && ok;
    ok = write_u32(f, data->gold) && ok;
    ok = write_u32(f, data->tick) && ok;
    ok = write_u16(f, (uint16_t)data->city_x) && ok;
    ok = write_u16(f, (uint16_t)data->city_y) && ok;
    ok = fwrite(&data->facing, 1, 1, f) == 1 && ok;
    ok = fwrite(&data->num_droids, 1, 1, f) == 1 && ok;

    for (int i = 0; i < data->num_droids; i++) {
        const LibSaveDroid *d = &data->droids[i];
        ok = fwrite(d->name, 1, 16, f) == 16 && ok;
        ok = write_u16(f, (uint16_t)d->hp) && ok;
        ok = write_u16(f, (uint16_t)d->hp_max) && ok;
        ok = write_u16(f, (uint16_t)d->energy) && ok;
        ok = write_u16(f, (uint16_t)d->energy_max) && ok;
        ok = fwrite(&d->level, 1, 1, f) == 1 && ok;
        ok = write_u32(f, d->xp) && ok;
        ok = fwrite(d->skills, 1,
                    LIB_SAVE_VERSION >= LIB_SAVE_SKILLS_VERSION ? 10 : 8, f) ==
             (size_t)(LIB_SAVE_VERSION >= LIB_SAVE_SKILLS_VERSION ? 10 : 8) && ok;
        for (int j = 0; j < 8; j++) ok = write_u16(f, d->equipment[j]) && ok;
        ok = fwrite(d->inventory, 1, sizeof(d->inventory), f) ==
             sizeof(d->inventory) && ok;
        ok = fwrite(d->body_part_hp, 1, sizeof(d->body_part_hp), f) ==
             sizeof(d->body_part_hp) && ok;
    }

    ok = fwrite(&data->shared_inventory_count, 1, 1, f) == 1 && ok;
    for (int i = 0; i < data->shared_inventory_count; i++) {
        ok = fwrite(data->shared_inventory[i].name, 1, 24, f) == 24 && ok;
        ok = write_u16(f, data->shared_inventory[i].item_type) && ok;
    }

    ok = fwrite(data->mission_complete, 1, LIB_SAVE_MAX_MISSIONS / 8, f) ==
         LIB_SAVE_MAX_MISSIONS / 8 && ok;
    ok = write_u16(f, data->generators_destroyed) && ok;
    ok = write_u16(f, data->generators_total) && ok;
    ok = write_u16(f, (uint16_t)data->reputation) && ok;

    ok = ok && !ferror(f);
    ok = fclose(f) == 0 && ok;
    return ok;
}

bool lib_save_read(LibSaveData *data, const char *path) {
    if (!data || !path) return false;
    LibSaveData *destination = data;
    /* Parse into an independent zeroed object.  Copying an uninitialised
     * destination here would read indeterminate bytes before the file has
     * been validated, while still providing no useful rollback state. */
    LibSaveData restored = {0};
    data = &restored;
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    memset(data, 0, sizeof(*data));

    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, LIB_SAVE_MAGIC, 4) != 0) {
        fclose(f);
        return false;
    }

    uint16_t ver;
    if (!read_u16(f, &ver) ||
        (ver != LIB_SAVE_VERSION && ver != LIB_SAVE_PREVIOUS_VERSION &&
         ver != LIB_SAVE_REPUTATION_VERSION &&
         ver != LIB_SAVE_XP_VERSION &&
         ver != LIB_SAVE_SHARED_INVENTORY_VERSION &&
         ver != LIB_SAVE_OLD_INVENTORY_VERSION && ver != LIB_SAVE_LEGACY_VERSION)) {
        fclose(f);
        return false;
    }

    data->version = ver;
    uint16_t city_x, city_y;
    if (!read_u16(f, &data->seed_hi) || !read_u16(f, &data->seed_lo) ||
        !read_u16(f, &data->difficulty) || !read_u16(f, &data->mission) ||
        !read_u32(f, &data->gold) || !read_u32(f, &data->tick) ||
        !read_u16(f, &city_x) || !read_u16(f, &city_y) ||
        fread(&data->facing, 1, 1, f) != 1 ||
        fread(&data->num_droids, 1, 1, f) != 1) {
        fclose(f);
        return false;
    }
    data->city_x = (int16_t)city_x;
    data->city_y = (int16_t)city_y;

    /* Every field above is mandatory; fread() on a short file must not be
     * silently converted into zero-valued state. */
    if (feof(f) || ferror(f) || data->facing > 3 ||
        data->city_x < 0 || data->city_x >= CITYGRID_WIDTH ||
        data->city_y < 0 || data->city_y >= CITYGRID_HEIGHT ||
        data->mission < 1 || data->mission >= LIB_SAVE_MAX_MISSIONS) {
        fclose(f);
        return false;
    }

    if (data->num_droids > LIB_SAVE_MAX_DROIDS) {
        fclose(f);
        return false;
    }

    /* The remainder has a fixed size once the droid count is known.  Check
     * it before decoding so every later field is guaranteed to have its
     * complete representation, rather than relying on feof() after a
     * sequence of ignored short fread() results. */
    long payload_start = ftell(f);
    if (payload_start < 0 || fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return false;
    }
    long file_end = ftell(f);
    long long droid_record_size = ver >= LIB_SAVE_SKILLS_VERSION ? 71LL :
                                   ver >= LIB_SAVE_BODY_PART_VERSION ? 69LL :
                                   ver >= LIB_SAVE_XP_VERSION ? 63LL :
                                   ver == LIB_SAVE_OLD_INVENTORY_VERSION ? 59LL :
                                   49LL;
    /* The count is read after the droid records, so the preflight can only
     * require the count byte here; the per-item bytes are checked while
     * decoding the declared count below. */
    long long shared_inventory_size = ver >= LIB_SAVE_SHARED_INVENTORY_VERSION ? 1LL : 0LL;
    long long required_end = (long long)payload_start +
                             (long long)data->num_droids * droid_record_size +
                             shared_inventory_size + LIB_SAVE_MAX_MISSIONS / 8 + 4LL;
    if (file_end < 0 || (long long)file_end < required_end ||
        fseek(f, payload_start, SEEK_SET) != 0) {
        fclose(f);
        return false;
    }

    for (int i = 0; i < data->num_droids; i++) {
        LibSaveDroid *d = &data->droids[i];
        uint16_t hp, hp_max, energy, energy_max;
        uint32_t xp = 0;
        memset(d->skills, 0, sizeof(d->skills));
        if (fread(d->name, 1, 16, f) != 16 ||
            !read_u16(f, &hp) || !read_u16(f, &hp_max) ||
            !read_u16(f, &energy) || !read_u16(f, &energy_max) ||
            fread(&d->level, 1, 1, f) != 1 ||
            (ver >= LIB_SAVE_XP_VERSION && !read_u32(f, &xp)) ||
            fread(d->skills, 1, 8, f) != 8 ||
            (ver >= LIB_SAVE_SKILLS_VERSION &&
             fread(d->skills + 8, 1, 2, f) != 2)) {
            fclose(f);
            return false;
        }
        d->name[sizeof(d->name) - 1] = '\0';
        d->hp = (int16_t)hp;
        d->hp_max = (int16_t)hp_max;
        d->energy = (int16_t)energy;
        d->energy_max = (int16_t)energy_max;
        d->xp = xp;
        for (int j = 0; j < 8; j++) {
            if (!read_u16(f, &d->equipment[j])) {
                fclose(f);
                return false;
            }
        }
        if (ver >= LIB_SAVE_OLD_INVENTORY_VERSION &&
            fread(d->inventory, 1, sizeof(d->inventory), f) !=
                sizeof(d->inventory)) {
            fclose(f);
            return false;
        }
        if (ver >= LIB_SAVE_BODY_PART_VERSION &&
            fread(d->body_part_hp, 1, sizeof(d->body_part_hp), f) !=
                sizeof(d->body_part_hp)) {
            fclose(f);
            return false;
        }
    }

    if (ver >= LIB_SAVE_SHARED_INVENTORY_VERSION) {
        if (fread(&data->shared_inventory_count, 1, 1, f) != 1 ||
            data->shared_inventory_count > LIB_SAVE_MAX_SHARED_ITEMS) {
            fclose(f);
            return false;
        }
        for (int i = 0; i < data->shared_inventory_count; i++) {
            if (fread(data->shared_inventory[i].name, 1, 24, f) != 24 ||
                !read_u16(f, &data->shared_inventory[i].item_type)) {
                fclose(f);
                return false;
            }
            data->shared_inventory[i].name[23] = '\0';
        }
    }

    if (feof(f) || ferror(f)) {
        fclose(f);
        return false;
    }

    for (int i = 0; i < data->num_droids; i++) {
        const LibSaveDroid *d = &data->droids[i];
        if (d->hp < 0 || d->hp_max < 0 || d->hp > d->hp_max ||
            d->energy < 0 || d->energy_max < 0 || d->energy > d->energy_max) {
            fclose(f);
            return false;
        }
    }

    if (fread(data->mission_complete, 1, LIB_SAVE_MAX_MISSIONS / 8, f) !=
            LIB_SAVE_MAX_MISSIONS / 8 ||
        !read_u16(f, &data->generators_destroyed) ||
        !read_u16(f, &data->generators_total)) {
        fclose(f);
        return false;
    }

    if (feof(f) || ferror(f) ||
        data->generators_destroyed > data->generators_total) {
        fclose(f);
        return false;
    }

    if (ver >= LIB_SAVE_REPUTATION_VERSION) {
        uint16_t reputation;
        if (!read_u16(f, &reputation)) {
            fclose(f);
            return false;
        }
        data->reputation = (int16_t)reputation;
        if (data->reputation < -100 || data->reputation > 100) {
            fclose(f);
            return false;
        }
    }

    /* A valid save has no extension area.  Accepting trailing bytes would
     * hide accidental concatenation or a format mismatch and make future
     * format changes ambiguous. */
    long payload_end = ftell(f);
    if (payload_end < 0 || payload_end != file_end) {
        fclose(f);
        return false;
    }
    if (fclose(f) != 0) return false;
    *destination = restored;
    return true;
}

void lib_save_from_state(LibSaveData *data, uint16_t seed_hi, uint16_t seed_lo,
                          uint16_t difficulty, uint16_t mission, uint32_t gold,
                          uint32_t tick, const CityNavState *nav,
                          const LibSaveDroid *droids, uint8_t num_droids) {
    if (!data) return;
    memset(data, 0, sizeof(*data));
    data->seed_hi = seed_hi;
    data->seed_lo = seed_lo;
    data->difficulty = difficulty;
    data->mission = mission;
    data->gold = gold;
    data->tick = tick;
    if (nav) {
        data->city_x = (int16_t)nav->cell_x;
        data->city_y = (int16_t)nav->cell_y;
        data->facing = nav->facing <= 3 ? (uint8_t)nav->facing : 0;
    }
    if (num_droids > LIB_SAVE_MAX_DROIDS) {
        data->num_droids = 0;
        return;
    }
    if (!droids) {
        data->num_droids = 0;
        return;
    }
    data->num_droids = num_droids;
    memcpy(data->droids, droids, num_droids * sizeof(LibSaveDroid));
}

bool lib_save_is_mission_complete(const LibSaveData *data, unsigned mission) {
    if (!data || mission >= LIB_SAVE_MAX_MISSIONS) return false;
    return (data->mission_complete[mission / 8] >> (mission % 8)) & 1;
}

void lib_save_set_mission_complete(LibSaveData *data, unsigned mission) {
    if (!data || mission >= LIB_SAVE_MAX_MISSIONS) return;
    data->mission_complete[mission / 8] |= (1 << (mission % 8));
}
