#ifndef POOLRAD_RECOVERED_SYSTEM_FREEMEM_0254_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_FREEMEM_0254_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_SYSTEM_FREEMEM_RAM_SIZE (1u<<20)
typedef struct {
    uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags;
} pr_system_freemem_cpu;
typedef enum {
    PR_SYSTEM_FREEMEM_RETURNED=0, PR_SYSTEM_FREEMEM_ARGUMENT,
    PR_SYSTEM_FREEMEM_RUNERROR, PR_SYSTEM_FREEMEM_TRANSFER, PR_SYSTEM_FREEMEM_LIMIT
} pr_system_freemem_status;
/* Complete SYSTEM FreeMem: entry [0254,026E), core/merge [03D3,0491),
 * rounding [0491,04A2). After far call: return IP/CS, size, pointer offset/
 * segment at entry SS:SP+[0,2,4,6,8]. RETF6 on success; invalid pointer or
 * duplicate returns RunError at current CS:00E2, AX204, caller frame intact.
 * A zero size is a no-op without pointer validation. A nonzero block must
 * have offset0/8 and lie in [HeapOrg,HeapPtr); no stronger ownership check
 * is invented. The original writes rounded size before duplicate checking.
 * Exact1MiB A20off RAM; IP0254, conservative SP[0020,FFF6]; immutable code;
 * CPU distinct from RAM. All reached return frames nonwrapping, including
 * alias-induced changes. Reads/writes remain live across stack/heap/global
 * aliases. Word/LES/return reads are physically contiguous. IRQs, faults and
 * self-modifying code excluded. Logical AF follows normal/386 zero.
 * max_steps caps taken walk edges back to040F; LIMIT stops there before
 * the next iteration, preserving partial state. TRANSFER retains changed
 * near-return state. ARGUMENT changes nothing; other stops never roll back.
 */
pr_system_freemem_status pr_system_freemem_0254(uint8_t *ram,size_t ram_size,
    pr_system_freemem_cpu *cpu,unsigned max_steps);
#endif
