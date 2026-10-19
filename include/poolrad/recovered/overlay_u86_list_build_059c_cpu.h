#ifndef POOLRAD_RECOVERED_OVERLAY_U86_LIST_BUILD_059C_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U86_LIST_BUILD_059C_CPU_H
#include "poolrad/recovered/overlay_u86_sidecar_gate_051c_cpu.h"
#include "poolrad/recovered/overlay_u86_string_node_028d.h"
#define PR_U86_059C_RAM_SIZE PR_U86_051C_RAM_SIZE
typedef pr_u86_051c_cpu pr_u86_059c_cpu;
typedef struct {
    size_t max_history_visits,max_item_visits;
    pr_u86_051c_limits gate;
} pr_u86_059c_limits;
typedef struct {
    int callee,status; /* 0 append,1 sidecar,2 gate,-1 none. */
    pr_u86_028d_diagnostic append;
    pr_u86_044e_diagnostic sidecar;
    pr_u86_051c_diagnostic gate;
} pr_u86_059c_diagnostic;
typedef enum { PR_U86_059C_OK=0,PR_U86_059C_ARGUMENT,PR_U86_059C_CODE,
    PR_U86_059C_CALLEE,PR_U86_059C_TRANSFER,PR_U86_059C_LOOP_LIMIT } pr_u86_059c_status;
/* Complete U86 [059C,0729), RETF2. Low argument byte selects modes0..6;
 * clear list DS:4766/4768 and 21 history bytes, compose raw +028D/+044E/
 * +051C, return AL=1 iff the live final list pointer is nonzero.
 * All operands, byte counters, signed category indexing and aliases live.
 * Exact1MiB A20off, IP059C, conservative SP[016A,FFFA]. Code/prefix/bindings
 * immutable; writes avoid code; every reached return frame nonwrapping.
 * CPU/limits/diagnostic distinct from RAM and each other. Reached callee
 * contracts apply. Normal/386 logical AF=0; no interrupts or faults.
 * Positive history/item and gate clear/record/sidecar caller/list budgets;
 * heap/lookup budgets may be zero. History visits at05B7, item visits at
 * 05D9/0627/0663/06BB; each dependency call gets fresh child budgets.
 * CODE before bad reached near CALL, after PUSH CS. CALLEE retains exact
 * stopped CPU/RAM and complete dependency diagnostics. TRANSFER preserves
 * changed direct returns. LOOP_LIMIT before the exhausted visit. No rollback.
 * Diagnostics reset at each valid direct call; unused nested fields=-1.
 * ARGUMENT preserves CPU/RAM and initializes diagnostic. NULL diag allowed.
 */
pr_u86_059c_status pr_u86_list_build_059c_cpu(uint8_t *ram,size_t size,
    pr_u86_059c_cpu *cpu,const pr_u86_059c_limits *limits,pr_u86_059c_diagnostic *diagnostic);
#endif
