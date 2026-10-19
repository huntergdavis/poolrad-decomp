#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY_0C33_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY_0C33_CPU_H
#include "poolrad/recovered/overlay_u3b0_entry_07a4_cpu.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u3b0_0c33_cpu;
typedef struct { unsigned step_loopbacks;pr_u3b0_07a4_limits display; } pr_u3b0_0c33_limits;
typedef struct { int callee,status,step_status;pr_u3b0_07a4_diagnostic display; } pr_u3b0_0c33_diagnostic;
typedef enum { PR_U3B0_0C33_STEP=0,PR_U3B0_0C33_DISPLAY } pr_u3b0_0c33_callee;
typedef enum { PR_U3B0_0C33_OK=0,PR_U3B0_0C33_ARGUMENT,PR_U3B0_0C33_CODE,
    PR_U3B0_0C33_CALLEE,PR_U3B0_0C33_TRANSFER } pr_u3b0_0c33_status;
/* Complete U3B0 [0C33,0CC5), RETF4. SS:[entry SP+4] is a context offset
 * in SS, [SP+6] a direction low byte. Save X^ - DS:6B29 (X at context+0A),
 * move DS:6B29 by context[-59] with signed clamping against context[-56]
 * minus context[-59] for nonzero direction, or zero for zero direction.
 * Restore X^ relative to the new base, call raw +0B03 step scan, then +07A4
 * display using the live DS:6B29. Word arithmetic wraps; operands are read
 * in source order, including aliased context/global/index and stack state.
 * Exactly1MiB A20off RAM, EGA attached, IP0C33, SP[0320,FFF4]. Initial DS/SS
 * windows and all caller/flat-RAM callee operands exclude A0000..AFFFF.
 * Drawing uses pr_bus. Immutable code/bindings outside aperture, writes avoid
 * code, reached frames nonwrapping; other aliases live. CPU/bus/EGA/limits/
 * diag/RAM storage distinct; callbacks only observe/refuse OUT. Normal/386;
 * synchronous, no IRQ/fault/self-modification/arbitrary-PC resume.
 * Root near opcode/offset checks precede CALL, after PUSH CS. Inherited
 * dependency checks apply; +0B03's selector sites have no opcode guards.
 * CALLEE preserves nested stops; step LIMIT stops after the taken jump at
 * its loop head, display budgets retain their existing before-jump contracts.
 * TRANSFER detects unexpected dependency returns, root RETF uses live frame.
 * Partial effects retained; ARGUMENT preserves CPU/RAM/EGA and resets diag.
 * Modes0/2 ordinary-memory effects do not establish CGA/Tandy closure. */
pr_u3b0_0c33_status pr_u3b0_entry_0c33_cpu(pr_bus *bus,size_t ram_size,
    pr_u3b0_0c33_cpu *cpu,const pr_u3b0_0c33_limits *limits,pr_u3b0_0c33_diagnostic *diagnostic);
#endif
