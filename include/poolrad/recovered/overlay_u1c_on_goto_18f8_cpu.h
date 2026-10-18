#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_ON_GOTO_18F8_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_ON_GOTO_18F8_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U1C_18F8_RAM_SIZE (1u << 20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u1c_18f8_cpu;
typedef struct {
    uint16_t system_cs;
    size_t max_operands,max_string_iterations,max_caller_iterations;
    unsigned max_heap_steps;
} pr_u1c_18f8_limits;
typedef enum {
    PR_U1C_18F8_NO_CALLEE=-1,PR_U1C_18F8_LOAD=0,PR_U1C_18F8_SLOT,
    PR_U1C_18F8_WORD,PR_U1C_18F8_ALLOC
} pr_u1c_18f8_callee;
typedef struct {
    pr_u1c_18f8_callee callee;
    int status,nested_callee,nested_status;
} pr_u1c_18f8_diagnostic;
typedef enum {
    PR_U1C_18F8_OK=0,PR_U1C_18F8_ARGUMENT,PR_U1C_18F8_CODE,
    PR_U1C_18F8_CALLEE,PR_U1C_18F8_TRANSFER,PR_U1C_18F8_CALLER_LIMIT
} pr_u1c_18f8_status;
/* Complete U1C [18F8,198D), RETF, no stack arguments. Loads selector/count,
 * rewinds PC by one, loads count targets, then scans zero-based selector.
 * Opcode DS:6F7F==25h sets PC; every other opcode pushes the return PC onto
 * the raw GOSUB heap list before jumping. Counts are full bytes; the three
 * operand tables may overlap. All six calls follow live EA overlay stubs.
 * Compose raw U25 +0000/+0136/+06AF/+1951; GetMem has no allocation hook.
 * Exact 1MiB A20-off RAM; distinct CPU/limits/diagnostic storage; immutable
 * loaded code and bindings. Data/stack aliases are read in source order.
 * Entry IP18F8, SP[0238,FFFC]; all reached dependency return frames must
 * remain nonwrapping. Dependencies retain their own contracts, including
 * terminating party lists for +0136 (that callee has no list-walk budget).
 * Positive operand/string/caller budgets; heap budget may be zero. Operand
 * and string budgets reset per dependency invocation; caller budget counts
 * visits to +1931. CALLEE preserves the dependency's partial CPU/RAM and
 * diagnostic; ALLOC additionally exposes its nested callee/status. CODE
 * stops before a reached malformed LCALL/stub. TRANSFER preserves a changed
 * direct return. No rollback. Final RETF may return anywhere. Null diagnostic
 * is allowed. Logical AF follows the measured normal/386 zero convention.
 * See docs/re/U1C_ON_GOTO_18F8_CPU.md for evidence and profile limitations. */
pr_u1c_18f8_status pr_u1c_on_goto_18f8_cpu(uint8_t *ram,size_t size,
    pr_u1c_18f8_cpu *cpu,const pr_u1c_18f8_limits *limits,
    pr_u1c_18f8_diagnostic *diagnostic);
#endif
