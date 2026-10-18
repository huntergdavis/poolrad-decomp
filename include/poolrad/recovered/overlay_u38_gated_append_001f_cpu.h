#ifndef POOLRAD_RECOVERED_OVERLAY_U38_GATED_APPEND_001F_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_GATED_APPEND_001F_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U38_001F_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u38_001f_cpu;
typedef struct { int append_status,getmem_status; } pr_u38_001f_diagnostic;
typedef enum { PR_U38_001F_OK=0,PR_U38_001F_ARGUMENT,PR_U38_001F_CODE,PR_U38_001F_CALLEE,PR_U38_001F_TRANSFER } pr_u38_001f_status;
/* Complete U38 [001F,0058), RETF0Ah. Five word arguments at entry SP+4:
 * a, c, b, record offset, record segment. If DS:[681F] is zero, store local
 * result1 then append (b,a,c,1) through live stub0052 / raw U9E +0E52/GetMem;
 * otherwise store result0. Return AL loaded from the live SS:[BP-1] local.
 * Argument byte loads preserve AH. Exact1MiB A20off RAM, IP001F, conservative
 * SP[0040,FFF2]; CPU/diagnostic distinct from RAM. Immutable code/bindings;
 * all reached dependency return frames nonwrapping, including after aliases.
 * Data/heap/stack aliases stay live. Dependency budgets may0. CODE stops at
 * malformed reached LCALL/stub; CALLEE retains raw status/state; TRANSFER
 * retains a changed direct return. ARGUMENT leaves CPU/RAM unchanged and
 * initializes diagnostic -1/-1; null diagnostic allowed. No rollback.
 * IRQs, faults and self-modifying code outside profile.
 */
pr_u38_001f_status pr_u38_gated_append_001f_cpu(uint8_t *ram,size_t size,pr_u38_001f_cpu *cpu,
    unsigned max_heap_steps,size_t max_links,pr_u38_001f_diagnostic *diagnostic);
#endif
