#include "poolrad/replay/cold_start_credits_rgb.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define INPUT_LIMIT (1024u * 1024u)

static uint8_t *read_file(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    uint8_t *data = NULL;
    long length;
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) || (length = ftell(file)) < 0 ||
        (size_t)length > INPUT_LIMIT || fseek(file, 0, SEEK_SET)) goto done;
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
    uint8_t *dax = NULL, *ovr = NULL, *rgb = NULL;
    size_t dax_size = 0u, ovr_size = 0u, written = 0u;
    pr_cold_credits_status status;
    FILE *output;
    int result = 1;

    if (argc != 4) {
        fprintf(stderr, "usage: %s 8X8D1.DAX GAME.OVR OUTPUT.rgb24\n", argv[0]);
        return 2;
    }
    dax = read_file(argv[1], &dax_size);
    ovr = read_file(argv[2], &ovr_size);
    rgb = malloc(PR_COLD_CREDITS_RGB24_BYTES);
    if (!dax || !ovr || !rgb) {
        fputs("cannot read inputs or allocate RGB output\n", stderr);
        goto done;
    }
    status = pr_cold_credits_rgb24(dax, dax_size, ovr, ovr_size, rgb,
                                    PR_COLD_CREDITS_RGB24_BYTES, &written);
    if (status != PR_COLD_CREDITS_OK ||
        written != PR_COLD_CREDITS_RGB24_BYTES) {
        fprintf(stderr, "credits render: %s\n",
                pr_cold_credits_status_string(status));
        goto done;
    }
    output = fopen(argv[3], "wbx");
    if (!output) {
        fputs("cannot create new RGB output\n", stderr);
        goto done;
    }
    {
        const int wrote_all = fwrite(rgb, 1u, written, output) == written;
        const int closed = fclose(output) == 0;
        if (wrote_all && closed) result = 0;
    }
    if (result) {
        remove(argv[3]); /* Created by this invocation. */
        fputs("cannot write RGB output\n", stderr);
    }
done:
    free(rgb);
    free(ovr);
    free(dax);
    return result;
}
