#ifndef POOLRAD_RECOVERED_OVERLAY_U38_ROLL_EFFECT_27D9_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_ROLL_EFFECT_27D9_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U38_27D9_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u38_27d9_cpu;
typedef struct { unsigned max_system_calls,max_heap_steps;size_t max_links; } pr_u38_27d9_limits;
typedef enum { PR_U38_27D9_NO_CALLEE=-1,PR_U38_27D9_ROLL=0,PR_U38_27D9_APPEND } pr_u38_27d9_callee;
typedef struct { pr_u38_27d9_callee callee;int status,getmem_status; } pr_u38_27d9_diagnostic;
typedef enum { PR_U38_27D9_OK=0,PR_U38_27D9_ARGUMENT,PR_U38_27D9_CODE,PR_U38_27D9_CALLEE,PR_U38_27D9_TRANSFER } pr_u38_27d9_status;
/* Complete U38 [27D9,2814), RETF 0Ah, public entry 84. Five word
 * arguments; far record P is entry SS:[SP+0Ah]/[SP+0Ch]. If DS:6819
 * bit0 or bit4 is set, return after testing it. Otherwise roll 3d6 via
 * raw U9E +0DE3/SYSTEM and append (b=66h,a=roll,c=FFh,d=1) via raw
 * U9E +0E52/GetMem. Reads/writes and retained high argument bytes stay live.
 * Exactly 1MiB A20-off RAM, IP27D9, SP[0040,FFF2]; immutable loaded
 * code/bindings, CPU/limits/diagnostic objects distinct from RAM. SYSTEM
 * is fixed at 0AF8 and dice code retains its pinned layout. Reached
 * dependency return frames must remain nonwrapping even after alias changes.
 * Data/heap/stack aliases are live; logical AF follows measured normal/386
 * zero. Dependency contracts apply. Budgets may be zero and retain the
 * dependency meanings. CODE stops before a malformed reached LCALL/EA
 * stub. CALLEE retains dependency state/status and nested GetMem status;
 * TRANSFER retains a changed direct return. ARGUMENT changes no CPU/RAM.
 * Diagnostic starts NO_CALLEE/-1; null allowed. No rollback. Interrupts,
 * faults and self-modifying code are outside this synchronous profile.
 */
pr_u38_27d9_status pr_u38_roll_effect_27d9_cpu(uint8_t *ram,size_t size,pr_u38_27d9_cpu *cpu,
    const pr_u38_27d9_limits *limits,pr_u38_27d9_diagnostic *diagnostic);
#endif
