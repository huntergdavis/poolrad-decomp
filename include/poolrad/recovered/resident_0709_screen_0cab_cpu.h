#ifndef POOLRAD_RECOVERED_RESIDENT_0709_SCREEN_0CAB_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0709_SCREEN_0CAB_CPU_H
#include "poolrad/recovered/resident_0709_rect_fill_00b7_cpu.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_r0709_screen_cpu;
typedef enum {
    PR_R0709_SCREEN_CPU_OK=0, PR_R0709_SCREEN_CPU_ARGUMENT,
    PR_R0709_SCREEN_CPU_CODE, PR_R0709_SCREEN_CPU_CALLEE,
    PR_R0709_SCREEN_CPU_TRANSFER=5
} pr_r0709_screen_cpu_status;
typedef struct { int callee,status;pr_r0709_rect_cpu_diagnostic rect; } pr_r0709_screen_cpu_diagnostic;
/* Raw 0709:[0CAB,0D68), RETF, no arguments. DS:4A1E selects mode once.
 * Mode0 fills BA00 then B800; mode2 fills B800,BA00,BC00,BE00: 8000
 * zero bytes per bank. Mode1 calls raw +00B7 with rectangle0,0..39,24,
 * color0 on pages0 and1. Other modes return without drawing.
 * Exactly1MiB A20off, attached EGA, IP0CAB, SP[0032,FFFC]. DS/SS whole
 * segment windows exclude A0000..AFFFF. Code/bindings immutable outside
 * EGA; writes avoid code, reached frames do not wrap; other aliases live.
 * CPU/bus/EGA/diagnostic distinct from RAM and each other; callbacks only
 * observe/refuse OUT without mutation. Normal/386 flags.
 * Far FillChar sites0CC4/0CD8/0D25/0D39/0D4D/0D61 require9A/16A8,
 * with live segment bindings. Near0CF7/0D0D requireE8/F3BD orE8/F3A7,
 * targeting same-CS00B7 after PUSH CS. CODE retains pre-call state.
 * rect_loopbacks is fresh per rectangle; child LIMIT/PORT/CODE return
 * CALLEE with exact stopped state. TRANSFER retains an unexpected child
 * return. callee0=FillChar,1=first rectangle,2=second rectangle,-1=none;
 * rect retains the latest rectangle diagnostic. ARGUMENT preserves
 * CPU/RAM/EGA and resets optional diagnostic. No arbitrary-PC resumption,
 * interrupts or faults. Native hardware scope is EGA; packed modes0/2
 * ordinary RAM effects do not establish CGA/Tandy hardware closure. */
pr_r0709_screen_cpu_status pr_r0709_screen_0cab_cpu(pr_bus *bus,size_t ram_size,
    pr_r0709_screen_cpu *cpu,size_t rect_loopbacks,pr_r0709_screen_cpu_diagnostic *diagnostic);
#endif
