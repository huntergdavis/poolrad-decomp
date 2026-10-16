#ifndef POOLRAD_REPLAY_COLD_START_CREDITS_EXPECTED_CELLS_H
#define POOLRAD_REPLAY_COLD_START_CREDITS_EXPECTED_CELLS_H

#include <stddef.h>
#include <stdint.h>

#define PR_COLD_CREDITS_EXPECTED_CELL_COUNT 459u

typedef struct {
    uint8_t row;
    uint8_t column;
    uint8_t color;
    uint8_t text_byte;
    uint8_t call_index;
    uint8_t text_index;
} pr_cold_credits_expected_cell;

typedef enum {
    PR_COLD_CREDITS_CELLS_OK = 0,
    PR_COLD_CREDITS_CELLS_ARGUMENT,
    PR_COLD_CREDITS_CELLS_INPUT_SIZE,
    PR_COLD_CREDITS_CELLS_SOURCE,
    PR_COLD_CREDITS_CELLS_CALL,
    PR_COLD_CREDITS_CELLS_PLACEMENT,
    PR_COLD_CREDITS_CELLS_OUTPUT_SIZE
} pr_cold_credits_cells_status;

/* Independently decode the 29 contiguous Pascal records and 33 fixed-stride
 * U17 printer callers from an original DOS v1.3 GAME.OVR image. Output is in
 * caller order, then text order. It contains source bytes, not rendered glyphs.
 * No game text or image data is embedded. On OUTPUT_SIZE, needed is set to 459;
 * on malformed input, needed is zero. In either case output is unchanged. */
pr_cold_credits_cells_status pr_cold_credits_expected_cells(
    const uint8_t *ovr, size_t ovr_size,
    pr_cold_credits_expected_cell *output, size_t capacity, size_t *needed);

#endif
