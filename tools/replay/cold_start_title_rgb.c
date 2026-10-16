#include "poolrad/replay/cold_start_title_rgb.h"

#include "poolrad/formats/dax.h"

#include <stdio.h>
#include <stdlib.h>

static uint8_t *read_archive(const char *path, size_t *size) {
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

int main(int argc, char **argv) {
    uint8_t *archive = NULL, *rgb = NULL;
    size_t archive_size = 0, written = 0;
    pr_cold_title_status status;
    FILE *output;
    int result = 1;

    if (argc != 3) {
        fprintf(stderr, "usage: %s TITLE.DAX OUTPUT.rgb24\n", argv[0]);
        return 2;
    }
    archive = read_archive(argv[1], &archive_size);
    rgb = malloc(PR_COLD_TITLE_RGB24_BYTES);
    if (!archive || !rgb) {
        fprintf(stderr, "cannot read archive or allocate RGB output\n");
        goto done;
    }
    status = pr_cold_title_rgb24(archive, archive_size, rgb,
                                 PR_COLD_TITLE_RGB24_BYTES, &written);
    if (status != PR_COLD_TITLE_OK || written != PR_COLD_TITLE_RGB24_BYTES) {
        fprintf(stderr, "title render: %s\n",
                pr_cold_title_status_string(status));
        goto done;
    }
    output = fopen(argv[2], "wbx");
    if (!output) {
        fprintf(stderr, "cannot create new RGB output\n");
        goto done;
    }
    {
        const int wrote_all = fwrite(rgb, 1u, written, output) == written;
        const int closed = fclose(output) == 0;
        if (wrote_all && closed) result = 0;
    }
    if (result) {
        /* The file was created by this invocation; remove partial output. */
        remove(argv[2]);
        fprintf(stderr, "cannot write RGB output\n");
    }
done:
    free(rgb);
    free(archive);
    return result;
}
