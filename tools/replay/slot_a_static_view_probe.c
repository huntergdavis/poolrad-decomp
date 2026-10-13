#include "poolrad/replay/slot_a_static_view.h"

#include "poolrad/formats/dax.h"
#include "poolrad/formats/dos_savgam.h"
#include "poolrad/formats/geo.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static uint8_t *read_file(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    uint8_t *data = NULL;
    long length;
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) || (length = ftell(file)) < 0 ||
        (size_t)length > PR_DAX_MAX_FILE || fseek(file, 0, SEEK_SET))
        goto done;
    data = malloc(length ? (size_t)length : 1u);
    if (!data || fread(data, 1u, (size_t)length, file) != (size_t)length) {
        free(data);
        data = NULL;
        goto done;
    }
    *size = (size_t)length;
done:
    fclose(file);
    return data;
}

static int read_block(const uint8_t *file, size_t size, uint8_t id,
                      uint8_t *output, size_t capacity, size_t *written) {
    pr_dax_archive archive;
    size_t i;
    if (pr_dax_open(file, size, &archive) != PR_DAX_OK) return -1;
    for (i = 0; i < archive.count; ++i) {
        if (archive.entries[i].id != id) continue;
        return pr_dax_read(&archive, i, output, capacity, written) ==
               PR_DAX_OK ? 0 : -1;
    }
    return -1;
}

static int number(const char *argument, size_t *out) {
    char *end;
    unsigned long value;
    errno = 0;
    value = strtoul(argument, &end, 0);
    if (errno || end == argument || *end || value > 65535ul) return -1;
    *out = (size_t)value;
    return 0;
}

int main(int argc, char **argv) {
    uint8_t *save = NULL, *geo_file = NULL, *art_file = NULL;
    uint8_t decoded[65535];
    size_t save_size = 0, geo_size = 0, art_size = 0, written = 0;
    pr_slot_a_native_state state;
    pr_slot_a_view_context context;
    pr_slot_a_view_frame frame;
    pr_ega_block image;
    int result = 1, i;

    if (argc < 7 || (argc - 4) % 3 != 0) {
        fprintf(stderr, "usage: probe SAVE GEO_DAX EGA_DAX ITEM X Y [ITEM X Y ...]\n");
        return 2;
    }
    save = read_file(argv[1], &save_size);
    geo_file = read_file(argv[2], &geo_size);
    art_file = read_file(argv[3], &art_size);
    if (!save || !geo_file || !art_file ||
        pr_slot_a_native_restore(save, save_size, &state) !=
            PR_SLOT_A_NATIVE_OK ||
        read_block(geo_file, geo_size, state.map_id, decoded,
                   sizeof decoded, &written) ||
        pr_slot_a_view_prepare(&state, decoded, written, state.map_id,
                               &context) != PR_SLOT_A_VIEW_OK ||
        read_block(art_file, art_size, 1u, decoded,
                   sizeof decoded, &written) ||
        pr_ega_open(decoded, written, &image) != PR_EGA_OK)
        goto done;
    pr_slot_a_view_clear(&frame);
    for (i = 4; i < argc; i += 3) {
        size_t item, x, y;
        if (number(argv[i], &item) || number(argv[i + 1], &x) ||
            number(argv[i + 2], &y) ||
            pr_slot_a_view_blit_tile(&frame, &image, item, x, y, 0) !=
                PR_SLOT_A_VIEW_OK) goto done;
    }
    if (fwrite(frame.index, 1u, sizeof frame.index, stdout) !=
            sizeof frame.index ||
        fwrite(frame.covered, 1u, sizeof frame.covered, stdout) !=
            sizeof frame.covered) goto done;
    result = 0;
done:
    if (result) fprintf(stderr, "static view input or tile error\n");
    free(save);
    free(geo_file);
    free(art_file);
    return result;
}
