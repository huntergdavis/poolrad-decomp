#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_ZERO_TEST_23B7_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_ZERO_TEST_23B7_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_RES_23B7_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_res_23b7_cpu;

typedef enum {
    PR_RES_23B7_RETURNED = 0,
    PR_RES_23B7_ARGUMENT,   /* null RAM/CPU, RAM not exactly 1 MiB, IP not 23B7h or entry SP outside [0004h,FFF8h]; nothing changed */
    PR_RES_23B7_TRANSFER    /* RETF 4 popped a return address other than the entry frame's; the CPU is left at the popped CS:IP */
} pr_res_23b7_status;

/* START.EXE resident segment 00BA [23B7,23DB): the character field-zero test, 36 bytes / 13 instructions, a leaf. Enter after the far call at
 * IP 23B7 with SS:[SP] return IP, [SP+2] return CS and [SP+4]/[SP+6] the character far pointer. PUSH BP; MOV BP,SP; SUB SP,2; LES DI,[BP+6];
 * CMP BYTE ES:[DI+10Eh],0; the result byte [BP-1] is 1 when that byte is zero, else 0; AL := [BP-1]; MOV SP,BP; POP BP; RETF 4.
 * The FULL pinned flags are computed: SUB SP,2's six arithmetic flags, then CMP's, which reach the RETF. LES follows pinned DOSBox-X: the
 * offset word at the EA, the segment word at (offset+2) mod 64K. Every operand is reread from RAM; all accesses are flat. Exactly 1 MiB of
 * A20-off RAM; entry SP in [0004h,FFF8h]. Code is immutable. The one raw translation of this body: unit 0x31 +0255 calls it. */
pr_res_23b7_status pr_res_00ba_zero_test_23b7_cpu(uint8_t *ram, size_t ram_size, pr_res_23b7_cpu *cpu);
#endif
