#ifndef POOLRAD_RECOVERED_OVERLAY_U3B9_BITMAP_SET_RELEASE_048F_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B9_BITMAP_SET_RELEASE_048F_CPU_H
#include "poolrad/recovered/resident_0464_bitmap_release_2595_cpu.h"
typedef pr_r0464_2595_cpu pr_u3b9_048f_cpu;
typedef enum {
    PR_U3B9_048F_RETURNED=0, PR_U3B9_048F_ARGUMENT,
    PR_U3B9_048F_CALLEE, PR_U3B9_048F_TRANSFER, PR_U3B9_048F_LIMIT
} pr_u3b9_048f_status;
typedef struct {
    int resident_status; /* -1 until a resident call */
    pr_r0464_2595_diagnostic resident; /* latest resident invocation */
    unsigned release_calls, fill_calls;
} pr_u3b9_048f_diagnostic;
/* GAME.OVR U3B9 [048F,04F8), far Pascal bitmap-set pointer, RETF4.
 * Exactly 1MiB A20-off ordinary RAM, IP048F, SP[0080,FFF8]. Immutable code,
 * valid nonwrapping reached dependency frames, no interrupts/faults. Both
 * far-call bindings are checked before mutation. Raw resident_cs:2595 and
 * SYSTEM system_cs:16A8 compose directly, without host callbacks. The
 * resident's three SYSTEM bindings must satisfy its published contract.
 * CPU and diagnostic are distinct host objects outside RAM. Live aliases
 * and the signed byte count (including 9..127) retain original semantics.
 * max_release_calls stops at 04CA before CALL, after its argument pushes;
 * max_heap_steps resets for each FreeMem call. Nonargument stops preserve
 * exact partial CPU14/RAM. Unexpected dependency return => TRANSFER.
 * ARGUMENT leaves all objects unchanged; null diagnostic is allowed.
 * FLAGS follow accepted DOSBox-X normal/386 models, without output masks.
 */
pr_u3b9_048f_status pr_u3b9_bitmap_set_release_048f_cpu(uint8_t *ram,
    size_t ram_size,pr_u3b9_048f_cpu *cpu,uint16_t resident_cs,
    uint16_t system_cs,unsigned max_release_calls,unsigned max_heap_steps,
    pr_u3b9_048f_diagnostic *diagnostic);
#endif
