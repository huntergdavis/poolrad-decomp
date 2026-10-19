#ifndef POOLRAD_RECOVERED_RESIDENT_0709_SPACE_05EB_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0709_SPACE_05EB_CPU_H
#include "poolrad/recovered/memory_bus.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_r0709_space_cpu;
typedef enum {
    PR_R0709_SPACE_CPU_OK=0, PR_R0709_SPACE_CPU_ARGUMENT,
    PR_R0709_SPACE_CPU_CODE, PR_R0709_SPACE_CPU_CALLEE,
    PR_R0709_SPACE_CPU_TRANSFER=5
} pr_r0709_space_cpu_status;
typedef struct { int callee,status,glyph_callee,glyph_status; } pr_r0709_space_cpu_diagnostic;
/* Raw0709:[05EB,0626), RETF8. Four words: count,color,y,x (low bytes).
 * Rejects x>39 or y>24, otherwise calls raw01DF with transform1,char32.
 * It draws the supplied font glyph; it does not assume space pixels are zero.
 * The two unsigned-below-zero branches cannot be taken. All arguments stay
 * live; AH is preserved while their words are pushed. Count is not clipped.
 * Exactly1MiB A20off, attached EGA, IP05EB, SP[002C,FFF4]. DS/SS whole
 * segment windows exclude EGA. Code/bindings immutable outside EGA, writes
 * avoid code, reached frames do not wrap; other aliases remain live.
 * CPU/bus/EGA/diagnostic distinct from RAM and each other; callbacks only
 * observe/refuse OUT without mutation. Normal/386 flags. No arbitrary-PC
 * resume, interrupts or faults. CALL061F requires E8/FBBD and follows PUSH
 * CS; CODE retains that pushed word. CALLEE preserves glyph status/state;
 * TRANSFER retains an unexpected successful return. callee0=glyph,-1=none;
 * glyph_callee/status retain UpCase diagnostics. Budget is passed to glyph,
 * whose LIMIT stops before the exhausted transfer. ARGUMENT preserves
 * CPU/RAM/EGA and resets diagnostics. Native hardware scope is EGA;
 * packed modes0/2 ordinary RAM effects do not establish CGA/Tandy closure. */
pr_r0709_space_cpu_status pr_r0709_space_05eb_cpu(pr_bus *bus,size_t ram_size,
    pr_r0709_space_cpu *cpu,size_t max_loopbacks,pr_r0709_space_cpu_diagnostic *diagnostic);
#endif
