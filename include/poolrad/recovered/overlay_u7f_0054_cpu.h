#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_0054_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_0054_CPU_H
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/overlay_u7f_entry0_0000_cpu.h"

typedef enum {
    PR_U7F_0054_RETURNED = 0,
    PR_U7F_0054_ARGUMENT,   /* null/size/IP/SP outside the contract; nothing changed */
    PR_U7F_0054_CODE,       /* a call site (0061 or 0080) is not the original `E8` + displacement; IP at the call, before its return frame */
    PR_U7F_0054_LIMIT,      /* the host budget ran out at the next unexecuted instruction (in this body or in 0000 and its resident closure) */
    PR_U7F_0054_TRANSFER,   /* a return went somewhere other than expected: 0000 returned elsewhere than 0064/0083, or RETF 000A popped a return address other than the entry frame's (the body's own store through the out pointer can reach it); the CPU is left at the popped CS:IP */
    PR_U7F_0054_CALLEE      /* the 0000 model stopped otherwise; diagnostic.callee_status says how */
} pr_u7f_0054_status;

typedef struct { int callee_status; uint16_t site; } pr_u7f_0054_diagnostic;

/* GAME.OVR unit 0x7F [0054,00A8): 84 bytes / 34 instructions. Enter after `PUSH CS; CALL 0054` (or a far call) at IP 0054. Frame (RETF 000Ah pops all ten argument bytes):
 * SP IP, SP+2 CS, SP+4 the out pointer's offset, SP+6 its segment, SP+8 a word `extra`, SP+10 the record pointer's offset, SP+12 its segment.
 * With V = pr_u7f_entry0_0000_cpu(record) (the 05DC-offset class value) and W = the word at record+0102h: if W + extra (16-bit) is above V (unsigned) the body stores
 * V - W through the out pointer and returns AL = 1; otherwise it stores 0 and returns AL = 0.
 * The body's own accesses THROUGH CALLER-SUPPLIED POINTERS (the record's word at +0102h, read twice, and the out store) go through the bus: they may alias each other, the stack, the heap or the
 * EGA aperture. Stack and code accesses are flat RAM; 0000 and the resident closure keep their own flat contracts. Exactly 1 MiB of A20-off RAM; entry SP in [0050h,FFF2h]. Each instruction of this body
 * and of 0000 and the resident closure consumes one unit of *budget; LIMIT leaves the next unexecuted instruction in cpu->cs:ip. CPU, budget, bus and diagnostic storage are distinct. Code is immutable. */
pr_u7f_0054_status pr_u7f_0054_cpu(pr_bus *bus, size_t ram_size, pr_res00ba_cpu *cpu, size_t *budget, pr_u7f_0054_diagnostic *diagnostic);
#endif
