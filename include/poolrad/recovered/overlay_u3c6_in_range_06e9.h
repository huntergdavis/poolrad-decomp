#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_IN_RANGE_06E9_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_IN_RANGE_06E9_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C6_06E9_RAM_SIZE PR_U6C_3F31_RAM_SIZE
typedef pr_u6c_3f31_cpu pr_u3c6_06e9_cpu;
typedef enum { PR_U3C6_06E9_OK = 0, PR_U3C6_06E9_ARGUMENT } pr_u3c6_06e9_status;
/* Complete entry8 +06E9..0719. After CALL, low bytes of words at
 * SS:SP+4 (a), SS:SP+6 (b) are signed; AL=1 iff both are within 0..6.
 * AH preserved; exact FLAGS from last CMP, live frame/RETF4 cleanup.
 * Offsets/SP wrap16; contiguous operands/A20-off RAM wrap20. Immutable
 * code required. Natural game invocation and loader dispatch unmeasured. */
pr_u3c6_06e9_status pr_u3c6_in_range_06e9(
    uint8_t *ram, size_t ram_size, pr_u3c6_06e9_cpu *cpu);
#endif
