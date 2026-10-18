#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_GOSUB_0111_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_GOSUB_0111_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U1C_GOSUB_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u1c_gosub_cpu;
typedef struct {
    uint16_t system_cs;
    size_t max_operands,max_string_iterations;
    unsigned max_heap_steps;
} pr_u1c_gosub_limits;
typedef enum { PR_U1C_GOSUB_NO_CALLEE=-1,PR_U1C_GOSUB_LOAD=0,PR_U1C_GOSUB_ALLOC } pr_u1c_gosub_callee;
typedef struct { pr_u1c_gosub_callee callee;int status,nested_callee,nested_status; } pr_u1c_gosub_diagnostic;
typedef enum { PR_U1C_GOSUB_OK=0,PR_U1C_GOSUB_ARGUMENT,PR_U1C_GOSUB_CODE,PR_U1C_GOSUB_CALLEE,PR_U1C_GOSUB_TRANSFER } pr_u1c_gosub_status;
/* Complete U1C [0111,0126), RETF, no arguments. Save BP; load one operand
 * through live stub:002A -> raw U25 +0000; push a GOSUB frame using slot1
 * through stub:0075 -> raw U25 +1951; restore BP and return. GetMem creates
 * a six-byte node holding the PC after operand loading and the old head;
 * +06AF assembles the new PC from slot1. No host callbacks or heap arrays.
 * Exact1MiB A20-off RAM; initial IP0111, SP[0232,FFFC], immutable loaded
 * code/bindings. CPU/limits/diagnostic storage must be distinct from RAM.
 * All data/heap/stack aliases are live. Every reached dependency return
 * frame must remain nonwrapping, including after frame aliases. Effective
 * offsets wrap16, physical addresses wrap20. Dependencies keep their own
 * contracts; logical AF follows the measured normal/386 zero convention.
 * Positive operand/string budgets; GetMem's per-call walk-edge budget may
 * be zero. CODE stops before a malformed reached LCALL or EA stub. CALLEE
 * preserves the dependency status/state; ALLOC additionally exposes its
 * nested callee/status. TRANSFER preserves an altered direct return.
 * ARGUMENT changes no CPU/RAM; diagnostic initializes to NO_CALLEE/-1.
 * No rollback. Null diagnostic allowed. Interrupts, faults and writes
 * overlapping code are outside the profile. See U1C_GOSUB_0111_CPU.md.
 */
pr_u1c_gosub_status pr_u1c_gosub_0111_cpu(uint8_t *ram,size_t size,
    pr_u1c_gosub_cpu *cpu,const pr_u1c_gosub_limits *limits,
    pr_u1c_gosub_diagnostic *diagnostic);
#endif
