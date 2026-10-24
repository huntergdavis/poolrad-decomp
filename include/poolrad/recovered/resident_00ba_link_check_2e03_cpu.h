#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_LINK_CHECK_2E03_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_LINK_CHECK_2E03_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_RES_2E03_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_res_2e03_cpu;

typedef enum {
    PR_RES_2E03_RETURNED = 0,
    PR_RES_2E03_ARGUMENT,   /* null RAM/CPU, RAM not exactly 1 MiB, IP not 2E03h or entry SP outside [0004h,FFF8h]; nothing changed */
    PR_RES_2E03_TRANSFER    /* RETF 4 popped a return address other than the entry frame's; the CPU is left at the popped CS:IP */
} pr_res_2e03_status;

/* START.EXE resident segment 00BA [2E03,2E46): the character link check, 67 bytes / 23 instructions, a leaf with no loop. Enter after the far
 * call at IP 2E03 with SS:[SP] return IP, [SP+2] return CS and [SP+4]/[SP+6] the character far pointer (offset, segment).
 * PUSH BP; MOV BP,SP; SUB SP,2. The link far pointer is the character's words at +0CCh/+0CEh: OR of the two words is zero -> the result byte
 * [BP-1] := 0. Otherwise LES re-reads the character pointer, LES DI,ES:[DI+0CCh] loads the link, AL := ES:[DI+2Eh], XOR AH,AH, DI := AX SHL 4
 * (CL = 4), and the result is 1 when the byte DS:[DI+558Eh] is above 1 (unsigned: JBE), else 0. AL := [BP-1]; MOV SP,BP; POP BP; RETF 4.
 * Flags follow the pinned DOSBox-X stored rules: at return they are OR AX's (CF=OF=AF=0) on the zero-link path and CMP's on the other; SHL's
 * (AF=1 for a non-zero count, OF=MSB(result) XOR CF) and XOR's are computed and then overwritten. Every operand is reread from RAM in source
 * order; LES reads its segment word at (offset+2) mod 64K, as pinned DOSBox-X does for offsets above FFFCh. All accesses are flat RAM (the far pointers name ordinary data). Exactly 1 MiB of A20-off RAM; entry SP in [0004h,FFF8h] so the frame,
 * the local and the arguments never straddle the segment end. Code is immutable. */
pr_res_2e03_status pr_res_00ba_link_check_2e03_cpu(uint8_t *ram, size_t ram_size, pr_res_2e03_cpu *cpu);
#endif
