#include "poolrad/replay/cold_start_publisher_rgb.h"

#include "poolrad/formats/dax.h"
#include "poolrad/formats/ega.h"

#include <stdlib.h>
#include <string.h>

static const uint8_t ega_rgb[16][3] = {
    {0, 0, 0},       {0, 0, 170},     {0, 170, 0},     {0, 170, 170},
    {170, 0, 0},     {170, 0, 170},   {170, 85, 0},   {170, 170, 170},
    {85, 85, 85},    {85, 85, 255},  {85, 255, 85},  {85, 255, 255},
    {255, 85, 85},  {255, 85, 255}, {255, 255, 85}, {255, 255, 255}
};

pr_cold_publisher_status pr_cold_publisher_rgb24(const uint8_t *title_dax,
                                                 size_t title_dax_size,
                                                 uint8_t *rgb24,
                                                 size_t capacity,
                                                 size_t *written) {
    pr_dax_archive archive;
    pr_ega_block image;
    uint8_t *block = NULL, *indices = NULL;
    size_t block_index, block_size = 0, index_size = 0, y, x;
    pr_cold_publisher_status result = PR_COLD_PUBLISHER_OK;

    if (!written) return PR_COLD_PUBLISHER_ARGUMENT;
    *written = 0;
    if (!title_dax) return PR_COLD_PUBLISHER_ARGUMENT;
    if (pr_dax_open(title_dax, title_dax_size, &archive) != PR_DAX_OK)
        return PR_COLD_PUBLISHER_ARCHIVE;
    for (block_index = 0; block_index < archive.count; ++block_index)
        if (archive.entries[block_index].id == 1u) break;
    if (block_index == archive.count) return PR_COLD_PUBLISHER_BLOCK;

    block_size = archive.entries[block_index].unpacked_size;
    block = malloc(block_size);
    if (!block) return PR_COLD_PUBLISHER_MEMORY;
    if (pr_dax_read(&archive, block_index, block, block_size, &block_size) !=
        PR_DAX_OK) {
        result = PR_COLD_PUBLISHER_BLOCK;
        goto done;
    }
    if (pr_ega_open(block, block_size, &image) != PR_EGA_OK ||
        image.width_pixels != 320u || image.height != 200u ||
        image.item_count != 1u) {
        result = PR_COLD_PUBLISHER_IMAGE;
        goto done;
    }
    *written = PR_COLD_PUBLISHER_RGB24_BYTES;
    if (!rgb24 || capacity < PR_COLD_PUBLISHER_RGB24_BYTES) {
        result = PR_COLD_PUBLISHER_OUTPUT_SIZE;
        goto done;
    }
    indices = malloc(320u * 200u);
    if (!indices) {
        *written = 0;
        result = PR_COLD_PUBLISHER_MEMORY;
        goto done;
    }
    if (pr_ega_decode_item(&image, 0u, indices, 320u * 200u,
                           &index_size) != PR_EGA_OK ||
        index_size != 320u * 200u) {
        *written = 0;
        result = PR_COLD_PUBLISHER_IMAGE;
        goto done;
    }
    for (y = 0; y < 200u; ++y) {
        uint8_t *upper = rgb24 + y * 2u * PR_COLD_PUBLISHER_WIDTH * 3u;
        uint8_t *lower = upper + PR_COLD_PUBLISHER_WIDTH * 3u;
        for (x = 0; x < 320u; ++x) {
            const uint8_t *colour = ega_rgb[indices[y * 320u + x]];
            memcpy(upper + x * 6u, colour, 3u);
            memcpy(upper + x * 6u + 3u, colour, 3u);
        }
        memcpy(lower, upper, PR_COLD_PUBLISHER_WIDTH * 3u);
    }
done:
    free(indices);
    free(block);
    return result;
}

const char *pr_cold_publisher_status_string(pr_cold_publisher_status status) {
    switch (status) {
    case PR_COLD_PUBLISHER_OK: return "ok";
    case PR_COLD_PUBLISHER_ARGUMENT: return "invalid argument";
    case PR_COLD_PUBLISHER_ARCHIVE: return "invalid DAX archive";
    case PR_COLD_PUBLISHER_BLOCK: return "missing or invalid block ID 1";
    case PR_COLD_PUBLISHER_IMAGE: return "invalid 320x200 EGA image";
    case PR_COLD_PUBLISHER_OUTPUT_SIZE: return "RGB24 output buffer too small";
    case PR_COLD_PUBLISHER_MEMORY: return "allocation failure";
    default: return "unknown error";
    }
}
