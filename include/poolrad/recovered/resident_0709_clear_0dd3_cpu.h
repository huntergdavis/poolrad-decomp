#ifndef POOLRAD_RECOVERED_RESIDENT_0709_CLEAR_0DD3_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0709_CLEAR_0DD3_CPU_H
#include "poolrad/recovered/resident_0709_rect_fill_00b7_cpu.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_r0709_clear_cpu;
typedef enum {
    PR_R0709_CLEAR_CPU_OK=0, PR_R0709_CLEAR_CPU_ARGUMENT,
    PR_R0709_CLEAR_CPU_CODE, PR_R0709_CLEAR_CPU_CALLEE,
    PR_R0709_CLEAR_CPU_TRANSFER=5, PR_R0709_CLEAR_CPU_LIMIT
} pr_r0709_clear_cpu_status;
typedef struct { int callee,status;pr_r0709_rect_cpu_diagnostic rect; } pr_r0709_clear_cpu_diagnostic;
/* Raw 0709:[0DD3,0F82), RETF8. Four words: bottom,right,top,left.
 * DS:4A1E selects mode0/1/2 once. Mode0/2 sign-extend coordinate bytes and
 * clear packed-memory rows through bus FillChar. Mode1 invokes raw +00B7
 * twice, color0 and pages0/1. Other modes return without drawing.
 * Exactly1MiB A20off, attached EGA, IP0DD3, SP[003E,FFF4]. DS/SS segment
 * windows exclude A0000..AFFFF. Code/bindings immutable outside EGA; writes
 * avoid code, reached return frames do not wrap; other aliases remain live.
 * CPU/bus/EGA/diag distinct from RAM and each other. Callbacks only observe
 * supplied operands or refuse OUT; no state mutation. Normal/386 flags.
 * Direct far FillChar calls require offset16A8 at0E41/0E63/0F18/0F35/0F52/0F6F and
 * CALL0E8E/0EA8 require E8 and displacementsF226/F20C, targeting same-CS
 * rectangle00B7 after PUSH CS. Far FillChar live segments select bodies. CODE
 * retains the pre-call state. CALLEE preserves a dependency stop and raw
 * status; TRANSFER retains an unexpected successful dependency return.
 * callee0=FillChar,1=first rectangle,2=second rectangle,-1=none. rect retains
 * the latest rectangle diagnostic. max_loopbacks counts taken JNE0E6E/0F7A;
 * rect_loopbacks is fresh per rectangle. LIMIT stops before the exhausted
 * outer transfer; nested LIMIT/PORT/CODE propagate as CALLEE. ARGUMENT
 * preserves CPU/RAM/EGA and resets optional diagnostic. No arbitrary-PC
 * resume, interrupts or faults. All +00B7 hardware limits apply: modes0/2
 * ordinary-memory effects do not establish CGA/Tandy hardware closure. */
pr_r0709_clear_cpu_status pr_r0709_clear_0dd3_cpu(pr_bus *bus,size_t ram_size,
    pr_r0709_clear_cpu *cpu,size_t max_loopbacks,size_t rect_loopbacks,
    pr_r0709_clear_cpu_diagnostic *diagnostic);
#endif
