#include "poolrad/recovered/resident_a40_delay_029c.h"

#include <string.h>

static uint64_t operations(const pr_a40_delay_report *report) {
    return report->bios_reads + report->calibration_reads;
}

static pr_a40_delay_status bios_read(const pr_a40_delay_env *env,
                                      pr_a40_delay_report *report,
                                      uint8_t *value) {
    int rc;
    if (env->read_bios_byte == NULL) {
        return PR_A40_DELAY_EXTERNAL;
    }
    if (operations(report) >= env->max_operations) {
        return PR_A40_DELAY_BUDGET;
    }
    rc = env->read_bios_byte(env->ctx, value);
    if (rc != 0) {
        report->callback_status = rc;
        return PR_A40_DELAY_CALLBACK;
    }
    ++report->bios_reads;
    return PR_A40_DELAY_OK;
}

static pr_a40_delay_status calibration_read(const pr_a40_delay_env *env,
                                             pr_a40_delay_report *report,
                                             uint16_t *value) {
    int rc;
    if (env->read_calibration_word == NULL) {
        return PR_A40_DELAY_EXTERNAL;
    }
    if (operations(report) >= env->max_operations) {
        return PR_A40_DELAY_BUDGET;
    }
    rc = env->read_calibration_word(env->ctx, value);
    if (rc != 0) {
        report->callback_status = rc;
        return PR_A40_DELAY_CALLBACK;
    }
    ++report->calibration_reads;
    return PR_A40_DELAY_OK;
}

pr_a40_delay_status pr_a40_delay_029c_run(
    uint16_t units, const pr_a40_delay_env *env, pr_a40_delay_report *report) {
    uint32_t unit;
    uint8_t baseline;
    pr_a40_delay_status status;

    if (report == NULL) {
        return PR_A40_DELAY_ARGUMENT;
    }
    memset(report, 0, sizeof *report);
    report->requested_units = units;
    if (units == 0u) {
        return PR_A40_DELAY_OK; /* +02A2, +02BA: neither external read is reached */
    }
    if (env == NULL) {
        return PR_A40_DELAY_ARGUMENT;
    }
    /* +02AD precedes the +02B0 loop target. The baseline is read once,
     * including when multiple delay units are requested. */
    status = bios_read(env, report, &baseline);
    if (status != PR_A40_DELAY_OK) return status;
    report->last_baseline = baseline;
    for (unit = 0; unit < units; ++unit) {
        uint8_t observed;
        uint16_t cx;

        /* +02B0 is the DX loop target: calibration is reloaded each unit. */
        status = calibration_read(env, report, &cx);
        if (status != PR_A40_DELAY_OK) return status;
        report->last_calibration = cx;
        report->cx_after = cx;

        /* +02B4 enters +02BD at least once. The near helper decrements BX
         * four times, reads BIOS again, then LOOPE decrements CX even when
         * the byte changed. An initial CX=0 wraps and permits 65,536 polls. */
        do {
            ++report->spin_calls;
            report->bx_decrements += 4u;
            status = bios_read(env, report, &observed);
            if (status != PR_A40_DELAY_OK) return status;
            report->last_observed = observed;
            cx = (uint16_t)(cx - 1u);
            report->cx_after = cx;
        } while (observed == baseline && cx != 0u);
        ++report->completed_units; /* +02B7: DX decremented after near return */
    }
    return PR_A40_DELAY_OK;
}

int pr_a40_delay_029c_u1c_hook(void *ctx, pr_u1c_delay_env *unused,
                                 uint16_t units) {
    pr_a40_delay_u1c_adapter *adapter = ctx;
    (void)unused;
    if (adapter == NULL) {
        return (int)PR_A40_DELAY_ARGUMENT;
    }
    adapter->status = pr_a40_delay_029c_run(units, &adapter->env,
                                              &adapter->report);
    return (int)adapter->status;
}
