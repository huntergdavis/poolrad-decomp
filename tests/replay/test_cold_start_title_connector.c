#include "poolrad/replay/cold_start_title_connector.h"
#include "poolrad/replay/cold_start_title_rgb.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_BYTES PR_COLD_TITLE_RGB24_BYTES

static unsigned renderer_calls;
static unsigned renderer_mode;

pr_cold_title_status pr_cold_title_rgb24(
    const uint8_t *title_dax, size_t title_dax_size,
    uint8_t *rgb24, size_t capacity, size_t *written) {
    ++renderer_calls;
    if (title_dax == NULL || title_dax_size != 4u ||
        title_dax[0] != 1u || title_dax[3] != 4u ||
        rgb24 == NULL || capacity != FRAME_BYTES || written == NULL)
        return PR_COLD_TITLE_ARGUMENT;
    if (renderer_mode == 1u)
        return PR_COLD_TITLE_BLOCK;
    if (renderer_mode == 2u) {
        *written = FRAME_BYTES - 1u;
        return PR_COLD_TITLE_OK;
    }
    memset(rgb24, 0x5au, FRAME_BYTES);
    *written = FRAME_BYTES;
    return PR_COLD_TITLE_OK;
}

static void fail(const char *message) {
    fprintf(stderr, "%s\n", message);
    exit(1);
}

static void make_path(char *out, size_t capacity,
                      const char *dir, const char *name) {
    int n = snprintf(out, capacity, "%s/%s", dir, name);
    if (n < 0 || (size_t)n >= capacity)
        fail("test path too long");
}

int main(int argc, char **argv) {
    static const uint8_t synthetic_asset[] = {1u, 2u, 3u, 4u};
    char asset_path[1024], output_path[1024], bad_output_path[1024];
    FILE *file;
    long output_size;
    int first, last;

    if (argc != 2 || strlen(argv[1]) > 900u)
        fail("expected one existing test directory");
    make_path(asset_path, sizeof asset_path, argv[1], "TITLE.DAX");
    make_path(output_path, sizeof output_path, argv[1], "title.rgb24");
    make_path(bad_output_path, sizeof bad_output_path,
              argv[1], "missing/title.rgb24");
    (void)remove(asset_path);
    (void)remove(output_path);

    if (pr_cold_start_title_connect_file(NULL, output_path) !=
        PR_COLD_START_TITLE_CONNECT_ARGUMENT)
        fail("NULL root boundary");
    if (pr_cold_start_title_connect_file(argv[1], asset_path) !=
        PR_COLD_START_TITLE_CONNECT_ARGUMENT)
        fail("same input/output path boundary");
    if (pr_cold_start_title_connect_file(argv[1], output_path) !=
        PR_COLD_START_TITLE_CONNECT_BLOCKED_ASSET || renderer_calls != 0u)
        fail("missing asset boundary");

    file = fopen(asset_path, "wb");
    if (file == NULL)
        fail("open synthetic asset");
    {
        size_t transferred = fwrite(synthetic_asset, 1u,
                                    sizeof synthetic_asset, file);
        int closed = fclose(file);
        if (transferred != sizeof synthetic_asset || closed != 0)
            fail("create synthetic asset");
    }

    renderer_mode = 1u;
    if (pr_cold_start_title_connect_file(argv[1], output_path) !=
        PR_COLD_START_TITLE_CONNECT_BLOCKED_FRAME || renderer_calls != 1u)
        fail("renderer failure boundary");
    file = fopen(output_path, "rb");
    if (file != NULL)
        fail("renderer failure created output");

    renderer_mode = 2u;
    if (pr_cold_start_title_connect_file(argv[1], output_path) !=
        PR_COLD_START_TITLE_CONNECT_BLOCKED_FRAME || renderer_calls != 2u)
        fail("renderer short-output boundary");

    renderer_mode = 0u;
    if (pr_cold_start_title_connect_file(argv[1], bad_output_path) !=
        PR_COLD_START_TITLE_CONNECT_OUTPUT || renderer_calls != 3u)
        fail("output open boundary");
    if (pr_cold_start_title_connect_file(argv[1], output_path) !=
        PR_COLD_START_TITLE_CONNECT_OK || renderer_calls != 4u)
        fail("successful renderer call count");
    if (pr_cold_start_title_connect_file(argv[1], output_path) !=
        PR_COLD_START_TITLE_CONNECT_OUTPUT || renderer_calls != 5u)
        fail("existing output boundary");
    file = fopen(output_path, "rb");
    if (file == NULL || fseek(file, 0, SEEK_END) != 0)
        fail("open output");
    output_size = ftell(file);
    if (output_size != (long)FRAME_BYTES || fseek(file, 0, SEEK_SET) != 0)
        fail("output size");
    first = fgetc(file);
    if (fseek(file, -1L, SEEK_END) != 0)
        fail("seek output last byte");
    last = fgetc(file);
    if (fclose(file) != 0 || first != 0x5a || last != 0x5a)
        fail("output contents");

    if (strcmp(pr_cold_start_title_connect_status_string(
                   PR_COLD_START_TITLE_CONNECT_BLOCKED_FRAME),
               "blocked title frame") != 0)
        fail("status string");
    if (remove(output_path) != 0 || remove(asset_path) != 0)
        fail("clean synthetic files");
    return 0;
}
