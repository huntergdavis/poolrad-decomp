#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_DISPOSE_11F3_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_DISPOSE_11F3_CPU_H
#include "poolrad/recovered/system_freemem_0254_cpu.h"
typedef pr_system_freemem_cpu pr_u3b0_11f3_cpu;
typedef enum {
    PR_U3B0_11F3_RETURNED=0, PR_U3B0_11F3_ARGUMENT, PR_U3B0_11F3_CODE,
    PR_U3B0_11F3_CALLEE, PR_U3B0_11F3_TRANSFER, PR_U3B0_11F3_LIMIT
} pr_u3b0_11f3_status;
/* Complete [11F3,1245), RETF4. Entry SS:SP contains return IP/CS and the
 * offset/segment of a cell containing the first node's far pointer. The cell
 * is not explicitly cleared. Each node's +2A/+2C link is saved before FreeMem(node,2E).
 * Exact 1MiB A20-off RAM; CPU/diagnostic storage distinct from RAM and each other; IP11F3, SP[0034,FFF8].
 * Loaded caller/SYSTEM code is immutable, with the live 1238 far-call segment
 * binding the original SYSTEM body. Opcode/offset are checked before CALL;
 * all other code bytes are preconditions. All reached return frames must
 * remain nonwrapping, including alias-induced changes. Words/LES/returns
 * are physically contiguous; individual effective offsets wrap at16 bits.
 * Live heap/global/stack aliases are preserved. No IRQs/faults/MMIO/SMC.
 * Logical AF follows DOSBox-X normal/386 zero. max_loopbacks caps taken
 * 123D jumps, stopping at1209 after the jump; zero allows the first free.
 * max_heap_steps is FreeMem's per-call walk budget. CALLEE retains its
 * exact nonzero status in heap_status (optional), CPU and RAM, including
 * RunError204 at SYSTEM:00E2. A changed successful far return is TRANSFER.
 * ARGUMENT changes no CPU/RAM; no other stop rolls back prior effects.
 */
pr_u3b0_11f3_status pr_u3b0_dispose_11f3_cpu(uint8_t *ram,size_t ram_size,
    pr_u3b0_11f3_cpu *cpu,unsigned max_loopbacks,unsigned max_heap_steps,
    int *heap_status);
#endif
