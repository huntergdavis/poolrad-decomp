#ifndef POOLRAD_RECOVERED_OVERLAY_U38_SIDECAR_1472_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_SIDECAR_1472_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U38_1472_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u38_1472_cpu;
typedef struct { int append_status,getmem_status; } pr_u38_1472_diagnostic;
typedef enum { PR_U38_1472_OK=0,PR_U38_1472_ARGUMENT,PR_U38_1472_CODE,PR_U38_1472_CALLEE,PR_U38_1472_TRANSFER } pr_u38_1472_status;
/* Complete U38 [1472,1490), public entry49, RETF0Ah. Five word arguments;
 * record P is entry SS:[SP+0Ah]/[SP+0Ch]. Append (b=62h,a=0,c=FFh,d=0)
 * through live stub:0052 -> raw U9E +0E52/GetMem. Key push preserves entry
 * AH; XOR AX,AX then sets flags before the remaining argument pushes.
 * Exact1MiB A20-off RAM; IP1472, conservative SP[0040,FFF2]; CPU/diagnostic
 * distinct from RAM. Loaded code/bindings immutable. Every reached return
 * frame must remain nonwrapping, including alias-induced changes. Data,
 * heap and stack aliases stay live. Logical AF follows normal/386 zero.
 * Dependencies retain their profiles; heap/link budgets retain dependency
 * semantics and may0. CODE stops before malformed reached LCALL/EA stub;
 * CALLEE preserves raw append status and nested GetMem status/state;
 * TRANSFER preserves a changed direct return. ARGUMENT changes no CPU/RAM
 * and initializes diagnostic to -1/-1. Null diagnostic allowed. No rollback.
 * IRQs, faults and self-modifying code are outside the profile.
 */
pr_u38_1472_status pr_u38_sidecar_1472_cpu(uint8_t *ram,size_t size,pr_u38_1472_cpu *cpu,
    unsigned max_heap_steps,size_t max_links,pr_u38_1472_diagnostic *diagnostic);
#endif
