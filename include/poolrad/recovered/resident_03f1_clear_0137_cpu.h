#ifndef POOLRAD_RECOVERED_RESIDENT_03F1_CLEAR_0137_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_03F1_CLEAR_0137_CPU_H
#include "poolrad/recovered/resident_0709_rect_fill_00b7_cpu.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_r03f1_clear_cpu;
typedef enum {
    PR_R03F1_CLEAR_CPU_OK=0, PR_R03F1_CLEAR_CPU_ARGUMENT,
    PR_R03F1_CLEAR_CPU_CODE, PR_R03F1_CLEAR_CPU_CALLEE,
    PR_R03F1_CLEAR_CPU_TRANSFER=5, PR_R03F1_CLEAR_CPU_LIMIT
} pr_r03f1_clear_cpu_status;
typedef struct { int callee,status;pr_r0709_rect_cpu_diagnostic rect; } pr_r03f1_clear_cpu_diagnostic;
/* Raw 03F1:[0137,02E8), RETF8. Four words: bottom,right,top,left.
 * DS:4A1E selects mode0/1/2 once. Mode0/2 sign-extend coordinate bytes and
 * clear packed-memory rows through bus FillChar. Mode1 invokes raw +00B7
 * twice, color0 and pages0/1. Other modes return without drawing.
 * Exactly1MiB A20off, attached EGA, IP0137, SP[003E,FFF4]. DS/SS segment
 * windows exclude A0000..AFFFF. Code/bindings immutable outside EGA; writes
 * avoid code, reached return frames do not wrap; other aliases remain live.
 * CPU/bus/EGA/diag distinct from RAM and each other. Callbacks only observe
 * supplied operands or refuse OUT; no state mutation. Normal/386 flags.
 * Direct far calls require offset16A8 at01A5/01C7/027E/029B/02B8/02D5 and
 * offset00B7 at01F1/020C; live segments select loaded dependencies. CODE
 * retains the pre-call state. CALLEE preserves a dependency stop and raw
 * status; TRANSFER retains an unexpected successful dependency return.
 * callee0=FillChar,1=first rectangle,2=second rectangle,-1=none. rect retains
 * the latest rectangle diagnostic. max_loopbacks counts taken JNE01D2/02E0;
 * rect_loopbacks is fresh per rectangle. LIMIT stops before the exhausted
 * outer transfer; nested LIMIT/PORT/CODE propagate as CALLEE. ARGUMENT
 * preserves CPU/RAM/EGA and resets optional diagnostic. No arbitrary-PC
 * resume, interrupts or faults. All +00B7 hardware limits apply: modes0/2
 * ordinary-memory effects do not establish CGA/Tandy hardware closure. */
pr_r03f1_clear_cpu_status pr_r03f1_clear_0137_cpu(pr_bus *bus,size_t ram_size,
    pr_r03f1_clear_cpu *cpu,size_t max_loopbacks,size_t rect_loopbacks,
    pr_r03f1_clear_cpu_diagnostic *diagnostic);
#endif
