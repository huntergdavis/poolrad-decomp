#include "poolrad/recovered/resident_464_row56_transfer.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

static void setup(pr_r464_row56_state *state) {
    unsigned i;
    memset(state, 0, sizeof *state);
    state->dirty = 1;
    state->first = 0x1200;
    state->last = 0x122B;
    state->row_base = 0x0A00;
    state->low = 2;
    state->page_base = 0x0140;
    state->gc_bit_mask = 0xFF;
    for (i = 0; i < 4; ++i) state->map_mask[i] = (uint8_t)(1u << i);
    memset(&state->staging, 0xA5, sizeof state->staging);
    memset(&state->page, 0x5A, sizeof state->page);
}

static int test_four_plane_latch_copy(void) {
    uint8_t object[0x1243] = {0};
    pr_r464_row56_state state;
    uint16_t offset = 0;
    unsigned p, j;
    setup(&state);
    for (p = 0; p < 4; ++p)
        for (j = 0; j < PR_R464_ROW56_BYTES; ++j)
            object[0x17u + state.first + p + 4u * j] =
                (uint8_t)(p * 40u + j + 1u);
    CHECK(pr_r464_row56_transfer(&state, object, sizeof object, &offset)
          == PR_R464_ROW56_OK);
    CHECK(offset == 0x0A03);
    for (p = 0; p < 4; ++p)
        for (j = 0; j < PR_R464_ROW56_BYTES; ++j) {
            uint8_t expected = (uint8_t)(p * 40u + j + 1u);
            CHECK(state.staging.plane[p][j] == expected);
            CHECK(state.page.plane[p][j] == expected);
        }
    CHECK(state.dirty == 0 && state.first == 0xFFFF &&
          state.last == 0);
    return 0;
}

static int test_map_mask_and_preexisting_stage(void) {
    uint8_t object[0x1243] = {0};
    pr_r464_row56_state state;
    uint16_t offset = 0;
    unsigned j;
    setup(&state);
    state.map_mask[0] = 0x03;
    state.map_mask[1] = 0;
    state.map_mask[2] = 0x04;
    state.map_mask[3] = 0;
    for (j = 0; j < PR_R464_ROW56_BYTES; ++j) {
        object[0x17u + state.first + 4u * j] = (uint8_t)(j + 7u);
        object[0x17u + state.first + 2u + 4u * j] = (uint8_t)(j + 80u);
    }
    CHECK(pr_r464_row56_transfer(&state, object, sizeof object, &offset)
          == PR_R464_ROW56_OK);
    for (j = 0; j < PR_R464_ROW56_BYTES; ++j) {
        CHECK(state.page.plane[0][j] == (uint8_t)(j + 7u));
        CHECK(state.page.plane[1][j] == (uint8_t)(j + 7u));
        CHECK(state.page.plane[2][j] == (uint8_t)(j + 80u));
        CHECK(state.page.plane[3][j] == 0xA5);
    }
    return 0;
}

static int test_host_stops_without_effects(void) {
    uint8_t object[0x1243] = {0};
    pr_r464_row56_state state, before;
    uint16_t offset = 0xBEEF;
    setup(&state);
    before = state;
    CHECK(pr_r464_row56_transfer(&state, object, 0x1242, &offset)
          == PR_R464_ROW56_SHORT_OBJECT);
    CHECK(memcmp(&state, &before, sizeof state) == 0 && offset == 0xBEEF);
    state.gc_bit_mask = 0x7F;
    before = state;
    CHECK(pr_r464_row56_transfer(&state, object, sizeof object, &offset)
          == PR_R464_ROW56_UNSUPPORTED);
    CHECK(memcmp(&state, &before, sizeof state) == 0 && offset == 0xBEEF);
    state.gc_bit_mask = 0xFF;
    state.dirty = 0;
    CHECK(pr_r464_row56_transfer(&state, object, sizeof object, &offset)
          == PR_R464_ROW56_UNSUPPORTED);
    CHECK(pr_r464_row56_transfer(NULL, object, sizeof object, &offset)
          == PR_R464_ROW56_BAD_ARG);
    return 0;
}

int main(void) {
    if (test_four_plane_latch_copy()) return 1;
    if (test_map_mask_and_preexisting_stage()) return 1;
    if (test_host_stops_without_effects()) return 1;
    return 0;
}
