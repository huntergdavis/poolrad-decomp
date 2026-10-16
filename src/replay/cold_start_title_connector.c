#include "poolrad/replay/cold_start_title_connector.h"

#include "poolrad/replay/cold_start_title_rgb.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TITLE_DAX_MAX_BYTES (1024u * 1024u)

pr_cold_start_title_connect_status pr_cold_start_title_connect_file(
    const char *asset_root, const char *output_path) {
    static const char suffix[] = "/TITLE.DAX";
    pr_cold_start_title_connect_status status;
    char *asset_path = NULL;
    uint8_t *asset = NULL, *frame = NULL;
    FILE *file = NULL;
    size_t root_size, asset_size = 0u, written = 0u;
    long file_size;

    if (asset_root == NULL || output_path == NULL ||
        asset_root[0] == '\0' || output_path[0] == '\0')
        return PR_COLD_START_TITLE_CONNECT_ARGUMENT;
    root_size = strlen(asset_root);
    if (root_size > SIZE_MAX - sizeof suffix)
        return PR_COLD_START_TITLE_CONNECT_ARGUMENT;
    asset_path = malloc(root_size + sizeof suffix);
    if (asset_path == NULL)
        return PR_COLD_START_TITLE_CONNECT_MEMORY;
    memcpy(asset_path, asset_root, root_size);
    memcpy(asset_path + root_size, suffix, sizeof suffix);
    if (strcmp(asset_path, output_path) == 0) {
        status = PR_COLD_START_TITLE_CONNECT_ARGUMENT;
        goto done;
    }

    file = fopen(asset_path, "rb");
    if (file == NULL || fseek(file, 0, SEEK_END) != 0 ||
        (file_size = ftell(file)) < 0 ||
        (unsigned long)file_size > TITLE_DAX_MAX_BYTES ||
        fseek(file, 0, SEEK_SET) != 0) {
        status = PR_COLD_START_TITLE_CONNECT_BLOCKED_ASSET;
        goto done;
    }
    asset_size = (size_t)file_size;
    asset = malloc(asset_size == 0u ? 1u : asset_size);
    if (asset == NULL) {
        status = PR_COLD_START_TITLE_CONNECT_MEMORY;
        goto done;
    }
    {
        size_t transferred = fread(asset, 1u, asset_size, file);
        int closed = fclose(file);
        file = NULL;
        if (transferred != asset_size || closed != 0) {
            status = PR_COLD_START_TITLE_CONNECT_BLOCKED_ASSET;
            goto done;
        }
    }

    frame = malloc(PR_COLD_TITLE_RGB24_BYTES);
    if (frame == NULL) {
        status = PR_COLD_START_TITLE_CONNECT_MEMORY;
        goto done;
    }
    if (pr_cold_title_rgb24(asset, asset_size, frame,
                            PR_COLD_TITLE_RGB24_BYTES,
                            &written) != PR_COLD_TITLE_OK ||
        written != PR_COLD_TITLE_RGB24_BYTES) {
        status = PR_COLD_START_TITLE_CONNECT_BLOCKED_FRAME;
        goto done;
    }

    file = fopen(output_path, "wbx");
    if (file == NULL) {
        status = PR_COLD_START_TITLE_CONNECT_OUTPUT;
        goto done;
    }
    {
        size_t transferred = fwrite(frame, 1u,
                                    PR_COLD_TITLE_RGB24_BYTES, file);
        int closed = fclose(file);
        file = NULL;
        status = (transferred == PR_COLD_TITLE_RGB24_BYTES && closed == 0)
            ? PR_COLD_START_TITLE_CONNECT_OK
            : PR_COLD_START_TITLE_CONNECT_OUTPUT;
        if (status != PR_COLD_START_TITLE_CONNECT_OK)
            (void)remove(output_path);
    }

done:
    if (file != NULL) fclose(file);
    free(frame);
    free(asset);
    free(asset_path);
    return status;
}

const char *pr_cold_start_title_connect_status_string(
    pr_cold_start_title_connect_status status) {
    switch (status) {
    case PR_COLD_START_TITLE_CONNECT_OK: return "ok";
    case PR_COLD_START_TITLE_CONNECT_ARGUMENT: return "argument";
    case PR_COLD_START_TITLE_CONNECT_BLOCKED_ASSET: return "blocked asset";
    case PR_COLD_START_TITLE_CONNECT_BLOCKED_FRAME: return "blocked title frame";
    case PR_COLD_START_TITLE_CONNECT_OUTPUT: return "output";
    case PR_COLD_START_TITLE_CONNECT_MEMORY: return "memory";
    }
    return "unknown";
}
