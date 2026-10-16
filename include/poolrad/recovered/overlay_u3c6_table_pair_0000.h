#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_TABLE_PAIR_0000_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_TABLE_PAIR_0000_H

#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
typedef pr_u6c_3f31_cpu pr_u3c6_0000_cpu;

typedef enum {
    PR_U3C6_0000_RETF_UNMEASURED = 0,
    PR_U3C6_0000_RETF_WRAPPED_SS,
    PR_U3C6_0000_RETF_CONTIGUOUS_PHYSICAL
} pr_u3c6_0000_retf_policy;

typedef enum {
    PR_U3C6_0000_RETURNED = 0,
    PR_U3C6_0000_RETURN_PENDING,
    PR_U3C6_0000_ARGUMENT
} pr_u3c6_0000_status;

/* Complete call-free body [0000,0068), entry after CALL at IP0000.
 * Exactly 1 MiB RAM, A20 off, immutable original code; CPU is a distinct
 * host object outside RAM. Effective offsets wrap at 16 bits; bytes within
 * one word/far-pointer operand are physically consecutive (20-bit wrap).
 * Live aliases include both pointers, coordinates, table, local, saved BP
 * and return frame. No callback/dependency and no normalized Boolean result.
 * Final CMP defines all arithmetic flags; no undefined-flag profile needed.
 * RETF policies are explicit synthetic candidates, NOT native facts.
 * UNMEASURED stops at0065 before RETF12, retaining all preceding effects.
 * Invalid host arguments/entry/policy reject before mutation.
 */
pr_u3c6_0000_status pr_u3c6_table_pair_0000(
    uint8_t *ram, size_t ram_size, pr_u3c6_0000_cpu *cpu,
    pr_u3c6_0000_retf_policy retf_policy);
#endif
