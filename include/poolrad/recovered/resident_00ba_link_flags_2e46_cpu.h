#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_LINK_FLAGS_2E46_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_LINK_FLAGS_2E46_CPU_H
#include "poolrad/recovered/resident_00ba_link_check_2e03_cpu.h"

typedef pr_res_2e03_cpu pr_res_2e46_cpu;
typedef enum {
    PR_RES_2E46_RETURNED = 0,
    PR_RES_2E46_ARGUMENT,   /* null RAM/CPU, RAM not exactly 1 MiB, IP not 2E46h or entry SP outside [0010h,FFF8h]; nothing changed */
    PR_RES_2E46_TRANSFER,   /* a RETF popped a return address other than the expected one (2E03's into 2E56, or this body's RETF 4); CPU at the popped CS:IP */
    PR_RES_2E46_CALLEE      /* the 2E03 model stopped otherwise (unreachable inside this contract) */
} pr_res_2e46_status;

/* START.EXE resident segment 00BA [2E46,2E8B): the character link flags test, 69 bytes / 27 instructions. Enter after the far call at IP 2E46 with
 * SS:[SP] return IP, [SP+2] return CS and [SP+4]/[SP+6] the character far pointer (offset, segment). PUSH BP; MOV BP,SP; SUB SP,2; it pushes the
 * character pointer and calls +2E03 (PUSH CS; CALL: the raw 2E03 model runs in place, RETF 4). OR AL,AL; a zero result gives the byte [BP-1] := 0.
 * Otherwise it reloads the character's link far pointer (+0CCh), takes the link's byte +2Eh as an index, DI := index SHL 4 (CL = 4), and the result is
 * 1 when (DS:[DI+5590h] AND 14h) == 14h, else 0. AL := [BP-1]; MOV SP,BP; POP BP; RETF 4. The flags follow the pinned DOSBox-X stored rules (AND and
 * OR clear CF/OF/AF; SHL sets AF for a non-zero count); LES reads its segment word at (offset+2) mod 64K as in 2E03. All accesses are flat RAM. Exactly
 * 1 MiB of A20-off RAM; entry SP in [0010h,FFF8h] so 2E03's entry SP (12 below) stays inside its own contract. Code is immutable. */
pr_res_2e46_status pr_res_00ba_link_flags_2e46_cpu(uint8_t *ram, size_t ram_size, pr_res_2e46_cpu *cpu);
#endif
