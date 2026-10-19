#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_CLEAR_17E3_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_CLEAR_17E3_CPU_H
#include "poolrad/recovered/resident_03f1_clear_0137_cpu.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_r00ba_clear_cpu;
typedef enum {
    PR_R00BA_CLEAR_CPU_OK=0, PR_R00BA_CLEAR_CPU_ARGUMENT,
    PR_R00BA_CLEAR_CPU_CODE, PR_R00BA_CLEAR_CPU_CALLEE,
    PR_R00BA_CLEAR_CPU_TRANSFER=5
} pr_r00ba_clear_cpu_status;
typedef struct { int callee,status;pr_r03f1_clear_cpu_diagnostic clear; } pr_r00ba_clear_cpu_diagnostic;
/* Raw 00BA:[17E3,1813), RETF, no arguments. Reads DS:49F3 after PUSH BP:
 * value5 selects bottom21,right38,top10,left23; other values select
 * bottom22,right38,top18,left1. AX high byte remains live in pushed words.
 * Composes raw 03F1 +0137 with caller-supplied clear/rectangle budgets.
 * Exactly1MiB A20off, attached EGA, IP17E3, SP[004C,FFFC]. DS/SS whole
 * segment windows exclude A0000..AFFFF. Code/bindings immutable outside
 * EGA, writes avoid code, reached frames do not wrap; other aliases live.
 * CPU/bus/EGA/diagnostic distinct from RAM and each other. Callbacks only
 * observe/refuse OUT without state mutation. Normal/386 flags; no interrupts,
 * faults or arbitrary-PC resume. Reached far bindings17F9/180C require
 * opcode9A and offset0137; live segment selects the loaded clear body.
 * CODE stops before CALL; CALLEE retains nested status/state; TRANSFER
 * retains unexpected successful child return. callee0=17F9,1=180C,-1=none.
 * clear retains nested diagnostics. ARGUMENT preserves CPU/RAM/EGA and
 * resets diagnostics. All +0137 hardware limits apply: native EGA only;
 * packed modes0/2 model ordinary RAM, not CGA/Tandy hardware closure. */
pr_r00ba_clear_cpu_status pr_r00ba_clear_17e3_cpu(pr_bus *bus,size_t ram_size,
    pr_r00ba_clear_cpu *cpu,size_t clear_loopbacks,size_t rect_loopbacks,
    pr_r00ba_clear_cpu_diagnostic *diagnostic);
#endif
