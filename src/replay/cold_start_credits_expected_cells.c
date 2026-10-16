#include "poolrad/replay/cold_start_credits_expected_cells.h"

#include <string.h>

#define SOURCE_BEGIN 0x011Fu
#define SOURCE_END 0x02DAu
#define CALL_BEGIN 0x032Cu
#define CALL_END 0x06E9u
#define CS_TO_FILE 0x00D4u
#define SOURCE_RECORDS 29u
#define CALL_RECORDS 33u
#define CALL_STRIDE 29u

static int call_layout(const uint8_t *r) {
    return r[0] == 0xB0u && r[2] == 0x50u &&
           r[3] == 0xB0u && r[5] == 0x50u &&
           r[6] == 0xB0u && r[8] == 0x50u &&
           r[9] == 0x8Du && r[10] == 0x7Eu &&
           r[12] == 0x16u && r[13] == 0x57u &&
           r[14] == 0xBFu && r[17] == 0x0Eu && r[18] == 0x57u &&
           r[19] == 0x9Au && r[20] == 0x0Bu && r[21] == 0x0Bu &&
           r[22] == 0xF8u && r[23] == 0x0Au &&
           r[24] == 0x9Au && r[25] == 0x26u && r[26] == 0x06u &&
           r[27] == 0x09u && r[28] == 0x07u;
}

pr_cold_credits_cells_status pr_cold_credits_expected_cells(
    const uint8_t *ovr, size_t ovr_size,
    pr_cold_credits_expected_cell *output, size_t capacity, size_t *needed) {
    pr_cold_credits_expected_cell pending[PR_COLD_CREDITS_EXPECTED_CELL_COUNT];
    size_t starts[SOURCE_RECORDS], lengths[SOURCE_RECORDS];
    uint8_t uses[SOURCE_RECORDS] = {0};
    uint8_t occupied[40u * 25u] = {0};
    size_t pos = SOURCE_BEGIN, sources = 0u, cells = 0u, call;

    if (!needed) return PR_COLD_CREDITS_CELLS_ARGUMENT;
    *needed = 0u;
    if (!ovr) return PR_COLD_CREDITS_CELLS_ARGUMENT;
    if (ovr_size < CALL_END || CALL_END - CALL_BEGIN != CALL_RECORDS * CALL_STRIDE)
        return PR_COLD_CREDITS_CELLS_INPUT_SIZE;

    while (pos < SOURCE_END) {
        size_t length = ovr[pos];
        if (sources == SOURCE_RECORDS || !length ||
            length > SOURCE_END - pos - 1u)
            return PR_COLD_CREDITS_CELLS_SOURCE;
        starts[sources] = pos;
        lengths[sources] = length;
        ++sources;
        pos += length + 1u;
    }
    if (pos != SOURCE_END || sources != SOURCE_RECORDS)
        return PR_COLD_CREDITS_CELLS_SOURCE;

    for (call = 0u; call < CALL_RECORDS; ++call) {
        const uint8_t *r = ovr + CALL_BEGIN + call * CALL_STRIDE;
        size_t source, source_index, length, i;
        uint8_t row, column, color;
        if (!call_layout(r)) return PR_COLD_CREDITS_CELLS_CALL;
        source = CS_TO_FILE + (size_t)r[15] + ((size_t)r[16] << 8u);
        for (source_index = 0u; source_index < sources; ++source_index)
            if (starts[source_index] == source) break;
        if (source_index == sources) return PR_COLD_CREDITS_CELLS_SOURCE;
        ++uses[source_index];
        length = lengths[source_index];
        if (r[11] != (uint8_t)(0u - (length + 1u)))
            return PR_COLD_CREDITS_CELLS_CALL;
        column = r[1];
        row = r[4];
        color = r[7];
        if (!row || row >= 25u || !column || column >= 40u ||
            (color != 10u && color != 11u && color != 14u) ||
            length > 40u - column)
            return PR_COLD_CREDITS_CELLS_PLACEMENT;
        for (i = 0u; i < length; ++i) {
            size_t cell_index = (size_t)row * 40u + column + i;
            if (cells == PR_COLD_CREDITS_EXPECTED_CELL_COUNT ||
                occupied[cell_index])
                return PR_COLD_CREDITS_CELLS_PLACEMENT;
            occupied[cell_index] = 1u;
            pending[cells].row = row;
            pending[cells].column = (uint8_t)(column + i);
            pending[cells].color = color;
            pending[cells].text_byte = ovr[source + 1u + i];
            pending[cells].call_index = (uint8_t)call;
            pending[cells].text_index = (uint8_t)i;
            ++cells;
        }
    }
    if (cells != PR_COLD_CREDITS_EXPECTED_CELL_COUNT)
        return PR_COLD_CREDITS_CELLS_PLACEMENT;
    {
        size_t repeated = 0u, i;
        for (i = 0u; i < SOURCE_RECORDS; ++i) {
            if (uses[i] == 2u) ++repeated;
            else if (uses[i] != 1u) return PR_COLD_CREDITS_CELLS_SOURCE;
        }
        if (repeated != 4u) return PR_COLD_CREDITS_CELLS_SOURCE;
    }
    *needed = cells;
    if (!output || capacity < cells)
        return PR_COLD_CREDITS_CELLS_OUTPUT_SIZE;
    memcpy(output, pending, cells * sizeof *output);
    return PR_COLD_CREDITS_CELLS_OK;
}
