#ifndef POOLRAD_RECOVERED_OVERLAY_U33_0B97_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U33_0B97_CPU_H
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/overlay_u9e_entry16_10e4.h"

typedef pr_u9e_10e4_cpu pr_u33_0b97_regs;
typedef enum {
    PR_U33_0B97_RETURNED = 0,
    PR_U33_0B97_ARGUMENT,   /* null/size/IP/SP outside the contract; nothing changed */
    PR_U33_0B97_LIMIT,      /* the host budget ran out at the next unexecuted instruction */
    PR_U33_0B97_TRANSFER    /* RETF 0006h popped a return address other than the entry frame's (the body's own stores can reach it); the CPU is left at the popped CS:IP */
} pr_u33_0b97_status;

/* GAME.OVR unit 0x33 [0B97,0BDC): 71 bytes / 29 instructions. Enter after the call at IP 0B97. Frame (RETF 0006h pops the three argument words):
 * SP IP, SP+2 CS, SP+4 a near pointer P (an offset in SS), SP+6 the byte X, SP+8 the byte Y (each a word slot, read as a byte and sign-extended).
 * Reads the far pointer G at DS:[6716] (offset) and DS:[6718] (segment) and stores 34h at G + Y*50 + X + 7 and 35h at G + Y*50 + X + 8 (16-bit offset arithmetic, signed X and Y),
 * then increments the byte at SS:[P-2]. The three stores go THROUGH CALLER-SUPPLIED POINTERS and so go through the bus: they may alias each other, the stack frame or the EGA aperture.
 * Stack, code and the DS globals are flat RAM. Exactly 1 MiB of A20-off RAM; entry SP in [0010h,FFF6h]. Each instruction consumes one unit of *budget; LIMIT leaves the next unexecuted
 * instruction in cpu->ip. CPU, budget and bus storage are distinct. Code is immutable. */
pr_u33_0b97_status pr_u33_0b97_cpu(pr_bus *bus, size_t ram_size, pr_u33_0b97_regs *cpu, size_t *budget);
#endif
