#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_LINE_INIT_01A0_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_LINE_INIT_01A0_H
#include "poolrad/recovered/overlay_u3c2_sign_entry1_0000.h"

typedef pr_u3c2_0000_cpu pr_u3c2_01a0_cpu;
typedef enum {
    PR_U3C2_01A0_OK = 0,
    PR_U3C2_01A0_ARGUMENT,
    PR_U3C2_01A0_CODE_ALIAS,
    PR_U3C2_01A0_CALLEE,
    PR_U3C2_01A0_TRANSFER
} pr_u3c2_01a0_status;

/* Complete U3C2 public entry 2 [01A0,02CC), composed with sign entry +0000.
 * Enter after far CALL at IP=01A0, SP in [0010,FFF8]; the far record pointer
 * is at SS:SP+4. Exactly 1 MiB RAM, distinct stable CPU/RAM objects and
 * immutable code. Record writes must not overlap modeled code. Reserved
 * stack/code alias and invalid argument rejection is atomic.
 * Every original LES reload remains live, including record/stack aliases.
 * Word/LES operands are physically contiguous with A20 wrapping; effective
 * offsets wrap at 16 bits. The accepted initial stack range avoids wrapped
 * far frames in the sign helper's separately wrapped RETF profile.
 * RETF 4 exposes the actual outer return frame. Non-OK dependency status
 * exposes its reached CPU/RAM; TRANSFER detects a changed nested return.
 * Logical/SHL undefined AF is chosen zero. Native reachability and strict
 * credit remain separate verification gates. No callback or host limit.
 */
pr_u3c2_01a0_status pr_u3c2_line_init_01a0(
    uint8_t *ram, size_t ram_size, pr_u3c2_01a0_cpu *cpu);
#endif
