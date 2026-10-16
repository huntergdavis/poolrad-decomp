#include "poolrad/replay/cold_start_title_credits_fixture.h"

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
    uint8_t *title = NULL, *credits = NULL, *ovr = NULL, *frames = NULL;
    size_t title_size = 0u, credits_size = 0u, ovr_size = 0u;
    size_t written = 0u, boundary = 0u;
    pr_cold_title_credits_status status;
    FILE *output;
    int result = 1;

    if (argc != 5) {
        fprintf(stderr, "usage: %s TITLE.DAX 8X8D1.DAX GAME.OVR OUTPUT.rgb24seq\n",
                argv[0]);
        return 2;
    }
    title = read_file(argv[1], &title_size);
    credits = read_file(argv[2], &credits_size);
    ovr = read_file(argv[3], &ovr_size);
    frames = malloc(PR_COLD_TITLE_CREDITS_BYTES);
    if (!title || !credits || !ovr || !frames) {
        fputs("cannot read inputs or allocate sequence output\n", stderr);
        goto done;
    }
    status = pr_cold_title_credits_fixture(
        title, title_size, credits, credits_size, ovr, ovr_size,
        frames, PR_COLD_TITLE_CREDITS_BYTES, &written, &boundary);
    if (status != PR_COLD_TITLE_CREDITS_OK ||
        written != PR_COLD_TITLE_CREDITS_BYTES || boundary != 1u) {
        fprintf(stderr, "transition fixture: %s\n",
                pr_cold_title_credits_status_string(status));
        goto done;
    }
    output = fopen(argv[4], "wbx");
    if (!output) {
        fputs("cannot create new RGB sequence output\n", stderr);
        goto done;
    }
    {
        const int wrote_all = fwrite(frames, 1u, written, output) == written;
        const int closed = fclose(output) == 0;
        if (wrote_all && closed) result = 0;
    }
    if (result) {
        remove(argv[4]); /* Created by this invocation. */
        fputs("cannot write RGB sequence output\n", stderr);
    } else {
        printf("frames=2 boundary_frame=%zu rgb24_bytes=%zu\n", boundary,
               written);
    }
done:
    free(frames);
    free(ovr);
    free(credits);
    free(title);
    return result;
}
