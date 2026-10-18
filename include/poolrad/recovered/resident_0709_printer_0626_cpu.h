#ifndef POOLRAD_RECOVERED_RESIDENT_0709_PRINTER_0626_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0709_PRINTER_0626_CPU_H
#include "poolrad/recovered/memory_bus.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_r0709_printer_cpu;
typedef enum {
    PR_R0709_PRINTER_CPU_OK=0, PR_R0709_PRINTER_CPU_ARGUMENT,
    PR_R0709_PRINTER_CPU_CODE, PR_R0709_PRINTER_CPU_CALLEE,
    PR_R0709_PRINTER_CPU_TRANSFER=5, PR_R0709_PRINTER_CPU_LIMIT
} pr_r0709_printer_cpu_status;
typedef struct { int callee,status,glyph_callee,glyph_status; } pr_r0709_printer_cpu_diagnostic;
/* Raw 0709:[0626,06A7), RETF10. Five argument words: string offset,
 * string segment, color, y, x. Raw StrStore copies into the live SS buffer;
 * each byte then uses raw +01DF (transform=1,count=1). Initial IP0626,
 * SP[0130,FFF2], exactly1MiB A20off, attached EGA. Initial DS/SS and source
 * string segment windows exclude A0000..AFFFF (StrStore uses ordinary RAM).
 * Code/bindings immutable outside EGA, writes avoid code, reached return
 * frames do not wrap; other data aliases remain live. CPU/bus/EGA/diagnostic
 * are distinct from RAM and each other. All +01DF hardware restrictions apply.
 * +063C must contain LCALL offset0B25, its live segment binds StrStore;
 * +0691 must be near CALL01DF. CODE retains the pre-call state.
 * max_loopbacks counts taken outer JNE069F; glyph_loopbacks is a fresh
 * +01DF budget per call. LIMIT retains the exhausted outer JNE; glyph stops
 * (including refused OUTs) propagate as CALLEE with nested diagnostics.
 * callee0=StrStore,1=glyph,-1=none. TRANSFER retains unexpected callee returns.
 * ARGUMENT preserves CPU/RAM/EGA and resets optional diagnostic. Callbacks
 * only observe supplied operands or refuse OUT; they cannot mutate state.
 * Normal/386 flags. No arbitrary-PC resume, interrupts, faults or other
 * video-hardware interpretation of modes0/2's ordinary-memory writes. */
pr_r0709_printer_cpu_status pr_r0709_printer_0626_cpu(pr_bus *bus,size_t ram_size,
    pr_r0709_printer_cpu *cpu,size_t max_loopbacks,size_t glyph_loopbacks,
    pr_r0709_printer_cpu_diagnostic *diagnostic);
#endif
