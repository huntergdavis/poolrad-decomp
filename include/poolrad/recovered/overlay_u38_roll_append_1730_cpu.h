#ifndef POOLRAD_RECOVERED_OVERLAY_U38_ROLL_APPEND_1730_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_ROLL_APPEND_1730_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U38_1730_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u38_1730_cpu;
typedef struct { unsigned max_system_calls,max_heap_steps;size_t max_links; } pr_u38_1730_limits;
typedef enum { PR_U38_1730_NO_CALLEE=-1,PR_U38_1730_ROLL=0,PR_U38_1730_APPEND } pr_u38_1730_callee;
typedef struct { pr_u38_1730_callee callee;int status,getmem_status; } pr_u38_1730_diagnostic;
typedef enum { PR_U38_1730_OK=0,PR_U38_1730_ARGUMENT,PR_U38_1730_CODE,PR_U38_1730_CALLEE,PR_U38_1730_TRANSFER } pr_u38_1730_status;
/* Complete executable U38 [1730,175E), RETF0Ah, public entry60. Five word
 * arguments; the far record P is entry SS:[SP+0Ah]/[SP+0Ch]. Roll1d100
 * using raw U9E +0DE3; when AL<=95 append node (b=19h,a=1,c=0Ch,d=0)
 * using raw U9E +0E52/GetMem. All arguments and effects use live RAM.
 * Exact1MiB A20-off RAM, IP1730, conservative SP[0040,FFF2], immutable
 * loaded code/bindings; CPU/limits/diagnostic storage distinct from RAM.
 * U9E dice and SYSTEM code retain their original pinned layout, SYSTEM
 * fixed at0AF8. Reached dependency return frames must remain nonwrapping,
 * including after alias changes. Data/heap/stack aliases remain live.
 * Logical AF follows measured normal/386 zero; dependency profiles apply.
 * Budgets retain dependency meanings and may0. CODE precedes a malformed
 * reached LCALL/EA stub; CALLEE retains exact dependency stop/status and
 * APPEND's nested GetMem status; TRANSFER preserves a changed direct return.
 * ARGUMENT changes no CPU/RAM; diagnostic starts NO_CALLEE/-1. Null diagnostic
 * allowed. No rollback. IRQs/faults/self-modifying code outside the profile.
 */
pr_u38_1730_status pr_u38_roll_append_1730_cpu(uint8_t *ram,size_t size,pr_u38_1730_cpu *cpu,
    const pr_u38_1730_limits *limits,pr_u38_1730_diagnostic *diagnostic);
#endif
