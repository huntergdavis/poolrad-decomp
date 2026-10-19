#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY_07A4_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY_07A4_CPU_H
#include "poolrad/recovered/resident_03f1_clear_0137_cpu.h"
#include "poolrad/recovered/resident_0709_printer_0626_cpu.h"
#include "poolrad/recovered/resident_0709_glyph_01df_cpu.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u3b0_07a4_cpu;
typedef struct { size_t row_loopbacks,clear_loopbacks,rect_loopbacks,printer_loopbacks,glyph_loopbacks; } pr_u3b0_07a4_limits;
typedef struct {
    int callee,status;
    pr_r03f1_clear_cpu_diagnostic clear;
    pr_r0709_printer_cpu_diagnostic printer;
    pr_r0709_glyph_cpu_diagnostic glyph;
} pr_u3b0_07a4_diagnostic;
typedef enum { PR_U3B0_07A4_CLEAR=0,PR_U3B0_07A4_SELECT,PR_U3B0_07A4_FLAGGED,
    PR_U3B0_07A4_PRINTER,PR_U3B0_07A4_GLYPH } pr_u3b0_07a4_callee;
typedef enum { PR_U3B0_07A4_OK=0,PR_U3B0_07A4_ARGUMENT,PR_U3B0_07A4_CODE,
    PR_U3B0_07A4_CALLEE,PR_U3B0_07A4_TRANSFER,PR_U3B0_07A4_LIMIT } pr_u3b0_07a4_status;
/* Complete U3B0 [07A4,08D7), RETF4. SS:[entry SP+4] context offset in SS,
 * [SP+6] start index. Clear context rectangle +18/+1A/+1C/+1E, select from
 * head +14/+16 and call +0711 (its returned pointer remains in stack locals).
 * Draw successive nodes through signed-byte bottom +18, using color +22 or
 * +24 for node+29 zero/nonzero; pad to byte context[-54] using black spaces.
 * Row byte wraps. row_loopbacks counts unconditional JMP08CE; LIMIT stops
 * before the exhausted jump, after link/row updates. Other budgets refresh
 * per dependency invocation. CALLEE retains nested stops/diagnostics;
 * TRANSFER retains unexpected returns. No rollback. Reached root near/far
 * opcode/offset checked before CALL; prior PUSH CS remains on near CODE.
 * Far segment bindings are live; inherited callee validation applies.
 * Exact1MiB A20off RAM, attached EGA, IP07A4, SP[0300,FFF4]. Initial DS/SS
 * windows and caller/flat-RAM selector, flagged traversal and SYSTEM accesses
 * exclude A0000..AFFFF; drawing dependencies use pr_bus. Code/bindings
 * immutable outside aperture, writes avoid code, reached frames nonwrapping.
 * Other data aliases remain live. CPU/bus/EGA/limits/diag/RAM storage distinct.
 * Callbacks only observe or refuse OUT, without state mutation. ARGUMENT
 * preserves CPU/RAM/EGA and resets optional diagnostic. Normal/386 flags;
 * no interrupts/faults/self-modification/arbitrary-PC resume. Modes0/2
 * ordinary-memory effects do not establish CGA/Tandy hardware closure. */
pr_u3b0_07a4_status pr_u3b0_entry_07a4_cpu(pr_bus *bus,size_t ram_size,
    pr_u3b0_07a4_cpu *cpu,const pr_u3b0_07a4_limits *limits,pr_u3b0_07a4_diagnostic *diagnostic);
#endif
