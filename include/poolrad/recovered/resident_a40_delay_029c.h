#ifndef POOLRAD_RECOVERED_RESIDENT_A40_DELAY_029C_H
#define POOLRAD_RECOVERED_RESIDENT_A40_DELAY_029C_H

#include <stdint.h>

#include "poolrad/recovered/overlay_u1c_delay_271f.h"

typedef enum {
    PR_A40_DELAY_OK = 0,
    PR_A40_DELAY_ARGUMENT,
    PR_A40_DELAY_EXTERNAL,
    PR_A40_DELAY_BUDGET,
    PR_A40_DELAY_CALLBACK
} pr_a40_delay_status;

/* Reads correspond to BIOS 0040:0000 and DS:84FC. A nonzero callback result
 * stops at that reached read and is retained in the report. max_operations
 * counts successful reads of either kind; zero permits no external read. */
typedef struct {
    void *ctx;
    int (*read_bios_byte)(void *ctx, uint8_t *value);
    int (*read_calibration_word)(void *ctx, uint16_t *value);
    uint64_t max_operations;
} pr_a40_delay_env;

typedef struct {
    uint16_t requested_units;
    uint16_t completed_units;
    uint16_t last_calibration;
    uint16_t cx_after;
    uint8_t last_baseline;
    uint8_t last_observed;
    uint64_t bios_reads;
    uint64_t calibration_reads;
    uint64_t spin_calls;
    uint64_t bx_decrements;
    int callback_status;
} pr_a40_delay_report;

/* Bounded state semantics for the complete resident A40:029C..02C9 pair:
 * far wait body +029C..+02BD and its near spin helper +02BD..+02C9.
 * No wall-clock duration is represented. A missing callback or exhausted
 * budget stops at the first reached external read, preserving prior counts.
 * Zero units return without consulting the environment. */
pr_a40_delay_status pr_a40_delay_029c_run(
    uint16_t units, const pr_a40_delay_env *env, pr_a40_delay_report *report);

/* Optional adapter for the existing U1C DELAY timing hook. Its nonzero
 * return is carried as PR_U1C_DELAY_CALLBACK with callback_status equal to
 * a pr_a40_delay_status. The detailed partial state stays in adapter.report. */
typedef struct {
    pr_a40_delay_env env;
    pr_a40_delay_report report;
    pr_a40_delay_status status;
} pr_a40_delay_u1c_adapter;

int pr_a40_delay_029c_u1c_hook(void *ctx, pr_u1c_delay_env *unused,
                                 uint16_t units);

#endif
