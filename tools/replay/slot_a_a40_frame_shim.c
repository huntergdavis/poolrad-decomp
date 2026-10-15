/* Standalone probe bridge for the existing bounded A40 C model. */
#include "poolrad/recovered/resident_a40_delay_029c.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t bios_byte;
    uint16_t calibration;
} pr_slot_a_a40_fixture;

static int read_bios(void *ctx, uint8_t *value) {
    const pr_slot_a_a40_fixture *fixture = ctx;
    *value = fixture->bios_byte;
    return 0;
}

static int read_calibration(void *ctx, uint16_t *value) {
    const pr_slot_a_a40_fixture *fixture = ctx;
    *value = fixture->calibration;
    return 0;
}

/* Flat output avoids depending on C structure padding in the Python audit.
 * Indices: requested, completed, BIOS reads, calibration reads, spin calls,
 * BX decrements, baseline, observed, last calibration, final CX. */
int pr_slot_a_a40_frame_run(uint16_t units, uint8_t bios_byte,
                            uint16_t calibration, uint64_t max_operations,
                            uint64_t output[10]) {
    pr_slot_a_a40_fixture fixture = {bios_byte, calibration};
    pr_a40_delay_env env = {&fixture, read_bios, read_calibration,
                            max_operations};
    pr_a40_delay_report report;
    pr_a40_delay_status status;

    if (output == NULL) return (int)PR_A40_DELAY_ARGUMENT;
    status = pr_a40_delay_029c_run(units, &env, &report);
    output[0] = report.requested_units;
    output[1] = report.completed_units;
    output[2] = report.bios_reads;
    output[3] = report.calibration_reads;
    output[4] = report.spin_calls;
    output[5] = report.bx_decrements;
    output[6] = report.last_baseline;
    output[7] = report.last_observed;
    output[8] = report.last_calibration;
    output[9] = report.cx_after;
    return (int)status;
}
