#include "poolrad/recovered/resident_dax_selector_push.h"
#include "poolrad/recovered/overlay_ecl_store_hook.h"

#include <stdio.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; \
} } while (0)

static int test_direct_word_and_snapshot(void) {
    struct {
        uint8_t before;
        pr_rec_dax_selector_state state;
        uint8_t after;
    } f = {0xA1, {0x7E, 0x91}, 0xB2};
    CHECK(pr_rec_res_0802_094a(&f.state, 0x0342) == 0x42);
    CHECK(f.state.b5376 == 0x42 && f.state.b5377 == 0x7E);
    CHECK(pr_rec_res_0802_094a(&f.state, 0xAB03) == 3);
    CHECK(f.state.b5376 == 3 && f.state.b5377 == 0x42);
    CHECK(pr_rec_res_0802_094a(&f.state, 0x0003) == 3);
    CHECK(f.state.b5376 == 3 && f.state.b5377 == 3);
    CHECK(f.before == 0xA1 && f.after == 0xB2);
    return 0;
}

static int test_hook_exact_pushed_word(void) {
    pr_rec_dax_selector_state state = {0x55, 0xAA};
    pr_u25_hook_flags flags = {0};
    pr_u25_hook_callouts callouts = {pr_rec_res_0802_094a_hook, NULL, &state};
    pr_u25_hook_report report;
    CHECK(pr_u25_store_hook_0a74(0x6E12, 0xFF42, NULL, &flags,
                                 &callouts, &report) == PR_U25_HOOK_OK);
    CHECK(report.which == PR_U25_HOOK_CASE_312 &&
          report.callout_made == 1 && report.callout_value == 0x0342);
    CHECK(state.b5376 == 0x42 && state.b5377 == 0x55);

    state.b5376 = 0x88;
    state.b5377 = 0x99;
    CHECK(pr_u25_store_hook_0a74(0x6E12, 0x0042, NULL, &flags,
                                 &callouts, &report) == PR_U25_HOOK_OK);
    CHECK(report.callout_value == 0x0342);
    CHECK(state.b5376 == 0x42 && state.b5377 == 0x88);
    return 0;
}

static int test_high_byte_independence(void) {
    unsigned high;
    for (high = 0; high <= 255; ++high) {
        pr_rec_dax_selector_state state = {0xE7, 0xD6};
        uint16_t word = (uint16_t)((high << 8) | 0x35u);
        CHECK(pr_rec_res_0802_094a(&state, word) == 0x35);
        CHECK(state.b5376 == 0x35 && state.b5377 == 0xE7);
    }
    return 0;
}

int main(void) {
    if (test_direct_word_and_snapshot() || test_hook_exact_pushed_word() ||
        test_high_byte_independence()) return 1;
    puts("resident DAX selector push tests passed");
    return 0;
}
