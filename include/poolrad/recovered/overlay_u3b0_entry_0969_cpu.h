#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY_0969_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY_0969_CPU_H
#include "poolrad/recovered/resident_0709_printer_0626_cpu.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u3b0_0969_cpu;
typedef struct { size_t printer_loopbacks,glyph_loopbacks; } pr_u3b0_0969_limits;
typedef struct { int callee,status,scan_store_status;pr_r0709_printer_cpu_diagnostic printer; } pr_u3b0_0969_diagnostic;
typedef enum { PR_U3B0_0969_SELECT=0,PR_U3B0_0969_LEAD,PR_U3B0_0969_TRAIL,
    PR_U3B0_0969_COPY,PR_U3B0_0969_PRINTER } pr_u3b0_0969_callee;
typedef enum { PR_U3B0_0969_OK=0,PR_U3B0_0969_ARGUMENT,PR_U3B0_0969_CODE,
    PR_U3B0_0969_CALLEE,PR_U3B0_0969_TRANSFER } pr_u3b0_0969_status;
/* Complete U3B0 [0969,0A08), RETF4. SS:[SP+4] context offset within SS,
 * [SP+6] list index. Select through context+14/+16, compute leading/trailing
 * space bounds through +08D7/+0923, copy the selected slice into a local
 * Pascal string and print it at the live context's x/y/color coordinates.
 * Exact1MiB A20off RAM, attached EGA, IP0969, SP[0300,FFF4]; initial DS/SS
 * windows and all caller/flat-RAM selector, scan and SYSTEM accesses exclude
 * A0000..AFFFF. Printer/glyph hardware accesses use the bus. Code/bindings
 * immutable outside aperture; writes avoid code; reached frames nonwrapping.
 * Other aliases remain live; CPU/bus/EGA/limits/diag distinct from RAM and
 * each other. Callbacks only observe or refuse OUT, without state mutation.
 * Every printer/glyph invocation gets fresh budgets. No invented cap is
 * added to the selector or scans; their inherited contracts apply.
 * Reached near/far call opcodes/offsets checked before CALL (PUSH CS has
 * already executed for near calls); far segments are read live. CALLEE
 * preserves the actual dependency stop; TRANSFER preserves altered returns.
 * No rollback. ARGUMENT changes only optional diagnostic. Normal/386 flags;
 * no interrupts/faults/self-modification/arbitrary-PC resume. Modes0/2
 * ordinary-memory effects do not establish CGA/Tandy hardware closure. */
pr_u3b0_0969_status pr_u3b0_entry_0969_cpu(pr_bus *bus,size_t ram_size,
    pr_u3b0_0969_cpu *cpu,const pr_u3b0_0969_limits *limits,pr_u3b0_0969_diagnostic *diagnostic);
#endif
