#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_SIDECAR_0160_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_SIDECAR_0160_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U9E_0160_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u9e_0160_cpu;
typedef struct { uint16_t slots_cs,chain_cs,system_cs,resident_cs; } pr_u9e_0160_layout;
typedef struct {
    size_t party_visits,result_visits,slot_visits,chain_iterations,line_iterations;
    unsigned lookup_loopbacks,sort_loopbacks;
} pr_u9e_0160_limits;
typedef struct {
    int callee,status; /* latest direct callee: -1 none,0 lookup,1 set,2 Move,3 slot,4 scan */
    uint16_t handler_offset,handler_segment; /* live target when PENDING */
} pr_u9e_0160_diagnostic;
typedef enum { PR_U9E_0160_OK=0,PR_U9E_0160_ARGUMENT,PR_U9E_0160_CODE,
    PR_U9E_0160_CALLEE,PR_U9E_0160_TRANSFER,PR_U9E_0160_PENDING,
    PR_U9E_0160_LIMIT } pr_u9e_0160_status;
/* Partial chain: U9E [0160,02FA), RETF6: type word, member off/seg.
 * Composes raw RES2199, SYSTEM0DC0/1685, U3C6 slots and U3C2 0909.
 * PENDING stops BEFORE U9E001E's indirect call, after the exact dispatcher
 * prefix has pushed five handler words. No handler effects are supplied.
 * A direct lookup hit can dispatch any byte; the four-member set only gates
 * the fallback party scan. No strict full-chain or gameplay claim.
 * Exact1MiB A20off; IP0160, SP[0186,FFF6]. CPU/layout/limits/diagnostic
 * distinct from RAM and each other; diagnostic optional. Original code and
 * bindings immutable, code writes excluded, reached return frames nonwrapping.
 * Direct far CALLs must be resolved to actual body offsets and layout segments;
 * reached bindings are checked before CALL. The scan's nested bindings retain
 * its own contract. Data/stack aliases and operand rereads remain live.
 * party_visits counts 01C3 visits; result_visits counts 026E visits, shared
 * across the call. Both may be zero. Each lookup/getter gets a fresh budget.
 * Chain/line/sort budgets must be positive and retain 0909's meanings.
 * LIMIT preserves the state before the exhausted caller visit; CALLEE keeps
 * dependency stops with diagnostic. TRANSFER keeps changed direct returns.
 * ARGUMENT leaves RAM/CPU unchanged; diagnostic is reset on entry.
 * Flags use normal/386 logical AF0, one-bit SHL AF1; IRQs/faults excluded.
 * See docs/re/U9E_SIDECAR_0160_CPU.md for evidence and explicit limits. */
pr_u9e_0160_status pr_u9e_sidecar_0160_cpu(uint8_t *ram,size_t size,
    pr_u9e_0160_cpu *cpu,const pr_u9e_0160_layout *layout,
    const pr_u9e_0160_limits *limits,pr_u9e_0160_diagnostic *diagnostic);
#endif
