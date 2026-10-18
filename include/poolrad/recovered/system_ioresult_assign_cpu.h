#ifndef POOLRAD_RECOVERED_SYSTEM_IORESULT_ASSIGN_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_IORESULT_ASSIGN_CPU_H

#include "poolrad/recovered/system_runtime_leaves_cpu.h"

/* Raw-CPU, full-state models of two Turbo Pascal SYSTEM leaves in the
 * START.EXE SYSTEM segment (0AF8 at load segment 0), each entered after a
 * far call (SS:[SP] return IP, [SP+2] return CS). Null RAM or CPU, or RAM
 * that is not exactly 1 MiB, returns ARGUMENT before any change; otherwise
 * RETURNED with CS:IP at the popped return address.
 *
 * IOResult 04A2..04A8 (7 bytes, 3 instructions): XOR AX,AX; XCHG
 * [4426h],AX; RETF. AX := the word DS:[4426h] (InOutRes), which becomes 0;
 * the arithmetic flags are XOR AX,AX's.
 *
 * Assign 13E2..140F (46 bytes, 23 instructions): [SP+4] the name S, [SP+8]
 * the file record F (both far); RETF 8. MOV BX,SP; PUSH DS; CLD; F's first
 * word := 0, its second := D7B0h, the next 22 words := 0 (STOSW), then
 * min(length(S), 79) characters of S at F+48 and a NUL byte. AX = 0 and the
 * arithmetic flags are XOR AL,AL's; DS returns through the stack (PUSH DS
 * ... POP DS, reread from RAM). Word stores, the LES/LDS pairs, POP and the
 * RETF frame are read or written physically contiguous; offsets wrap at 16
 * bits, physical addresses at 1 MiB. */
pr_system_leaf_status pr_system_ioresult_04a2(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);
pr_system_leaf_status pr_system_assign_13e2(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);

#endif
