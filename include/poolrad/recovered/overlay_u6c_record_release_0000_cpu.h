#ifndef POOLRAD_RECOVERED_OVERLAY_U6C_RECORD_RELEASE_0000_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U6C_RECORD_RELEASE_0000_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U6C_0000_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags;
} pr_u6c_0000_cpu;
typedef struct { unsigned max_list_loopbacks, max_heap_loopbacks; } pr_u6c_0000_limits;
typedef struct {
    int freemem_status; /* -1 until called; otherwise pr_system_freemem_status */
    uint16_t call_site; /* last reached LCALL; zero before any */
} pr_u6c_0000_diagnostic;
typedef enum {
    PR_U6C_0000_OK=0, PR_U6C_0000_ARGUMENT, PR_U6C_0000_CODE,
    PR_U6C_0000_CALLEE, PR_U6C_0000_TRANSFER, PR_U6C_0000_LIMIT
} pr_u6c_0000_status;
/* Complete U6C [0000,00E2), RETF 4. Entry stack: return IP/CS, record
 * offset/segment. Free optional record+108 (22 bytes), +C8 chain (63-byte
 * nodes, next at +2A), +7F chain (9-byte nodes, next at +5), then record
 * (285 bytes). Links are read before freeing. The final free rereads the
 * original argument, then clears its stack copy, not an external pointer.
 * Exact 1 MiB A20-off RAM; IP=0, SP in [0040,FFF8]. CPU, limits and
 * diagnostic are distinct from RAM and each other. Code/bindings immutable;
 * all reached return frames nonwrapping. No IRQs, faults or self-modification.
 * Words/LES use physically contiguous bytes; data/stack aliases stay live.
 * Reached far CALL opcodes/offsets are checked, segments read live; each
 * bound segment must contain the same SYSTEM FreeMem body and near closure.
 * Logical AF follows DOSBox-X normal/386 zero. Shared list budget counts
 * taken +007B->0047 and +00C3->008F edges; zero permits one node before LIMIT
 * at the next loop head. Heap budget is per FreeMem invocation. CALLEE
 * preserves its RunError204, LIMIT, TRANSFER or ARGUMENT state, with status
 * in diagnostic. No rollback. ARGUMENT leaves CPU/RAM unchanged.
 */
pr_u6c_0000_status pr_u6c_record_release_0000_cpu(uint8_t *ram,size_t size,
    pr_u6c_0000_cpu *cpu,const pr_u6c_0000_limits *limits,
    pr_u6c_0000_diagnostic *diagnostic);
#endif
