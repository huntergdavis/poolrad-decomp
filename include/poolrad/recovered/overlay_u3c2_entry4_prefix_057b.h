#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_ENTRY4_PREFIX_057B_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_ENTRY4_PREFIX_057B_H

#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"

#define PR_U3C2_057B_PREFIX_RAM_SIZE PR_U6C_3F31_RAM_SIZE
#define PR_U3C2_057B_PREFIX_ENTRY 0x057Bu
#define PR_U3C2_057B_PREFIX_STOP 0x0585u

typedef pr_u6c_3f31_cpu pr_u3c2_057b_prefix_cpu;
typedef enum {
    PR_U3C2_057B_PREFIX_OK = 0,
    PR_U3C2_057B_PREFIX_ARGUMENT
} pr_u3c2_057b_prefix_status;

/* Bounded entry4 prefix +057B..+0584 only. Stops BEFORE +0585 JL;
 * no branch, body continuation or far return is executed.
 * Enter after CALL with IP=057B and immutable code. Saves BP, establishes
 * the frame, allocates four untouched local bytes and compares the byte
 * SS:[BP+0E] with zero. All 14 CPU words and defined flags are retained.
 * Offsets/SP wrap at 16 bits; the saved word uses contiguous A20-wrapped
 * physical bytes. No natural native frame or full-function credit claim. */
pr_u3c2_057b_prefix_status pr_u3c2_entry4_prefix_057b(
    uint8_t *ram, size_t ram_size, pr_u3c2_057b_prefix_cpu *cpu);

#endif
