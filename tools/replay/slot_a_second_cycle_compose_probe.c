#include "poolrad/recovered/slot_a_printer_page.h"
#include "poolrad/recovered/slot_a_second_cycle_zero_row.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    PLANE_BYTES = 8000,
    PLANE_COUNT = 4,
    CALL_COUNT = 18,
    TEXT_CAP = 255,
    CALL_META_BYTES = 9,
    CALL_BYTES = CALL_META_BYTES + TEXT_CAP,
    INITIAL_BYTES = PLANE_COUNT * PLANE_BYTES,
    INPUT_BYTES = PR_FONT201_BLOCK_SIZE + INITIAL_BYTES + CALL_COUNT * CALL_BYTES
};

static uint8_t input[INPUT_BYTES];
static uint8_t storage[PLANE_COUNT][PLANE_BYTES];

/* Private record: one font, one initial four-plane page, then 18 records.
 * Each record has clear flag/row/left/right/color, printer
 * length/column/row/color, and a 255-byte padded source. */

static int write_page(FILE *output) {
    size_t plane;
    for (plane = 0u; plane < PLANE_COUNT; ++plane) {
        if (fwrite(storage[plane], 1u, PLANE_BYTES, output) != PLANE_BYTES) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv) {
    FILE *source, *output;
    pr_font201 font;
    uint8_t *page[PLANE_COUNT] = {
        storage[0], storage[1], storage[2], storage[3]
    };
    size_t plane, call;
    size_t printer_cells = 0u, printer_stores = 0u, printer_changes = 0u;
    size_t clear_calls = 0u, clear_stores = 0u, clear_changes = 0u;
    const size_t initial_at = PR_FONT201_BLOCK_SIZE;
    const size_t calls_at = initial_at + INITIAL_BYTES;

    if (argc != 3) {
        fprintf(stderr, "usage: probe PRIVATE_INPUT PRIVATE_OUTPUT\n");
        return 2;
    }
    source = fopen(argv[1], "rb");
    if (source == NULL) {
        perror("input");
        return 2;
    }
    {
        size_t read_count = fread(input, 1u, sizeof input, source);
        int trailing = fgetc(source);
        int read_error = ferror(source);
        int close_error = fclose(source);
        if (read_count != sizeof input || trailing != EOF ||
            read_error != 0 || close_error != 0) {
            fprintf(stderr, "invalid private input size\n");
            return 2;
        }
    }
    if (pr_font201_open(input, PR_FONT201_BLOCK_SIZE, &font) != PR_FONT201_OK) {
        fprintf(stderr, "invalid private font\n");
        return 2;
    }
    for (plane = 0u; plane < PLANE_COUNT; ++plane) {
        memcpy(storage[plane], input + initial_at + plane * PLANE_BYTES,
               PLANE_BYTES);
    }
    output = fopen(argv[2], "wb");
    if (output == NULL) {
        perror("output");
        return 2;
    }
    for (call = 0u; call < CALL_COUNT; ++call) {
        const uint8_t *record = input + calls_at + call * CALL_BYTES;
        pr_slot_a_printer_page_report printer;
        if (record[0] != 0u) {
            pr_slot_a_second_zero_row_report clear;
            if (record[0] != 1u ||
                pr_slot_a_second_zero_row_apply(
                    page, 0u, PLANE_BYTES, record[1], record[2], record[3],
                    record[4], &clear) != PR_SLOT_A_SECOND_ZERO_ROW_OK) {
                fclose(output);
                fprintf(stderr, "unsupported clear before call %zu\n", call + 25u);
                return 2;
            }
            ++clear_calls;
            clear_stores += clear.plane_stores;
            clear_changes += clear.changed_plane_bytes;
        } else if (record[1] != 0u || record[2] != 0u ||
                   record[3] != 0u || record[4] != 0u) {
            fclose(output);
            fprintf(stderr, "unexpected clear arguments before call %zu\n",
                    call + 25u);
            return 2;
        }
        if (!write_page(output)) {
            fclose(output);
            fprintf(stderr, "entry page write failed\n");
            return 2;
        }
        if (pr_slot_a_printer_page_apply(
                page, 0u, PLANE_BYTES, &font,
                record[6], record[7], record[8],
                (const char *)(record + CALL_META_BYTES), record[5],
                &printer) != PR_SLOT_A_PRINTER_PAGE_OK) {
            fclose(output);
            fprintf(stderr, "printer model rejected call %zu\n", call + 25u);
            return 2;
        }
        printer_cells += printer.cells;
        printer_stores += printer.plane_stores;
        printer_changes += printer.changed_plane_bytes;
        if (!write_page(output)) {
            fclose(output);
            fprintf(stderr, "return page write failed\n");
            return 2;
        }
    }
    if (fclose(output) != 0) {
        fprintf(stderr, "output close failed\n");
        return 2;
    }
    printf("calls=%u printer_cells=%zu printer_stores=%zu "
           "printer_changed=%zu clear_calls=%zu clear_stores=%zu "
           "clear_changed=%zu\n",
           CALL_COUNT, printer_cells, printer_stores, printer_changes,
           clear_calls, clear_stores, clear_changes);
    return 0;
}
