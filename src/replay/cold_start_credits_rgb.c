#include "poolrad/replay/cold_start_credits_rgb.h"

#include "poolrad/formats/dax.h"
#include "poolrad/formats/ega.h"
#include "poolrad/recovered/font_8x8d_201.h"

#include <stdlib.h>
#include <string.h>

#define OVR_CREDITS_BEGIN 0x02DAu
#define OVR_CREDITS_END 0x06F5u
#define OVR_CS_TO_FILE 0x00D4u
#define PRINTER_CALLS 33u
#define CELLS_X 40u
#define CELLS_Y 25u

static const uint8_t ega_rgb[16][3] = {
    {0, 0, 0},       {0, 0, 170},     {0, 170, 0},     {0, 170, 170},
    {170, 0, 0},     {170, 0, 170},   {170, 85, 0},   {170, 170, 170},
    {85, 85, 85},    {85, 85, 255},  {85, 255, 85},  {85, 255, 255},
    {255, 85, 85},  {255, 85, 255}, {255, 255, 85}, {255, 255, 255}
};

static int read_block(const pr_dax_archive *archive, uint8_t id,
                      uint8_t *out, size_t expected) {
    size_t i, written = 0u;
    for (i = 0u; i < archive->count; ++i) {
        if (archive->entries[i].id != id) continue;
        return archive->entries[i].unpacked_size == expected &&
               pr_dax_read(archive, i, out, expected, &written) == PR_DAX_OK &&
               written == expected;
    }
    return 0;
}

static void put_tile(pr_slot_a_view_frame *frame, const uint8_t tile[64],
                     size_t column, size_t row) {
    size_t y;
    for (y = 0u; y < 8u; ++y)
        memcpy(frame->index + (row * 8u + y) * 320u + column * 8u,
               tile + y * 8u, 8u);
}

static void put_box(pr_slot_a_view_frame *frame,
                    uint8_t tile[3][64], size_t top, size_t bottom) {
    size_t x, y;
    put_tile(frame, tile[0], 0u, top);
    put_tile(frame, tile[0], CELLS_X - 1u, top);
    put_tile(frame, tile[0], 0u, bottom);
    put_tile(frame, tile[0], CELLS_X - 1u, bottom);
    for (x = 1u; x < CELLS_X - 1u; ++x) {
        put_tile(frame, tile[2], x, top);
        put_tile(frame, tile[2], x, bottom);
    }
    for (y = top + 1u; y < bottom; ++y) {
        put_tile(frame, tile[1], 0u, y);
        put_tile(frame, tile[1], CELLS_X - 1u, y);
    }
}

/* Every source CALL FAR 0x709:0x626 in this procedure follows the same
 * 24-byte immediate/push/Copy setup. The local stack displacement varies;
 * all operands used for source text and display placement are checked. */
static int printer_setup(const uint8_t *p) {
    static const uint8_t fixed[24] = {
        0xB0u, 0u, 0x50u, 0xB0u, 0u, 0x50u, 0xB0u, 0u,
        0x50u, 0x8Du, 0x7Eu, 0u, 0x16u, 0x57u, 0xBFu, 0u,
        0u, 0x0Eu, 0x57u, 0x9Au, 0x0Bu, 0x0Bu, 0xF8u, 0x0Au
    };
    size_t i;
    for (i = 0u; i < sizeof fixed; ++i)
        if (i != 1u && i != 4u && i != 7u && i != 11u &&
            i != 15u && i != 16u && p[i] != fixed[i]) return 0;
    return 1;
}

static int print_script(pr_slot_a_view_frame *frame, const pr_font201 *font,
                        const uint8_t *ovr, size_t ovr_size) {
    static const uint8_t call[5] = {0x9Au, 0x26u, 0x06u, 0x09u, 0x07u};
    size_t p, calls = 0u;
    if (ovr_size < OVR_CREDITS_END) return 0;
    for (p = OVR_CREDITS_BEGIN + 24u; p + sizeof call <= OVR_CREDITS_END;
         ++p) {
        const uint8_t *setup;
        size_t source, length, drawn = 0u;
        uint8_t column, row, color;
        if (memcmp(ovr + p, call, sizeof call) != 0) continue;
        setup = ovr + p - 24u;
        if (!printer_setup(setup)) return 0;
        column = setup[1];
        row = setup[4];
        color = setup[7];
        source = OVR_CS_TO_FILE + (size_t)setup[15] +
                 ((size_t)setup[16] << 8u);
        if (source >= ovr_size || column >= CELLS_X || row >= CELLS_Y ||
            color > 15u) return 0;
        length = ovr[source];
        if (length > ovr_size - source - 1u ||
            length > CELLS_X - column ||
            pr_font201_print_ega(frame, font, column, row, color,
                                 (const char *)(ovr + source + 1u), length,
                                 &drawn) != PR_FONT201_OK ||
            drawn != length) return 0;
        ++calls;
    }
    return calls == PRINTER_CALLS;
}

static void expand_rgb(const pr_slot_a_view_frame *frame, uint8_t *rgb24) {
    size_t x, y;
    for (y = 0u; y < 200u; ++y) {
        uint8_t *upper = rgb24 + y * 2u * PR_COLD_CREDITS_WIDTH * 3u;
        uint8_t *lower = upper + PR_COLD_CREDITS_WIDTH * 3u;
        for (x = 0u; x < 320u; ++x) {
            const uint8_t *colour = ega_rgb[frame->index[y * 320u + x]];
            memcpy(upper + x * 6u, colour, 3u);
            memcpy(upper + x * 6u + 3u, colour, 3u);
        }
        memcpy(lower, upper, PR_COLD_CREDITS_WIDTH * 3u);
    }
}

pr_cold_credits_status pr_cold_credits_rgb24(
    const uint8_t *dax, size_t dax_size, const uint8_t *ovr, size_t ovr_size,
    uint8_t *rgb24, size_t capacity, size_t *written) {
    pr_dax_archive archive;
    pr_ega_block image;
    pr_font201 font;
    pr_slot_a_view_frame *frame = NULL;
    uint8_t font_data[PR_FONT201_BLOCK_SIZE], tile_data[1009u];
    uint8_t tile[3][64];
    pr_cold_credits_status result = PR_COLD_CREDITS_OK;
    size_t i, tile_size = 0u;

    if (!written) return PR_COLD_CREDITS_ARGUMENT;
    *written = 0u;
    if (!dax || !ovr) return PR_COLD_CREDITS_ARGUMENT;
    if (pr_dax_open(dax, dax_size, &archive) != PR_DAX_OK)
        return PR_COLD_CREDITS_ARCHIVE;
    if (!read_block(&archive, PR_FONT201_BLOCK_ID, font_data,
                    sizeof font_data) ||
        pr_font201_open(font_data, sizeof font_data, &font) != PR_FONT201_OK)
        return PR_COLD_CREDITS_FONT;
    if (!read_block(&archive, 202u, tile_data, sizeof tile_data) ||
        pr_ega_open(tile_data, sizeof tile_data, &image) != PR_EGA_OK ||
        image.width_pixels != 8u || image.height != 8u ||
        image.item_count < 23u) return PR_COLD_CREDITS_TILES;
    for (i = 0u; i < 3u; ++i)
        if (pr_ega_decode_item(&image, 20u + i, tile[i], sizeof tile[i],
                               &tile_size) != PR_EGA_OK || tile_size != 64u)
            return PR_COLD_CREDITS_TILES;
    frame = calloc(1u, sizeof *frame);
    if (!frame) return PR_COLD_CREDITS_MEMORY;
    put_box(frame, tile, 0u, 4u);
    put_box(frame, tile, 4u, 24u);
    if (!print_script(frame, &font, ovr, ovr_size)) {
        result = PR_COLD_CREDITS_SCRIPT;
        goto done;
    }
    *written = PR_COLD_CREDITS_RGB24_BYTES;
    if (!rgb24 || capacity < *written) {
        result = PR_COLD_CREDITS_OUTPUT_SIZE;
        goto done;
    }
    expand_rgb(frame, rgb24);
done:
    free(frame);
    return result;
}

const char *pr_cold_credits_status_string(pr_cold_credits_status status) {
    switch (status) {
    case PR_COLD_CREDITS_OK: return "ok";
    case PR_COLD_CREDITS_ARGUMENT: return "invalid argument";
    case PR_COLD_CREDITS_ARCHIVE: return "invalid DAX archive";
    case PR_COLD_CREDITS_FONT: return "missing or invalid DAX font block 201";
    case PR_COLD_CREDITS_TILES: return "missing or invalid DAX tile block 202";
    case PR_COLD_CREDITS_SCRIPT: return "invalid unit-0x17 credits call setup";
    case PR_COLD_CREDITS_OUTPUT_SIZE: return "RGB24 output buffer too small";
    case PR_COLD_CREDITS_MEMORY: return "allocation failure";
    default: return "unknown error";
    }
}
