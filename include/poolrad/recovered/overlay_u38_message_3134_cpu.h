#ifndef POOLRAD_RECOVERED_OVERLAY_U38_MESSAGE_3134_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_MESSAGE_3134_CPU_H
#include "poolrad/recovered/resident_00ba_display_1693_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u38_3134_cpu;
typedef enum {
    PR_U38_3134_OK=0, PR_U38_3134_ARGUMENT,
    PR_U38_3134_CODE, PR_U38_3134_CALLEE, PR_U38_3134_TRANSFER
} pr_u38_3134_status;
typedef enum { PR_U38_3134_NONE=-1, PR_U38_3134_STRLOAD=0, PR_U38_3134_DISPLAY=1 } pr_u38_3134_callee;
typedef struct {
    size_t clear_loopbacks,rect_loopbacks,printer_loopbacks,glyph_loopbacks;
    uint64_t delay_loopbacks;
} pr_u38_3134_limits;
typedef struct {
    int callee,status;
    pr_r00ba_1693_diagnostic display;
} pr_u38_3134_diagnostic;
/* Complete U38 [3134,317A), RETF 0Ah; later bytes are another function.
 * Frame after the far return: [SP+4] gate byte (word, only the low byte is
 * tested), [SP+6]/[SP+8] far pointer A, [SP+10]/[SP+12] far pointer B.
 * Nonzero gate, or byte ES:[B+10h] >= 13h, returns with the compare flags and
 * without touching A. Otherwise byte ES:[A+34h] := 0, the Pascal string at
 * CS:311B is copied into SS:BP-1Dh through raw 0AF8:0B0B, and raw 00BA:1693
 * displays it (its RETF 4 consumes the copied-string pointer).
 * Exactly 1 MiB A20-off RAM, attached EGA, IP3134, SP[0192,FFF2]; the lower
 * bound is 1693's own entry minimum 016A plus this frame (2+1Eh+8 bytes), the
 * upper bound keeps all 16 frame bytes below SS:FFFF. CS/DS/SS
 * full 64 KiB windows exclude A0000..AFFFF because 0B0B and 1693 read the
 * string and frames from flat RAM. Pointers A and B are NOT restricted: their
 * single byte accesses go through the bus, so aperture and aliased targets are
 * live. Code, the string literal's executed bytes and both far bindings are
 * immutable except as written by the callees' own contracts; the A write
 * must avoid code. Reached return frames do not wrap. No interrupts, faults,
 * self-modification or arbitrary-PC resume. CPU/bus/EGA/limits/diagnostic are
 * distinct host objects outside RAM.
 * CALL 316A/316F check opcode 9A and offsets 0B0B/1693 before the return frame
 * is pushed (CODE leaves IP at the call); each live segment word selects the
 * loaded body. CALLEE preserves the nested stop (callee=0 string load,
 * 1 display with its own diagnostic); TRANSFER an unexpected successful
 * return. ARGUMENT preserves CPU/RAM/EGA and resets the diagnostic.
 * Limits are 1693's fresh per-callee budgets, passed unchanged. */
pr_u38_3134_status pr_u38_message_3134_cpu(pr_bus *bus,size_t ram_size,
    pr_u38_3134_cpu *cpu,const pr_u38_3134_limits *limits,
    pr_u38_3134_diagnostic *diagnostic);
#endif
