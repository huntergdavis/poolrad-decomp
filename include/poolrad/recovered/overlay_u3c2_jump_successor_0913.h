#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_JUMP_SUCCESSOR_0913_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_JUMP_SUCCESSOR_0913_H

#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"

typedef enum {
    PR_U3C2_0913_JUMP_OK = 0,
    PR_U3C2_0913_JUMP_ARGUMENT
} pr_u3c2_0913_jump_status;

/* One near JMP at +0913, stopping before +0918's first instruction.
 * Only IP changes; no RAM access or stack effect. Code must be immutable
 * and debug exceptions/asynchronous traps are excluded. Null/other entry IP
 * is rejected without mutation. No full-caller or natural/native claim.
 */
pr_u3c2_0913_jump_status pr_u3c2_jump_successor_0913(pr_u6c_3f31_cpu *cpu);

#endif
