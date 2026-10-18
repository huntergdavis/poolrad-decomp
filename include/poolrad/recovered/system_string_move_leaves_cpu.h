#ifndef POOLRAD_RECOVERED_SYSTEM_STRING_MOVE_LEAVES_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_STRING_MOVE_LEAVES_CPU_H

#include "poolrad/recovered/system_runtime_leaves_cpu.h"

/* Raw-CPU, full-state models of four Turbo Pascal SYSTEM leaves in the
 * START.EXE SYSTEM segment (0AF8 at load segment 0), each entered after a
 * far call with SS:[SP] the return IP and [SP+2] the return CS. Every leaf
 * reads its frame through SS:[BX+disp] with BX := SP, and leaves BX = the
 * entry SP; the pointer pairs (LES/LDS) and the RETF frame are read
 * physically contiguous. REP MOVSB/CMPSB step one byte at a time and reread
 * each source byte, so overlapping strings behave as on the CPU. SI and DI
 * wrap at 16 bits, physical addresses at 1 MiB. Null RAM or CPU, or RAM
 * that is not exactly 1 MiB, returns ARGUMENT before any change; otherwise
 * RETURNED with CS:IP at the popped return address.
 *
 * Copy 0B57..0B97 (65 bytes, 29 instructions): [SP+4] Count, [SP+6]
 * Index, [SP+8] the string S, [SP+0Ch] the result D; RETF 8 leaves D on the
 * stack. CLD; Index <= 0 (signed) becomes 1 and SI := S + Index; if
 * length(S) < Index, AX := 0 (XOR AX,AX's flags); otherwise AX :=
 * length(S) - Index + 1, a negative Count becomes 0, and AX := min(AX,
 * Count) unsigned (CMP AX,CX's flags). D's length byte := AL and AX bytes
 * are copied; DS returns through DX.
 *
 * String compare 0C10..0C3A (43 bytes, 20 instructions): [SP+4] the right
 * string R, [SP+8] the left L; RETF 8. CLD; AL := length(L), AH :=
 * length(R), CL := the smaller; if CL is 0, CH keeps its entry value,
 * otherwise CH := 0 and REPE CMPSB compares L's and R's text. A difference
 * leaves that CMPSB's flags; otherwise the flags are CMP AL,AH's. DS
 * returns through DX.
 *
 * Char to string 0C3B..0C4C (18 bytes, 8 instructions): [SP+4] the char
 * word, [SP+6] the result D; RETF 2 leaves D on the stack. CLD; D := the
 * one-character string (bytes 1 and the char); AL := the char; no
 * arithmetic flag changes.
 *
 * Move 1685..16A7 (35 bytes, 16 instructions): [SP+4] Count, [SP+6] the
 * destination, [SP+0Ah] the source; RETF 0Ah. CLD; CMP SI,DI compares the
 * offsets only. Source offset >= destination offset copies forward (CMP's
 * flags); otherwise SI and DI move to their last byte (ADD, DEC), STD, and
 * the copy runs backward, so DF stays set on return, CF is ADD DI,CX's and
 * the other arithmetic flags are DEC DI's. AX is unchanged; DS returns
 * through DX. */
pr_system_leaf_status pr_system_strcopy_0b57(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);
pr_system_leaf_status pr_system_strcmp_0c10(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);
pr_system_leaf_status pr_system_charstr_0c3b(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);
pr_system_leaf_status pr_system_move_1685(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);

#endif
