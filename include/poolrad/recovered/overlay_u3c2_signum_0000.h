#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_SIGNUM_0000_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_SIGNUM_0000_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_0000_RAM_SIZE PR_U6C_3F31_RAM_SIZE
typedef pr_u6c_3f31_cpu pr_u3c2_0000_cpu;
typedef enum { PR_U3C2_0000_OK = 0, PR_U3C2_0000_ARGUMENT } pr_u3c2_0000_status;
/* Complete entry1 +0000..002D. After CALL, signed word at SS:SP+4;
 * AX returns FFFF/0000/0001 for negative/zero/positive. Nonzero FLAGS
 * come from CMP input,0; zero XOR defines all arithmetic flags except
 * AF (undefined, chosen zero). Live local/saved BP/far return, RETF2.
 * Offsets/SP wrap16, contiguous operands/A20-off RAM wrap20. Code must
 * remain immutable; loader/natural-game invocation is unmeasured. */
pr_u3c2_0000_status pr_u3c2_signum_0000(
    uint8_t *ram, size_t ram_size, pr_u3c2_0000_cpu *cpu);
#endif
