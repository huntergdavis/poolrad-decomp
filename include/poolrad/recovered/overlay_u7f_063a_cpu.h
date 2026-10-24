#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_063A_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_063A_CPU_H
#include "poolrad/recovered/overlay_u7f_0054_cpu.h"

typedef pr_res00ba_cpu pr_u7f_063a_cpu;
typedef enum {
    PR_U7F_063A_RETURNED = 0,
    PR_U7F_063A_ARGUMENT,
    PR_U7F_063A_CODE,
    PR_U7F_063A_LIMIT,
    PR_U7F_063A_TRANSFER,
    PR_U7F_063A_CALLEE
} pr_u7f_063a_status;
typedef struct {
    int callee; /* -1 none; 0 party count, 1 FillChar, 2 LongDiv,
                 * 3 cap test (+0054), 4 add (+003A), 5 threshold (+0000). */
    int callee_status;
    uint16_t site;
    pr_u7f_0054_diagnostic d0054;
    pr_u7f_0000_diagnostic d0000;
} pr_u7f_063a_diagnostic;

/* Unit 007F entry 3, [063A,0A01), 967 bytes / 385 instructions, RETF.
 * Distribute the seven DS:67F4 pool dwords over party-node +88h words,
 * using the class threshold and node +102h; store residual pools and 6DC0.
 * Exactly 1 MiB A20-off RAM, IP 063A, entry SP in [0200,FFFC].
 * Party-node accesses in this body use the bus; code/DS/SS use flat RAM.
 * Callees retain their own bus/flat contracts, including the aperture
 * exclusion of +0000's resident closure. Code is immutable. CPU, bus,
 * budget and diagnostic storage are distinct from RAM and each other.
 * Each caller, +0054, +0000 and resident-closure instruction consumes one
 * budget unit. +003A, +05E0, FillChar and LongDiv do not consume that budget.
 * In particular +05E0 requires a terminating list traversal, including
 * aliases with the stack it writes; its merged API has no loop budget.
 * ARGUMENT changes nothing. CODE stops before a malformed call pushes its
 * return frame. LIMIT leaves the next unexecuted counted instruction.
 * TRANSFER exposes a changed return target. CALLEE preserves the callee's
 * stopped CPU/RAM and reports its status (including LongDiv divide error).
 */
pr_u7f_063a_status pr_u7f_063a_run(pr_bus *bus, size_t ram_size,
    pr_u7f_063a_cpu *cpu, size_t *budget, pr_u7f_063a_diagnostic *diagnostic);
#endif
