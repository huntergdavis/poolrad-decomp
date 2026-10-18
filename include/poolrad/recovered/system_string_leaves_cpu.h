#ifndef POOLRAD_RECOVERED_SYSTEM_STRING_LEAVES_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_STRING_LEAVES_CPU_H

#include "poolrad/recovered/system_runtime_leaves_cpu.h"

/* Raw-CPU, full-state models of two Turbo Pascal SYSTEM string leaves at
 * loaded segment 0AF8 (START.EXE expanded image), with the types and
 * conventions of system_runtime_leaves_cpu.h. Each is entered after the far
 * CALL with SS:[SP] the return IP and [SP+2] the return CS; [SP+4]/[SP+6]
 * is the source string S and [SP+8]/[SP+0Ah] the destination D, both read
 * through SS:[BX+disp] with BX := SP. Each ends with RETF 4, popping S and
 * leaving D on the stack. DF ends clear, DS keeps its entry value, CX ends
 * 0 and BX holds the entry SP. REP MOVSB copies one byte at a time forward,
 * rereading each source byte after earlier writes, so overlapping strings
 * behave as on the CPU; SI and DI wrap at 16 bits. */

/* String load (copy), 0AF8:0B0B..0B24 (26 bytes, 12 instructions): CLD;
 * MOV BX,SP; MOV DX,DS; LES DI,SS:[BX+8]; LDS SI,SS:[BX+4]; LODSB; STOSB
 * (the length); MOV CL,AL; XOR CH,CH; REP MOVSB; MOV DS,DX; RETF 4. AL is
 * the length; the final flags are XOR CH,CH's. */
pr_system_leaf_status pr_system_strload_0b0b(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);

/* String concatenation, 0AF8:0B98..0BC3 (44 bytes, 19 instructions): CLD;
 * MOV BX,SP; MOV DX,DS; LES DI,SS:[BX+8]; LDS SI,SS:[BX+4]; MOV CL,ES:[DI];
 * XOR CH,CH; LODSB; ADD ES:[DI],AL; on carry the length byte := FFh and AL
 * := NOT CL (the bytes that still fit); ADD DI,CX; INC DI; MOV CL,AL; REP
 * MOVSB; MOV DS,DX; RETF 4. AL is the count copied; the final flags are INC
 * DI's with CF from ADD DI,CX. */
pr_system_leaf_status pr_system_strcat_0b98(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);

#endif
