#ifndef POOLRAD_RECOVERED_RESIDENT_0709_GLYPH_01DF_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0709_GLYPH_01DF_CPU_H
#include "poolrad/recovered/memory_bus.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_r0709_glyph_cpu;
typedef enum {
    PR_R0709_GLYPH_CPU_OK=0, PR_R0709_GLYPH_CPU_ARGUMENT,
    PR_R0709_GLYPH_CPU_CODE, PR_R0709_GLYPH_CPU_CALLEE,
    PR_R0709_GLYPH_CPU_PORT, PR_R0709_GLYPH_CPU_TRANSFER, PR_R0709_GLYPH_CPU_LIMIT
} pr_r0709_glyph_cpu_status;
typedef struct { int callee,status; } pr_r0709_glyph_cpu_diagnostic;
/* Raw 0709:[01DF,05EB), RETF12. Six words: transform,character,count,
 * color,y,x (low bytes). One checked far binding +0207 -> SYSTEM182A;
 * its live segment binds the raw UpCase leaf. Stack/data accesses stay live.
 * Exact1MiB A20off, attached EGA, IP01DF, SP[001A,FFF0]. Initial DS/SS
 * segment windows exclude A0000..AFFFF; code/bindings immutable outside that
 * aperture, writes avoid code, reached return frames do not wrap. CPU/bus/
 * EGA/diagnostic are distinct from RAM and each other. Other aliases remain live.
 * pr_bus models planar EGA at A0000..AFFFF and ordinary RAM elsewhere.
 * Modes0/2's B800/BA00/BC00/BE00 writes do not imply other hardware models.
 * max_loopbacks counts every taken backward branch/jump, shared across the
 * call. LIMIT stops before the exhausted transfer. A refused OUT is PORT,
 * does not update EGA, and preserves the pre-OUT CPU. Callbacks only observe
 * their supplied operands or refuse OUT; they cannot change execution state.
 * CALLEE/TRANSFER preserve stopped UpCase/unexpected return state.
 * ARGUMENT leaves CPU/RAM/EGA unchanged; optional diagnostic resets.
 * Normal/386 flags; interrupts, faults and unsupported EGA modes excluded. */
pr_r0709_glyph_cpu_status pr_r0709_glyph_01df_cpu(pr_bus *bus,size_t ram_size,
    pr_r0709_glyph_cpu *cpu,size_t max_loopbacks,pr_r0709_glyph_cpu_diagnostic *diagnostic);
#endif
