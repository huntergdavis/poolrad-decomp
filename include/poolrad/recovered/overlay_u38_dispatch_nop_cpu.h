#ifndef POOLRAD_RECOVERED_OVERLAY_U38_DISPATCH_NOP_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_DISPATCH_NOP_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U38_NOP_RAM_SIZE (1u<<20)
typedef struct {
    uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags;
} pr_u38_nop_cpu;
typedef enum { PR_U38_NOP_OK=0, PR_U38_NOP_ARGUMENT } pr_u38_nop_status;
/* Four distinct U38 handlers: +0647, +1355, +135C, +319B, each seven
 * bytes / four instructions: PUSH BP; MOV BP,SP; POP BP; RETF 10.
 * Enter after far CALL with return IP/CS and five argument words on SS:SP.
 * All FLAGS and general/segment registers survive, except actual CS:IP and
 * SP advanced by14. The saved BP write at entry SS:SP-2 remains observable.
 * Exact1MiB A20off RAM, CPU separate from RAM, IP one of these four entries,
 * SP[0002,FFFC], immutable code, no interrupts/faults. Word reads/writes are
 * physically contiguous; physical addresses wrap at1MiB. Final SP wraps.
 * ARGUMENT leaves CPU/RAM untouched. No original instruction bytes needed.
 * The handler dispatchers and surrounding game route are outside this API.
 */
pr_u38_nop_status pr_u38_dispatch_nop_cpu(uint8_t *ram,size_t ram_size,
    pr_u38_nop_cpu *cpu);
#endif
