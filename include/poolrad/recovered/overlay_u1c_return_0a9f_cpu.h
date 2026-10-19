#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_RETURN_0A9F_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_RETURN_0A9F_CPU_H
#include "poolrad/recovered/system_freemem_0254_cpu.h"
typedef pr_system_freemem_cpu pr_u1c_return_cpu;
typedef enum {
    PR_U1C_RETURN_CPU_RETURNED=0, PR_U1C_RETURN_CPU_ARGUMENT,
    PR_U1C_RETURN_CPU_CODE, PR_U1C_RETURN_CPU_CALLEE, PR_U1C_RETURN_CPU_TRANSFER
} pr_u1c_return_cpu_status;
/* Complete U1C [0A9F,0AED), RETF with no arguments. Increment DS49ED; if
 * DS6F70:6F72 is nonnil, restore the node's saved PC, snapshot its +2/+4
 * next pointer to DS6F74:6F76, FreeMem(head,6), then reload those live
 * temporaries into the head. Heap writes/aliases are never substituted.
 * Exact 1MiB A20-off flat RAM; IP0A9F, initial SP[002C,FFFC]. CPU and
 * diagnostic storage distinct from RAM and each other. Loaded code is
 * immutable; live CALL0AD8 opcode/offset checked before the call, its
 * segment binds SYSTEM. Other instruction bytes are identity preconditions.
 * All reached return frames remain nonwrapping, including after aliases.
 * Effective offsets wrap at16 bits; words/LES/returns are physically
 * contiguous with20-bit wrapping. No MMIO/IRQ/fault/SMC. Logical AF follows
 * DOSBox-X normal/386 zero. max_heap_steps caps FreeMem's taken walk edges.
 * CALLEE preserves a nonzero FreeMem status in optional heap_status and
 * all partial CPU/RAM (RunError204 at SYSTEM:00E2, walk LIMIT, near TRANSFER).
 * A changed successful callee far return is TRANSFER. CODE stops before
 * CALL after prior effects. ARGUMENT changes no CPU/RAM; no rollback.
 */
pr_u1c_return_cpu_status pr_u1c_return_0a9f_cpu(uint8_t *ram,size_t ram_size,
    pr_u1c_return_cpu *cpu,unsigned max_heap_steps,int *heap_status);
#endif
