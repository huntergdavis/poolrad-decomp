#ifndef POOLRAD_RECOVERED_OVERLAY_U38_TARGET_MASK81_25BC_H
#define POOLRAD_RECOVERED_OVERLAY_U38_TARGET_MASK81_25BC_H

#include "poolrad/recovered/overlay_u38_target_select_101d.h"

typedef enum {
    PR_U38_25BC_OK = 0,
    PR_U38_25BC_ARGUMENT,
    PR_U38_25BC_CALL_ABORT
} pr_u38_25bc_status;

/* Reachable 73-byte prefix of U38 public entry 78; the 11-byte tail is data.
 * select_target handles the resident far call made by +101D. */
pr_u38_25bc_status pr_u38_target_mask81_25bc(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context);


/* Raw CPU composition API; legacy callback API above is retained. */

typedef enum {
    PR_TARGET_SHIFT_UNICORN = 0, /* SHR AF=0 */
    PR_TARGET_SHIFT_DOSBOX386 = 1 /* DOSBox-X normal/386 SHR AF=1 */
} pr_target_shift_profile;
/* Complete actual U38 [25BC,2605), 73 bytes, RETF0Ah. The following eleven
 * bytes are data. Compose +101D/resident on live shared CPU/RAM, then halve
 * DS:6818 iff the selected object's DS table byte has mask81 set.
 * Same profile as +101D; entry IP25BC, SP002E..FFFC; ten unused argument
 * bytes. Live near-CALL opcode/target guard; exact partial stop states.
 * SHR AF is architecturally undefined: choose a measured oracle profile
 * explicitly. No flag masking. Invalid arguments reject atomically. */
pr_target_status pr_u38_target_mask81_25bc_cpu(uint8_t *ram,size_t size,
    pr_target_cpu *cpu,pr_target_shift_profile shift_profile,
    pr_target_diagnostic *diagnostic);
#endif
