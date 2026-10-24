#ifndef POOLRAD_RECOVERED_OVERLAY_U25_CHAR_REMAP_06D3_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U25_CHAR_REMAP_06D3_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_U25_06D3_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_06d3_cpu;

typedef enum {
    PR_U25_06D3_RETURNED = 0,
    PR_U25_06D3_ARGUMENT,   /* null RAM/CPU, RAM not exactly 1 MiB, IP not 06D3h or entry SP outside [0004h,FFFAh]; nothing changed */
    PR_U25_06D3_TRANSFER    /* RETF 2 popped a return address other than the entry frame's; the CPU is left at the popped CS:IP */
} pr_u25_06d3_status;

/* GAME.OVR unit 0x25 [06D3,06FB): the six-bit text-code remap, 40 bytes / 16 instructions, a leaf (no public entry: callers PUSH CS; CALL).
 * Enter at IP 06D3 with SS:[SP] return IP, [SP+2] return CS and the code byte at [SP+4]. PUSH BP; MOV BP,SP; SUB SP,2; CMP BYTE [BP+6],1Fh;
 * JBE: a code above 1Fh is kept, else AL := code, XOR AH,AH; ADD AX,40h; the byte goes through [BP-1] to AL; MOV SP,BP; POP BP; RETF 2.
 * FULL pinned flags: SUB SP,2's, then CMP's; on the JBE path XOR AH,AH's and then ADD's, which reach the RETF (CMP's otherwise). Every operand
 * is reread from RAM; all accesses are flat. Exactly 1 MiB of A20-off RAM; entry SP in [0004h,FFFAh]. The shared entry for the full-state
 * body inside the credited overlay_u25_string_cores.c (its +1153 and +12FB call it), which also defines this function; pr_rec_ecl_char_remap
 * is its semantic view. */
pr_u25_06d3_status pr_u25_char_remap_06d3_cpu(uint8_t *ram, size_t ram_size, pr_u25_06d3_cpu *cpu);
#endif
