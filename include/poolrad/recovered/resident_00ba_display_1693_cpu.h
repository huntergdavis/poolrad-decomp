#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_DISPLAY_1693_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_DISPLAY_1693_CPU_H
#include "poolrad/recovered/resident_03f1_clear_0137_cpu.h"
#include "poolrad/recovered/resident_0709_printer_0626_cpu.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_r00ba_1693_cpu;
typedef enum {
    PR_R00BA_1693_OK=0, PR_R00BA_1693_ARGUMENT,
    PR_R00BA_1693_CODE, PR_R00BA_1693_CALLEE,
    PR_R00BA_1693_TRANSFER=5
} pr_r00ba_1693_status;
typedef struct {
    int callee,status,delay_status;
    pr_r03f1_clear_cpu_diagnostic clear;
    pr_r0709_printer_cpu_diagnostic printer;
} pr_r00ba_1693_diagnostic;
/* Raw 00BA:[1693,16EC), RETF4. Entry words: string offset, segment.
 * StrStore caps the live source at40 bytes into SS:BP-41; clear row24,
 * print at column0,row24,color10; delay; clear row24 again. No callbacks
 * replace callees. Exactly1MiB A20off, attached EGA, IP1693, SP[016A,FFF8].
 * Initial CS/DS/SS/source segment windows exclude A0000..AFFFF, because
 * StrStore and Delay use ordinary RAM. Code and bindings are immutable,
 * writes avoid code, reached return frames do not wrap; all other aliases
 * remain live. CPU/bus/EGA/diagnostic objects are distinct from RAM/each
 * other. Callbacks observe operands or refuse OUT without mutating state.
 * Calls at16A7/16B8/16CB/16D0/16E1 require offsets0B25/0137/0626/0DBE/0137;
 * live segment operands bind loaded dependencies. CODE stops before CALL.
 * CALLEE retains the dependency stop; TRANSFER retains unexpected successful
 * callee returns. callee=0..4 in call order, -1 before any call; status is
 * raw. clear/printer retain the latest nested diagnostics, delay_status the
 * CRT status. Each clear/rectangle/printer/glyph gets a fresh named budget;
 * delay_loopbacks is passed to the single delay. ARGUMENT preserves CPU,
 * RAM and EGA, resetting only optional diagnostic. Normal/386 flag profile.
 * No arbitrary-PC resume, interrupts, faults, timing guarantee or CGA/Tandy
 * hardware closure. All underlying graphics bus/hardware limits apply. */
pr_r00ba_1693_status pr_r00ba_display_1693_cpu(pr_bus*,size_t,pr_r00ba_1693_cpu*,
    size_t clear_loopbacks,size_t rect_loopbacks,size_t printer_loopbacks,
    size_t glyph_loopbacks,uint64_t delay_loopbacks,pr_r00ba_1693_diagnostic*);
#endif
