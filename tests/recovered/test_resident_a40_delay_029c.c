#include "poolrad/recovered/resident_a40_delay_029c.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

typedef struct {
    uint8_t baseline, changed;
    uint16_t calibration, second_calibration;
    unsigned change_at, fail_bios_at, fail_calibration_at;
    unsigned bios_calls, calibration_calls;
    char order[8];
    unsigned order_length;
} fixture;

static int read_bios(void *ctx, uint8_t *value) {
    fixture *f = ctx;
    ++f->bios_calls;
    if (f->order_length < sizeof f->order) f->order[f->order_length++] = 'B';
    if (f->fail_bios_at == f->bios_calls) return 7;
    *value = f->bios_calls >= f->change_at ? f->changed : f->baseline;
    return 0;
}

static int read_calibration(void *ctx, uint16_t *value) {
    fixture *f = ctx;
    ++f->calibration_calls;
    if (f->order_length < sizeof f->order) f->order[f->order_length++] = 'C';
    if (f->fail_calibration_at == f->calibration_calls) return 8;
    *value = f->calibration_calls == 2u ? f->second_calibration : f->calibration;
    return 0;
}

static void setup(fixture *f, pr_a40_delay_env *env, uint16_t calibration,
                  uint64_t budget) {
    memset(f, 0, sizeof *f);
    f->baseline = 248u;
    f->changed = 249u;
    f->change_at = UINT_MAX;
    f->calibration = calibration;
    f->second_calibration = calibration;
    *env = (pr_a40_delay_env){f, read_bios, read_calibration, budget};
}

static int test_zero_and_saved_calibration(void) {
    fixture f;
    pr_a40_delay_env env;
    pr_a40_delay_report report;

    CHECK(pr_a40_delay_029c_run(0, NULL, &report) == PR_A40_DELAY_OK);
    CHECK(report.requested_units == 0u && report.bios_reads == 0u);
    setup(&f, &env, 269u, 243001u);
    CHECK(pr_a40_delay_029c_run(900u, &env, &report) == PR_A40_DELAY_OK);
    CHECK(report.requested_units == 900u && report.completed_units == 900u);
    CHECK(report.bios_reads == 242101u && report.calibration_reads == 900u);
    CHECK(report.spin_calls == 242100u && report.bx_decrements == 968400u);
    CHECK(report.last_calibration == 269u && report.cx_after == 0u);
    CHECK(report.last_baseline == 248u && report.last_observed == 248u);
    CHECK(f.bios_calls == 242101u && f.calibration_calls == 900u);
    CHECK(f.order[0] == 'B' && f.order[1] == 'C' && f.order[2] == 'B');
    return 0;
}

static int test_changed_byte_and_calibration_reload(void) {
    fixture f;
    pr_a40_delay_env env;
    pr_a40_delay_report report;

    setup(&f, &env, 269u, 3u);
    f.baseline = 7u;
    f.changed = 8u;
    f.change_at = 2u;
    CHECK(pr_a40_delay_029c_run(1u, &env, &report) == PR_A40_DELAY_OK);
    CHECK(report.completed_units == 1u && report.bios_reads == 2u);
    CHECK(report.calibration_reads == 1u && report.spin_calls == 1u);
    CHECK(report.cx_after == 268u && report.last_observed == 8u);
    CHECK(memcmp(f.order, "BCB", 3u) == 0);

    setup(&f, &env, 1u, 6u);
    f.second_calibration = 2u;
    CHECK(pr_a40_delay_029c_run(2u, &env, &report) == PR_A40_DELAY_OK);
    CHECK(report.completed_units == 2u && report.calibration_reads == 2u);
    CHECK(report.bios_reads == 4u && report.spin_calls == 3u);
    CHECK(report.last_calibration == 2u && report.cx_after == 0u);

    setup(&f, &env, 4u, 5u);
    f.baseline = 7u;
    f.changed = 8u;
    f.change_at = 2u;
    CHECK(pr_a40_delay_029c_run(2u, &env, &report) == PR_A40_DELAY_OK);
    CHECK(report.bios_reads == 3u && report.calibration_reads == 2u);
    CHECK(report.last_baseline == 7u && report.last_observed == 8u);
    CHECK(report.spin_calls == 2u && report.completed_units == 2u);
    return 0;
}

static int test_wrap_and_first_reached_boundaries(void) {
    fixture f;
    pr_a40_delay_env env;
    pr_a40_delay_report report;

    setup(&f, &env, 0u, 65538u);
    CHECK(pr_a40_delay_029c_run(1u, &env, &report) == PR_A40_DELAY_OK);
    CHECK(report.completed_units == 1u && report.spin_calls == 65536u);
    CHECK(report.bios_reads == 65537u && report.calibration_reads == 1u);
    CHECK(report.cx_after == 0u);

    setup(&f, &env, 4u, 3u);
    CHECK(pr_a40_delay_029c_run(1u, &env, &report) == PR_A40_DELAY_BUDGET);
    CHECK(report.completed_units == 0u && report.bios_reads == 2u);
    CHECK(report.calibration_reads == 1u && report.spin_calls == 2u);
    CHECK(report.bx_decrements == 8u && report.cx_after == 3u);

    setup(&f, &env, 4u, 10u);
    env.read_bios_byte = NULL;
    CHECK(pr_a40_delay_029c_run(1u, &env, &report) == PR_A40_DELAY_EXTERNAL);
    CHECK(report.bios_reads == 0u && report.calibration_reads == 0u);
    env.read_bios_byte = read_bios;
    env.read_calibration_word = NULL;
    CHECK(pr_a40_delay_029c_run(1u, &env, &report) == PR_A40_DELAY_EXTERNAL);
    CHECK(report.bios_reads == 1u && report.calibration_reads == 0u);

    setup(&f, &env, 4u, 10u);
    f.fail_bios_at = 2u;
    CHECK(pr_a40_delay_029c_run(1u, &env, &report) == PR_A40_DELAY_CALLBACK);
    CHECK(report.callback_status == 7 && report.bios_reads == 1u);
    CHECK(report.calibration_reads == 1u && report.spin_calls == 1u);
    CHECK(report.cx_after == 4u);
    CHECK(pr_a40_delay_029c_run(1u, NULL, &report) == PR_A40_DELAY_ARGUMENT);
    CHECK(pr_a40_delay_029c_run(1u, &env, NULL) == PR_A40_DELAY_ARGUMENT);
    return 0;
}

static int test_u1c_timing_hook_adapter(void) {
    static pr_rec_ecl_loader_state state;
    fixture f;
    pr_a40_delay_u1c_adapter adapter;
    pr_u1c_delay_env u1c = {0};
    pr_u1c_delay_report u1c_report;

    setup(&f, &adapter.env, 1u, 2700u);
    adapter.status = PR_A40_DELAY_ARGUMENT;
    u1c.b49e2 = 4u;
    u1c.hooks.delay_a40_029c = pr_a40_delay_029c_u1c_hook;
    u1c.ctx = &adapter;
    memset(&state, 0, sizeof state);
    state.pc = 0x9C28u;
    CHECK(pr_u1c_op_delay(&state, &u1c, &u1c_report) == PR_U1C_DELAY_OK);
    CHECK(state.pc == 0x9C29u && u1c_report.ms == 900u);
    CHECK(adapter.status == PR_A40_DELAY_OK);
    CHECK(adapter.report.completed_units == 900u);
    CHECK(adapter.report.bios_reads == 901u &&
          adapter.report.calibration_reads == 900u);

    setup(&f, &adapter.env, 1u, 2700u);
    adapter.env.read_calibration_word = NULL;
    state.pc = 0x9C28u;
    CHECK(pr_u1c_op_delay(&state, &u1c, &u1c_report) == PR_U1C_DELAY_CALLBACK);
    CHECK(state.pc == 0x9C29u && u1c_report.ms == 900u);
    CHECK(u1c_report.callback_status == PR_A40_DELAY_EXTERNAL);
    CHECK(adapter.report.bios_reads == 1u && adapter.report.calibration_reads == 0u);
    return 0;
}

int main(void) {
    if (test_zero_and_saved_calibration()) return 1;
    if (test_changed_byte_and_calibration_reload()) return 1;
    if (test_wrap_and_first_reached_boundaries()) return 1;
    if (test_u1c_timing_hook_adapter()) return 1;
    puts("synthetic RES_A40:029C bounded delay passed");
    return 0;
}
