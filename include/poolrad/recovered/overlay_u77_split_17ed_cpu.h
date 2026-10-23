#ifndef POOLRAD_RECOVERED_OVERLAY_U77_SPLIT_17ED_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U77_SPLIT_17ED_CPU_H
#include "poolrad/recovered/overlay_u77_split_17ed.h"
#include "poolrad/recovered/resident_00ba_display_1693_cpu.h"
typedef enum {
    PR_U77_17ED_CPU_OK=0, PR_U77_17ED_CPU_ARGUMENT, PR_U77_17ED_CPU_CODE,
    PR_U77_17ED_CPU_CALLEE, PR_U77_17ED_CPU_TRANSFER
} pr_u77_17ed_cpu_status;
typedef struct {
    size_t clear_loopbacks,rect_loopbacks,printer_loopbacks,glyph_loopbacks;
    uint64_t delay_loopbacks;
    unsigned heap_steps;
} pr_u77_17ed_limits;
typedef struct {
    int callee,status;
    pr_r00ba_1693_diagnostic display;
} pr_u77_17ed_diagnostic;
/* Raw U77 [17ED,189D), RETF 4. SS:[SP+4/+6] is the node far pointer.
 * Composes actual GetMem 023F, Move 1685, StrLoad 0B0B and display 1693;
 * callee diagnostics use indices 0,1,2,3 respectively (-1 before a call).
 * Exactly 1 MiB A20-off RAM, attached EGA, IP17ED, SP[018C,FFF8]. The
 * lower bound allows display's 016A minimum plus 34 bytes of caller frame.
 * CS/DS/SS whole segment windows exclude A0000..AFFFF. All reached flat
 * GetMem/Move/StrLoad operands must also avoid that aperture; node and heap
 * aliases elsewhere remain live. Code/bindings are immutable, executed
 * frames do not wrap, and each live call segment selects the corresponding
 * loaded body at its original offset. LCALL opcode/offset checked before
 * pushing the real far frame; CODE retains the call IP, CALLEE retains the
 * child's CPU and diagnostic, TRANSFER retains an unexpected successful
 * child return. ARGUMENT preserves CPU/RAM/EGA and resets diagnostics.
 * Normal/386 flags; no interrupts, faults, or arbitrary-PC resume. Host
 * bus/CPU/EGA/limits/diagnostic objects are distinct and outside RAM.
 * Limits pass unchanged to actual callees. All nested display/device and
 * flat-runtime contracts apply. No semantic callee callbacks are accepted.
 */
pr_u77_17ed_cpu_status pr_u77_split_17ed_cpu(pr_bus *bus,size_t ram_size,
    pr_u77_17ed_cpu *cpu,const pr_u77_17ed_limits *limits,
    pr_u77_17ed_diagnostic *diagnostic);
#endif
