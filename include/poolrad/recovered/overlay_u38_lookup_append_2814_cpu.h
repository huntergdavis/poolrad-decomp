#ifndef POOLRAD_RECOVERED_OVERLAY_U38_LOOKUP_APPEND_2814_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_LOOKUP_APPEND_2814_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U38_2814_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u38_2814_cpu;
typedef struct { unsigned max_lookup_loopbacks,max_heap_steps;size_t max_links; } pr_u38_2814_limits;
typedef struct { int callee,status,getmem_status; } pr_u38_2814_diagnostic;
typedef enum { PR_U38_2814_OK=0,PR_U38_2814_ARGUMENT,PR_U38_2814_CODE,PR_U38_2814_CALLEE,PR_U38_2814_TRANSFER } pr_u38_2814_status;
/* Complete U38 entry85 [2814,2866), RETF0Ah. Five word arguments; record P
 * at entry SS:[SP+0Ah]/[SP+0Ch], other three unused. Raw resident2199 looks
 * for62h then3Bh, using live local cursor SS:[BP-4]; return on either hit.
 * On two misses, raw U9E0E52/GetMem appends (b=3Bh,a=3,c=FFh,d=1).
 * Exact1MiB A20off RAM; IP2814, conservative SP[0040,FFF2]. CPU, limits and
 * diagnostic distinct from RAM. Immutable code/bindings; all reached return
 * frames nonwrapping, including after aliases. Data/heap/stack aliases live.
 * Dependency profiles retained; budgets may0 and lookup budget resets per
 * call. Logical AF follows normal/386 zero. CODE stops before a malformed
 * reached call/stub; last call diagnostic retained. CALLEE preserves raw
 * partial state; TRANSFER preserves a changed direct return. Diagnostic
 * callee=-1 none,0 lookup,1 append; status/getmem_status initialize-1, nested
 * GetMem is-1 for lookup. ARGUMENT leaves CPU/RAM unchanged and initializes
 * diagnostic. Null diagnostic allowed; null limits rejected. No rollback.
 * IRQs, faults and self-modifying code excluded.
 */
pr_u38_2814_status pr_u38_lookup_append_2814_cpu(uint8_t *ram,size_t size,pr_u38_2814_cpu *cpu,
    const pr_u38_2814_limits *limits,pr_u38_2814_diagnostic *diagnostic);
#endif
