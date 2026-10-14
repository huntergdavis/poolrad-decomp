#include "poolrad/recovered/resident_464_dirty_transfer.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    pr_r464_dirty_state state = {0};
    pr_r464_dirty_report report = {0};
    uint8_t object[64] = {0};
    uint8_t staging[4][2], page[4][2], saved[4][2];
    unsigned plane, column;
    size_t stage_changes = 0, page_changes = 0;

    state.row[0].dirty = 1;
    state.row[0].first = 0;
    state.row[0].last = 7;
    state.row[0].stage_base = 0x0100;
    state.page_base = 0x0100;
    state.vram_origin = 0x0101;
    state.vram_size = 2;
    state.gc_bit_mask = 0xff;
    for (plane = 0; plane < 4; ++plane) {
        state.map_mask[plane] = (uint8_t)(1u << plane);
        state.staging[plane] = staging[plane];
        state.page[plane] = page[plane];
        for (column = 0; column < 2; ++column) {
            uint8_t value = (uint8_t)(0x20u + plane * 4u + column);
            object[0x17u + plane + 4u * column] = value;
            staging[plane][column] = value;
            page[plane][column] = value;
        }
    }
    memcpy(saved, staging, sizeof saved);

    /* The rows remain dirty, so the original path copies two bytes on
     * four planes. Every destination already holds the copied value. */
    assert(pr_r464_dirty_transfer(&state, object, sizeof object, &report)
           == PR_R464_DIRTY_OK);
    assert(report.dirty_rows == 1 && report.transfer_bytes == 2);
    assert(report.stage_plane_writes == 8 && report.page_plane_writes == 8);
    assert(!memcmp(staging, saved, sizeof saved));
    assert(!memcmp(page, saved, sizeof saved));
    assert(state.row[0].dirty == 0 && state.row[0].first == 0xffff &&
           state.row[0].last == 0);

    /* Re-arm the row and change one source plane byte. The same transfer
     * path now changes exactly that one byte in staging and page. */
    state.row[0].dirty = 1;
    state.row[0].first = 0;
    state.row[0].last = 7;
    object[0x17u + 2u + 4u] = 0xe5;
    assert(pr_r464_dirty_transfer(&state, object, sizeof object, &report)
           == PR_R464_DIRTY_OK);
    for (plane = 0; plane < 4; ++plane)
        for (column = 0; column < 2; ++column) {
            stage_changes += staging[plane][column] != saved[plane][column];
            page_changes += page[plane][column] != saved[plane][column];
        }
    assert(stage_changes == 1 && page_changes == 1);
    assert(staging[2][1] == 0xe5 && page[2][1] == 0xe5);
    assert(state.row[0].dirty == 0 && state.row[0].first == 0xffff &&
           state.row[0].last == 0);
    return 0;
}
