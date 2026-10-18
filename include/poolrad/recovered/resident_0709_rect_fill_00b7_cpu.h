#ifndef POOLRAD_RECOVERED_RESIDENT_0709_RECT_FILL_00B7_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0709_RECT_FILL_00B7_CPU_H
#include "poolrad/recovered/memory_bus.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_r0709_rect_cpu;
typedef enum {
    PR_R0709_RECT_CPU_OK=0, PR_R0709_RECT_CPU_ARGUMENT,
    PR_R0709_RECT_CPU_CODE, PR_R0709_RECT_CPU_CALLEE,
    PR_R0709_RECT_CPU_PORT, PR_R0709_RECT_CPU_TRANSFER, PR_R0709_RECT_CPU_LIMIT
} pr_r0709_rect_cpu_status;
typedef struct { int callee,status; } pr_r0709_rect_cpu_diagnostic;
/* Raw 0709:[00B7,0171), RETF12. Six argument words: color,page,bottom,
 * right,top,left (low bytes used). Composes raw +00A0/+0000 and bus FillChar.
 * Exact1MiB RAM, attached EGA, IP00B7, SP[0020,FFF0]; DS and SS segment
 * windows must not intersect A0000..AFFFF (the two raw helpers use RAM).
 * CPU/bus/EGA/diagnostic are distinct from RAM and each other. Code outside the EGA aperture; code and
 * bindings immutable, all writes avoid code, reached return frames do not
 * wrap. Other data/stack aliases remain live. Callbacks only observe/refuse.
 * +0140 must contain LCALL offset16A8; its live segment binds FillChar.
 * max_loopbacks counts taken +014F JNEs; zero allows the first row.
 * LIMIT retains the state before the exhausted JNE. A refused own OUT is
 * PORT; a helper stop is CALLEE (callee 0 page,1 reset,2 fill; -1 none).
 * Refused OUTs do not reach the EGA. TRANSFER retains changed helper returns.
 * ARGUMENT preserves CPU/RAM/EGA and resets the optional diagnostic.
 * Normal/386 flags; interrupts, faults, code mutation and other video
 * hardware outside the EGA aperture are outside this model. */
pr_r0709_rect_cpu_status pr_r0709_rect_fill_00b7_cpu(pr_bus *bus,size_t ram_size,
    pr_r0709_rect_cpu *cpu,size_t max_loopbacks,pr_r0709_rect_cpu_diagnostic *diagnostic);
#endif
