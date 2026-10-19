#ifndef POOLRAD_RECOVERED_RESIDENT_0709_TEXT_06A7_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0709_TEXT_06A7_CPU_H
#include "poolrad/recovered/memory_bus.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_r0709_text_cpu;
typedef enum {
    PR_R0709_TEXT_CPU_OK=0, PR_R0709_TEXT_CPU_ARGUMENT,
    PR_R0709_TEXT_CPU_CODE, PR_R0709_TEXT_CPU_CALLEE,
    PR_R0709_TEXT_CPU_TRANSFER=5, PR_R0709_TEXT_CPU_LIMIT
} pr_r0709_text_cpu_status;
typedef struct { int callee,status,glyph_callee,glyph_status; } pr_r0709_text_cpu_diagnostic;
/* Raw 0709:[06A7,0714), RETF2. One word: parent-frame offset in SS.
 * Live byte index SS:[parent-0103] runs while <= SS:[parent-0104]. Each
 * iteration draws SS:[parent-0100+index], transform1,count1, color at
 * parent+000C and x/y at DS:5E24/5E25. Optional DS:4A00 gate invokes raw
 * CRT Delay with DS:49E2 times10; then index and x increment as bytes.
 * Index255 wraps, so termination needs a budget for some live states.
 * Exactly1MiB A20off, attached EGA, IP06A7, SP[002C,FFFA]; DS/SS whole
 * segment windows exclude EGA. Code/bindings immutable outside EGA, writes
 * avoid code, reached frames do not wrap; other aliases remain live.
 * CPU/bus/EGA/diagnostic distinct from RAM and each other. Callbacks only
 * observe/refuse OUT without mutation. Normal/386 flags; MUL undefined
 * SZP follow the product, AF cleared, matching the accepted delay wrapper.
 * CALL06E8 requires E8/FAF4 (near01DF, preceding PUSH CS); LCALL06FD requires
 * 9A/029C with live segment. CODE retains pre-call state; CALLEE preserves
 * child stop, TRANSFER unexpected return. callee0=glyph,1=delay,-1=none;
 * glyph diagnostics retain the last invocation. Outer LIMIT stops before
 * JMP070E; glyph/delay budgets are fresh per call. Delay LIMIT retains its
 * target-IP convention. ARGUMENT preserves execution state, resets diag.
 * No arbitrary-PC resume, interrupts or faults. Packed modes0/2 ordinary
 * RAM effects are not CGA/Tandy hardware closure; native hardware is EGA. */
pr_r0709_text_cpu_status pr_r0709_text_06a7_cpu(pr_bus *bus,size_t ram_size,
    pr_r0709_text_cpu *cpu,size_t max_loopbacks,size_t glyph_loopbacks,
    uint64_t delay_loopbacks,pr_r0709_text_cpu_diagnostic *diagnostic);
#endif
