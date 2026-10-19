#ifndef POOLRAD_RECOVERED_OVERLAY_U7D_ENTRY_05F0_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7D_ENTRY_05F0_CPU_H
#include "poolrad/recovered/resident_0709_printer_0626_cpu.h"
#include "poolrad/recovered/overlay_u7d_two_digits_0565_cpu.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u7d_05f0_cpu;
typedef struct { size_t caller_loopbacks,printer_loopbacks,glyph_loopbacks; } pr_u7d_05f0_limits;
typedef struct { int callee,status;pr_u7d_0565_callee format_callee;int format_status;pr_r0709_printer_cpu_diagnostic printer; } pr_u7d_05f0_diagnostic;
typedef enum { PR_U7D_05F0_LOAD=0,PR_U7D_05F0_FORMAT,PR_U7D_05F0_PRINTER } pr_u7d_05f0_callee;
typedef enum { PR_U7D_05F0_OK=0,PR_U7D_05F0_ARGUMENT,PR_U7D_05F0_CODE,
    PR_U7D_05F0_CALLEE,PR_U7D_05F0_TRANSFER,PR_U7D_05F0_LIMIT } pr_u7d_05f0_status;
/* Complete U7D [05F0,06D8), RETF2; later queue bytes are data. Argument
 * SS:[entry SP+4] low byte is sign-extended for the raw highlight word store
 * at BP+2*index-0F; it is NOT clamped to the seven-color array. Values outside
 * 0..6 may alter other stack bytes, including the final return frame.
 * Print CS:05E3 title, fields DS:6DCA/6DC8 low bytes, separators CS:05EE,
 * then low byte of (10*DS:6DC6+DS:6DC4), through raw +0565 and +0626.
 * CS:0563 holds the formatter's prefix; its numeric table is inherited.
 * Exactly1MiB A20off RAM, EGA attached, IP05F0, SP[0500,FFF6]. Initial DS/SS
 * windows and all caller/flat-RAM callee operands exclude A0000..AFFFF.
 * Drawing uses pr_bus. Code/bindings immutable outside aperture; writes avoid
 * code, reached frames nonwrapping; other data aliases remain live. CPU/bus/
 * EGA/limits/diagnostic/RAM storage distinct. Callbacks observe/refuse OUT
 * only. Caller budget counts taken JNE0611/06A7, shared across both loops;
 * LIMIT stops before transfer. Printer/glyph budgets refresh per invocation.
 * Root near/far opcode/offset checks precede CALL; prior PUSH CS remains at
 * near CODE. Far segments read live. CALLEE preserves nested status, TRANSFER
 * unexpected dependency returns. No rollback; root RETF uses the live frame.
 * ARGUMENT preserves CPU/RAM/EGA and resets optional diagnostic. Normal/386;
 * no interrupts/faults/self-modification/arbitrary-PC resume. Modes0/2
 * ordinary-memory effects do not establish CGA/Tandy hardware closure. */
pr_u7d_05f0_status pr_u7d_entry_05f0_cpu(pr_bus *bus,size_t ram_size,
    pr_u7d_05f0_cpu *cpu,const pr_u7d_05f0_limits *limits,pr_u7d_05f0_diagnostic *diagnostic);
#endif
