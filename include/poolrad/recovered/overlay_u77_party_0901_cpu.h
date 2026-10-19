#ifndef POOLRAD_RECOVERED_OVERLAY_U77_PARTY_0901_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U77_PARTY_0901_CPU_H
#include "poolrad/recovered/resident_03f1_clear_0137_cpu.h"
#include "poolrad/recovered/resident_0709_printer_0626_cpu.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u77_0901_cpu;
typedef struct { size_t clear_loopbacks,rect_loopbacks,printer_loopbacks,glyph_loopbacks; } pr_u77_0901_limits;
typedef struct {
    int callee,status;
    pr_r03f1_clear_cpu_diagnostic clear;
    int decimal_callee,decimal_status;
    pr_r0709_printer_cpu_diagnostic printer;
} pr_u77_0901_diagnostic;
typedef enum { PR_U77_0901_CLEAR=0,PR_U77_0901_DECIMAL,PR_U77_0901_PRINTER,
    PR_U77_0901_STRLOAD,PR_U77_0901_STRSTORE,PR_U77_0901_STRCAT } pr_u77_0901_callee;
typedef enum { PR_U77_0901_OK=0,PR_U77_0901_ARGUMENT,PR_U77_0901_CODE,
    PR_U77_0901_CALLEE,PR_U77_0901_TRANSFER } pr_u77_0901_status;
/* Complete U77 [0901,0A4C), RETF4. Arguments SS:[SP+4] color selector,
 * [SP+6] signed row/index byte. Clear a row, print its byte value from the
 * DS:5D92 record; row0/value18/positive extension prints a parenthesized
 * decimal extension, with leading padding below10 and special value100.
 * Exact1MiB A20off, attached EGA, IP0901, SP[0300,FFF4]. Initial DS/SS
 * windows, caller record reads, and ALL accesses by flat-RAM SYSTEM/decimal
 * callees are outside
 * A0000..AFFFF. Clear/printer hardware accesses use the bus. Code/bindings
 * immutable outside aperture; writes avoid code; reached frames nonwrapping.
 * Other aliases remain live. CPU/bus/EGA/limits/diag distinct from RAM and
 * each other. Callbacks observe or refuse OUT only, with no state mutation.
 * Each dependency invocation gets fresh limits. Reached LCALL opcode/offset
 * checked before CALL; live segment word selects the loaded dependency.
 * CALLEE keeps real nested status; TRANSFER keeps altered far return state;
 * neither rolls back. Optional diagnostics retain the last invocation of
 * each dependency. ARGUMENT changes only the optional diagnostic.
 * Normal/386 flags; no IRQs/faults/self-modification/arbitrary-PC resume.
 * All inherited drawing restrictions apply, including no CGA/Tandy hardware
 * closure claim for modes0/2's ordinary-memory effects. */
pr_u77_0901_status pr_u77_party_0901_cpu(pr_bus *bus,size_t ram_size,
    pr_u77_0901_cpu *cpu,const pr_u77_0901_limits *limits,pr_u77_0901_diagnostic *diagnostic);
#endif
