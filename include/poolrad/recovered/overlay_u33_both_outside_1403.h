#ifndef POOLRAD_RECOVERED_OVERLAY_U33_BOTH_OUTSIDE_1403_H
#define POOLRAD_RECOVERED_OVERLAY_U33_BOTH_OUTSIDE_1403_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U33_1403_RAM_SIZE PR_U6C_3F31_RAM_SIZE
typedef pr_u6c_3f31_cpu pr_u33_1403_cpu;
typedef enum { PR_U33_1403_OK = 0, PR_U33_1403_ARGUMENT } pr_u33_1403_status;
/* Complete +1403..1433. After CALL, low bytes of words SS:SP+4/+6 are
 * signed a/b. AL=1 exactly when a is outside 0..5 AND b outside 0..10;
 * AH is preserved. Final CMP FLAGS, local byte, saved BP and RETF4 use
 * raw RAM. Offsets/SP wrap16, contiguous operands/A20-off RAM wrap20.
 * Code must stay immutable. No loader/natural-game reach is claimed. */
pr_u33_1403_status pr_u33_both_outside_1403(
    uint8_t *ram, size_t ram_size, pr_u33_1403_cpu *cpu);
#endif
