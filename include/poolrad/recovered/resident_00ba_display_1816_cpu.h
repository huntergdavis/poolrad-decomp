#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_DISPLAY_1816_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_DISPLAY_1816_CPU_H
#include "poolrad/recovered/resident_0709_printer_0626_cpu.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_r00ba_display_cpu;
typedef enum {
    PR_R00BA_DISPLAY_CPU_OK=0, PR_R00BA_DISPLAY_CPU_ARGUMENT,
    PR_R00BA_DISPLAY_CPU_CODE, PR_R00BA_DISPLAY_CPU_CALLEE,
    PR_R00BA_DISPLAY_CPU_TRANSFER=5
} pr_r00ba_display_cpu_status;
typedef struct {
    int callee,status;
    pr_r0709_printer_cpu_diagnostic printer;
} pr_r00ba_display_cpu_diagnostic;
/* Raw 00BA:[1816,1892), RETF10. Five argument words: flag,row,column,
 * node offset,node segment. Low scalar bytes are used; node fields select
 * color12/14/11. Raw +0626 prints the node; flag!=0 then loads the live node
 * length, copies CS:1813 via raw SYSTEM +0B0B, and prints that local suffix.
 * Exactly1MiB A20off, attached EGA, initial IP1816, SP[0144,FFF2]. Initial
 * CS/DS/SS and node segment windows exclude A0000..AFFFF. CS:1813 holds a
 * length2 Pascal literal (payload supplied in RAM, never embedded here).
 * Code, bindings and that literal are immutable; writes avoid them, reached
 * return frames do not wrap. Other aliases remain live. CPU/bus/EGA/diag
 * are distinct from RAM and each other. +0626 hardware restrictions apply.
 * Far calls at1853/1887 require offset0626;1882 requires0B0B. Live segments
 * select loaded bodies. CODE retains the pre-call state; CALLEE propagates
 * dependency stops, TRANSFER retains unexpected successful callee returns.
 * callee0=first printer,1=StrLoad,2=suffix printer,-1=none; status is raw.
 * printer retains the latest printer diagnostic. Printer budget is fresh
 * per call; glyph budget is fresh per glyph. ARGUMENT preserves CPU/RAM/EGA
 * and resets optional diagnostic. Callbacks only observe supplied operands
 * or refuse OUT; no state mutation. Normal/386 flags; no arbitrary-PC
 * resume, interrupts, faults, or mode0/2 CGA/Tandy hardware closure. */
pr_r00ba_display_cpu_status pr_r00ba_display_1816_cpu(pr_bus *bus,size_t ram_size,
    pr_r00ba_display_cpu *cpu,size_t printer_loopbacks,size_t glyph_loopbacks,
    pr_r00ba_display_cpu_diagnostic *diagnostic);
#endif
