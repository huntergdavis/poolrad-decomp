#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_STEP_071A_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_STEP_071A_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
typedef pr_u6c_3f31_cpu pr_u3c6_071a_cpu;
typedef enum {
    PR_U3C6_071A_OK = 0,
    PR_U3C6_071A_ARGUMENT,
    PR_U3C6_071A_LIMIT,
    PR_U3C6_071A_TRANSFER
} pr_u3c6_071a_status;
/* Complete [071A,07D4), including +0C5F, +0000 and +06E9 on live
 * 1 MiB A20-off RAM. Enter after CALL, IP=071A. SS:SP+4 is a byte mode,
 * SS:SP+6/+8 the exact offset/segment pair searched in the slot table.
 * RETF 6 consumes the real frame. Word/LES bytes are physically consecutive
 * after 16-bit EA; RETF pops IP and CS separately with wrapped SS offsets.
 * The +0000 contiguous LES profile is NOT native-verified at FFFD..FFFF.
 * All pointers/table/local bytes remain live. No callbacks or result oracle.
 * budget counts +0C5F loop heads and caller +0753 loop heads. LIMIT keeps
 * prior effects and points to the unexecuted loop head; it is not resumable.
 * TRANSFER keeps an unexpected callee return. ARGUMENT is atomic, including
 * a conservative 31-byte stack/code overlap screen. CPU must be outside RAM.
 * Immutable code for all four extents is required. See U3C6_STEP_071A.md. */
pr_u3c6_071a_status pr_u3c6_step_071a(uint8_t *ram, size_t ram_size,
                                    pr_u3c6_071a_cpu *cpu, size_t budget);
#endif
