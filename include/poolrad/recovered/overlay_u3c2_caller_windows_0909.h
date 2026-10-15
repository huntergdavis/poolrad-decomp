#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_CALLER_WINDOWS_0909_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_CALLER_WINDOWS_0909_H

#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"

typedef enum {
    PR_U3C2_CALLER_WINDOWS_OK = 0,
    PR_U3C2_CALLER_WINDOWS_ARGUMENT
} pr_u3c2_caller_windows_status;

/* Synthetic entry at +0A71 or +0B73 inside the +0909 caller.
 * Execute one 24-instruction argument-setup/CALL window and stop at +057B,
 * before any callee instruction. No entry-to-window reachability claim.
 * Effective offsets/SP wrap at 16 bits; word stores use contiguous physical
 * bytes with A20 off. Caller code must remain immutable. XOR's undefined AF
 * is set to zero by convention, matching the pinned Unicorn 2.1.4 oracle;
 * this is not an assertion about native AF. All other flags are retained or
 * updated according to the executed instructions. No asynchronous traps.
 */
pr_u3c2_caller_windows_status pr_u3c2_caller_windows_0909(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu);

#endif
