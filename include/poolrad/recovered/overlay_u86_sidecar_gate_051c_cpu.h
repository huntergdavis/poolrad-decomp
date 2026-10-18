#ifndef POOLRAD_RECOVERED_OVERLAY_U86_SIDECAR_GATE_051C_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U86_SIDECAR_GATE_051C_CPU_H
#include "poolrad/recovered/overlay_u86_sidecar_list_044e.h"
#define PR_U86_051C_RAM_SIZE PR_U86_044E_RAM_SIZE
typedef pr_u86_044e_cpu pr_u86_051c_cpu;
typedef struct {
    size_t max_clear_visits,max_record_visits;
    pr_u86_044e_limits sidecar;
} pr_u86_051c_limits;
typedef enum { PR_U86_051C_NO_CALLEE=-1,PR_U86_051C_PREDICATE=0,PR_U86_051C_SIDECAR } pr_u86_051c_callee;
typedef struct {
    pr_u86_051c_callee callee;
    int status;
    pr_u86_044e_diagnostic sidecar;
} pr_u86_051c_diagnostic;
typedef enum { PR_U86_051C_OK=0,PR_U86_051C_ARGUMENT,PR_U86_051C_CODE,
    PR_U86_051C_CALLEE,PR_U86_051C_TRANSFER,PR_U86_051C_LOOP_LIMIT } pr_u86_051c_status;
/* Complete U86 [051C,059C), RETF2, composing raw +323D and +044E.
 * Clears 49 far-pointer slots DS:477F..4842, resets byte4843, follows the
 * character's +C8 chain through record +2A links, applying +044E to records
 * whose +2E-indexed DS:5582 category is 11..13. The low argument byte selects
 * +044E's slot mode. All reads/writes and returned CPU state remain live.
 * Exact1MiB A20-off RAM, IP051C, SP[0160,FFF8], immutable code/prefix/bindings.
 * Reached callee contracts apply; return frames must remain nonwrapping.
 * CPU/limits/diagnostic distinct from RAM and each other. No IRQs or faults.
 * Positive clear/record and sidecar caller/list visit limits. Other limits
 * may be zero. Clear visits count +052B (49 ordinarily); record visits count
 * +0560 including terminal null. Each +044E call resets its own budgets.
 * CODE stops before a malformed reached near CALL (PUSH CS already happened).
 * CALLEE preserves raw partial state and complete sidecar diagnostics;
 * TRANSFER preserves altered direct return state. LOOP_LIMIT stops before
 * the exhausted visit. No rollback. ARGUMENT leaves RAM/CPU unchanged.
 * A null diagnostic is allowed; otherwise initialized NO_CALLEE/-1.
 */
pr_u86_051c_status pr_u86_sidecar_gate_051c_cpu(uint8_t *ram,size_t size,
    pr_u86_051c_cpu *cpu,const pr_u86_051c_limits *limits,pr_u86_051c_diagnostic *diagnostic);
#endif
