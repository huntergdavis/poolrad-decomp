#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_CALLER_0909_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_CALLER_0909_H
#include <stddef.h>
#include <stdint.h>
#define PR_U3C2_0909_RAM_SIZE (1u<<20)
typedef struct {
    uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags;
} pr_u3c2_0909_cpu;
typedef enum {
    PR_U3C2_0909_OK=0, PR_U3C2_0909_ARGUMENT,
    PR_U3C2_0909_CALLER_LIMIT, PR_U3C2_0909_LINE_LIMIT,
    PR_U3C2_0909_SORT_LIMIT, PR_U3C2_0909_CALLEE, PR_U3C2_0909_TRANSFER
} pr_u3c2_0909_status;
/* Full U3C2 entry6 [0909,0BD9), entered after far CALL with seven word
 * arguments; RETF14. Exact 1MiB A20-off RAM, distinct CPU object, immutable
 * loaded code/bindings, synchronous access, writes must avoid modeled code.
 * Initial IP0909 and SP[008E,FFEE]. pair_cs binds U3C6 +0000; the three
 * SYSTEM Move call bindings in the loaded +002E code are read live.
 * Positive caller budget counts visits to 0918,0993,09BA,0A48,0A61,0B73,
 * shared across loops. Each +0422 call receives a fresh line budget and
 * +002E receives the sort loop-back budget. Stops expose exact partial state.
 * CALLEE preserves a dependency's rejected/partial/changed-return state.
 * TRANSFER preserves unexpected direct-callee returns. No invented return.
 * See docs/re/U3C2_CALLER_0909.md for detailed ABI and evidence scope. */
pr_u3c2_0909_status pr_u3c2_caller_0909(
    uint8_t *ram,size_t size,pr_u3c2_0909_cpu *cpu,uint16_t pair_cs,
    size_t max_caller_iterations,size_t max_line_iterations,unsigned max_sort_loopbacks);
#endif
