#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_CLEARMONSTERS_125D_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_CLEARMONSTERS_125D_CPU_H
#include "poolrad/recovered/system_freemem_0254_cpu.h"
typedef pr_system_freemem_cpu pr_u1c_cmon_cpu;
typedef enum {
    PR_U1C_CMON_CPU_RETURNED=0, PR_U1C_CMON_CPU_ARGUMENT,
    PR_U1C_CMON_CPU_CODE, PR_U1C_CMON_CPU_CALLEE,
    PR_U1C_CMON_CPU_TRANSFER, PR_U1C_CMON_CPU_LIMIT
} pr_u1c_cmon_cpu_status;
typedef struct { int callee,status; } pr_u1c_cmon_cpu_diagnostic; /* 0 FillChar,1 FreeMem; -1 unreached. */
/* Complete [125D,12C6), no arguments, RETF. Increment DS49ED; reset three
 * globals; FillChar(DS67F4,1C,0); release DS6810:6812 nodes with FreeMem3F.
 * Node +2A/+2C is saved in live stack locals before each free, then reloaded
 * to the head after success. Exact1MiB A20-off flat RAM; IP125D; initial
 * SP[0032,FFFC]; CPU/diagnostic storage separate from RAM and each other.
 * All reached return frames nonwrapping, including alias-induced changes.
 * Effective offsets wrap at16 bits; word/LES/return operands are physically
 * contiguous with20-bit wrapping. Loaded code immutable. Live far-call
 * opcode/offset checked before CALL; each segment binds the original
 * SYSTEM body. Other code bytes are identity preconditions. No MMIO, IRQs,
 * faults or SMC. Logical AF follows measured DOSBox-X normal/386 zero.
 * max_loopbacks caps taken JMP12C0 edges: stop at1287 after the jump; zero
 * allows one successful free before the next nil test. max_heap_steps is
 * FreeMem's per-call walk budget. CALLEE preserves its nonzero status and
 * exact partial CPU/RAM; TRANSFER retains a changed successful far return.
 * CODE stops before its call after prior effects; ARGUMENT changes no
 * CPU/RAM. No other stop rolls back prior effects. Legacy API unchanged.
 */
pr_u1c_cmon_cpu_status pr_u1c_clearmonsters_125d_cpu(uint8_t *ram,size_t size,
    pr_u1c_cmon_cpu *cpu,unsigned max_loopbacks,unsigned max_heap_steps,
    pr_u1c_cmon_cpu_diagnostic *diagnostic);
#endif
