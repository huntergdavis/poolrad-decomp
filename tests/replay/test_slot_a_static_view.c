#include "poolrad/replay/slot_a_static_view.h"

#include "poolrad/formats/dos_savgam.h"
#include "poolrad/formats/geo.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static int test_save_to_geo_context(void) {
    uint8_t save[PR_DOS_SAVGAM_SIZE] = {0};
    uint8_t geo[PR_GEO_SIZE] = {0};
    pr_slot_a_native_state state;
    pr_slot_a_view_context context, unchanged;
    size_t cell = 4u * PR_GEO_WIDTH + 15u;

    save[0x01E5u] = 20u;
    save[0x3201u] = 15u;
    save[0x3202u] = 4u;
    save[0x3203u] = 6u;
    save[1u + PR_SLOT_A_CLOCK_TENS_OFFSET] = 5u;
    save[1u + PR_SLOT_A_CLOCK_HOUR_OFFSET] = 10u;
    geo[2u + cell] = 0xABu;
    geo[2u + 256u + cell] = 0xCDu;
    geo[514u + cell] = 0x42u;
    geo[770u + cell] = 0xE4u;

    CHECK(pr_slot_a_native_restore(save, sizeof save, &state) ==
          PR_SLOT_A_NATIVE_OK);
    CHECK(pr_slot_a_view_prepare(&state, geo, sizeof geo, 20u, &context) ==
          PR_SLOT_A_VIEW_OK);
    CHECK(context.map_id == 20u);
    CHECK(context.saved.x == 15u && context.saved.y == 4u &&
          context.saved.facing == 6u);
    CHECK(context.saved.minute == 0u && context.saved.tens == 5u &&
          context.saved.hour == 10u);
    CHECK(context.wall[0] == 10u && context.wall[1] == 11u &&
          context.wall[2] == 12u && context.wall[3] == 13u);
    CHECK(context.door[0] == 0u && context.door[1] == 1u &&
          context.door[2] == 2u && context.door[3] == 3u);
    CHECK(context.event == 0x42u);

    unchanged = context;
    CHECK(pr_slot_a_view_prepare(&state, geo, sizeof geo, 19u, &context) ==
          PR_SLOT_A_VIEW_MAP_ID);
    CHECK(memcmp(&context, &unchanged, sizeof context) == 0);
    CHECK(pr_slot_a_view_prepare(&state, geo, sizeof geo - 1u, 20u,
                                 &context) == PR_SLOT_A_VIEW_GEO);
    CHECK(memcmp(&context, &unchanged, sizeof context) == 0);
    state.party[0] = 16u;
    CHECK(pr_slot_a_view_prepare(&state, geo, sizeof geo, 20u, &context) ==
          PR_SLOT_A_VIEW_POSITION);
    CHECK(memcmp(&context, &unchanged, sizeof context) == 0);
    CHECK(pr_slot_a_view_prepare(NULL, geo, sizeof geo, 20u, &context) ==
          PR_SLOT_A_VIEW_ARGUMENT);
    return 0;
}

static int test_bounded_art_blit(void) {
    uint8_t data[81] = {0};
    pr_ega_block image, invalid;
    pr_slot_a_view_frame frame, saved;
    size_t row;

    data[0] = 8u;
    data[2] = 1u;
    data[8] = 2u;
    for (row = 0; row < 8u; ++row) {
        memset(data + 17u + row * 4u, 0xA5, 4u);
        memset(data + 49u + row * 4u, 0x01, 4u);
    }
    CHECK(pr_ega_open(data, sizeof data, &image) == PR_EGA_OK);
    pr_slot_a_view_clear(&frame);
    CHECK(pr_slot_a_view_covered(&frame) == 0u);
    CHECK(pr_slot_a_view_blit_tile(&frame, &image, 0u, 312u, 192u, 0) ==
          PR_SLOT_A_VIEW_OK);
    CHECK(pr_slot_a_view_covered(&frame) == 64u);
    CHECK(frame.index[192u * PR_SLOT_A_VIEW_WIDTH + 312u] == 10u);
    CHECK(frame.index[192u * PR_SLOT_A_VIEW_WIDTH + 313u] == 5u);
    CHECK(frame.index[199u * PR_SLOT_A_VIEW_WIDTH + 319u] == 5u);
    CHECK(pr_slot_a_view_blit_tile(&frame, &image, 1u, 312u, 192u, 1) ==
          PR_SLOT_A_VIEW_OK);
    CHECK(pr_slot_a_view_covered(&frame) == 64u);
    CHECK(frame.index[192u * PR_SLOT_A_VIEW_WIDTH + 312u] == 10u);
    CHECK(frame.index[192u * PR_SLOT_A_VIEW_WIDTH + 313u] == 1u);

    saved = frame;
    CHECK(pr_slot_a_view_blit_tile(&frame, &image, 0u, 313u, 192u, 0) ==
          PR_SLOT_A_VIEW_BOUNDS);
    CHECK(pr_slot_a_view_blit_tile(&frame, &image, 0u, 312u, 193u, 0) ==
          PR_SLOT_A_VIEW_BOUNDS);
    CHECK(pr_slot_a_view_blit_tile(&frame, &image, 2u, 0u, 0u, 0) ==
          PR_SLOT_A_VIEW_ITEM);
    invalid = image;
    invalid.item_bytes++;
    CHECK(pr_slot_a_view_blit_tile(&frame, &invalid, 0u, 0u, 0u, 0) ==
          PR_SLOT_A_VIEW_IMAGE);
    CHECK(memcmp(&frame, &saved, sizeof frame) == 0);
    CHECK(pr_slot_a_view_blit_tile(NULL, &image, 0u, 0u, 0u, 0) ==
          PR_SLOT_A_VIEW_ARGUMENT);
    CHECK(pr_slot_a_view_blit_tile(&frame, NULL, 0u, 0u, 0u, 0) ==
          PR_SLOT_A_VIEW_ARGUMENT);
    CHECK(pr_slot_a_view_covered(NULL) == 0u);
    return 0;
}

int main(void) {
    if (test_save_to_geo_context() || test_bounded_art_blit()) return 1;
    puts("synthetic slot-A static view context and art blit passed");
    return 0;
}
