#include "poolrad/recovered/resident_464_first_stage_row.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define PLANE_SIZE 32u

static void reset_planes(uint8_t stage[4][PLANE_SIZE],
                         uint8_t page[4][PLANE_SIZE])
{
    memset(stage, 0xA5, 4u * PLANE_SIZE);
    memset(page, 0x5A, 4u * PLANE_SIZE);
}

static void unchanged(uint8_t stage[4][PLANE_SIZE],
                      uint8_t page[4][PLANE_SIZE])
{
    unsigned plane;
    size_t at;
    for (plane = 0; plane < 4u; ++plane)
        for (at = 0; at < PLANE_SIZE; ++at) {
            assert(stage[plane][at] == 0xA5u);
            assert(page[plane][at] == 0x5Au);
        }
}

int main(void)
{
    pr_r464_dirty_state state = {0};
    pr_r464_first_stage_report report;
    uint8_t object[96];
    uint8_t stage[4][PLANE_SIZE];
    uint8_t page[4][PLANE_SIZE];
    unsigned plane;
    size_t at;

    for (at = 0; at < sizeof object; ++at)
        object[at] = (uint8_t)(3u * at + 1u);
    state.gc_bit_mask = 0xFFu;
    state.vram_size = PLANE_SIZE;
    for (plane = 0; plane < 4u; ++plane) {
        state.map_mask[plane] = (uint8_t)(1u << plane);
        state.staging[plane] = stage[plane];
        state.page[plane] = page[plane];
    }
    state.row[16].dirty = 1u;
    state.row[16].first = 4u;
    state.row[16].last = 15u;
    state.row[16].stage_base = 3u;
    state.row[16].low = 1u;
    state.row[22].dirty = 1u;
    state.row[22].first = 0u;
    state.row[22].last = 7u;
    reset_planes(stage, page);
    memset(&report, 0xCC, sizeof report);
    assert(pr_r464_first_stage_row(&state, 1u, object, sizeof object,
                                   &report) == PR_R464_FIRST_STAGE_OK);
    assert(report.row == 16u && report.first == 4u && report.last == 15u);
    assert(report.stage_offset == 5u && report.bytes_per_plane == 3u &&
           report.plane_writes == 12u);
    for (plane = 0; plane < 4u; ++plane)
        for (at = 0; at < PLANE_SIZE; ++at) {
            uint8_t expected = 0xA5u;
            if (at >= 5u && at < 8u)
                expected = object[0x17u + 4u + plane + 4u * (at - 5u)];
            assert(stage[plane][at] == expected);
            assert(page[plane][at] == 0x5Au);
        }
    assert(state.row[16].dirty == 1u && state.row[22].dirty == 1u);

    /* A second earlier dirty row is selected before row 16. */
    state.row[0].dirty = 1u;
    state.row[0].first = 0u;
    state.row[0].last = 7u;
    state.row[0].stage_base = 0u;
    state.row[0].low = 0u;
    reset_planes(stage, page);
    assert(pr_r464_first_stage_row(&state, 1u, object, sizeof object,
                                   &report) == PR_R464_FIRST_STAGE_OK);
    assert(report.row == 0u && report.bytes_per_plane == 2u &&
           report.stage_offset == 1u);
    for (plane = 0; plane < 4u; ++plane) {
        assert(stage[plane][1] == object[0x17u + plane]);
        assert(stage[plane][2] == object[0x17u + plane + 4u]);
    }

    state.row[0].dirty = 0u;
    reset_planes(stage, page);
    memset(&report, 0xCC, sizeof report);
    assert(pr_r464_first_stage_row(&state, 0u, object, sizeof object,
                                   &report) == PR_R464_FIRST_STAGE_OTHER_PATH);
    unchanged(stage, page);
    assert(report.row == 0xCCCCCCCCu);

    state.row[16].last = 14u;
    assert(pr_r464_first_stage_row(&state, 1u, object, sizeof object,
                                   &report) == PR_R464_FIRST_STAGE_OTHER_PATH);
    unchanged(stage, page);
    state.row[16].last = 15u;
    assert(pr_r464_first_stage_row(&state, 1u, object, 0x17u + 15u,
                                   &report) == PR_R464_FIRST_STAGE_BOUNDS);
    unchanged(stage, page);
    state.vram_size = 7u;
    assert(pr_r464_first_stage_row(&state, 1u, object, sizeof object,
                                   &report) == PR_R464_FIRST_STAGE_BOUNDS);
    unchanged(stage, page);
    state.vram_size = PLANE_SIZE;
    state.staging[3] = NULL;
    assert(pr_r464_first_stage_row(&state, 1u, object, sizeof object,
                                   &report) == PR_R464_FIRST_STAGE_BAD_ARG);
    unchanged(stage, page);
    state.staging[3] = stage[3];
    state.map_mask[2] = 0u;
    assert(pr_r464_first_stage_row(&state, 1u, object, sizeof object,
                                   &report) == PR_R464_FIRST_STAGE_OTHER_PATH);
    unchanged(stage, page);

    state.map_mask[2] = 4u;
    state.row[16].dirty = 0u;
    state.row[22].dirty = 0u;
    assert(pr_r464_first_stage_row(&state, 1u, object, sizeof object,
                                   &report) == PR_R464_FIRST_STAGE_OTHER_PATH);
    unchanged(stage, page);
    return 0;
}
