#ifndef POOLRAD_RECOVERED_SYSTEM_STR_1366_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_STR_1366_CPU_H

#include "poolrad/recovered/system_runtime_leaves_cpu.h"

/* Raw-CPU, full-state model of Turbo Pascal Str(X: Longint : Width, S) in the
 * START.EXE SYSTEM segment (0AF8 at load segment 0): the far entry
 * 1366..13B0 (75 bytes, 39 instructions) and its near longint-to-decimal
 * core 1254..12A5 (82 bytes, 37 instructions), which reads the ten 32-bit
 * powers of ten 10^9..1 at CS:12A6..12CD. Entered after a far call: SS:[SP]
 * return IP, [SP+2] return CS, then [SP+4] the result's maximum length M,
 * [SP+6]/[SP+8] the result S (offset, segment), [SP+0Ah] Width and
 * [SP+0Ch]/[SP+0Eh] X (low word first); RETF 0Ch.
 *  1. PUSH BP; MOV BP,SP; SUB SP,20h; the digits go to the temporary SS:BP-20h
 *     through ES := SS (PUSH SS; POP ES) and the near CALL 1254, whose return
 *     word +137A and saved DI stay below the temporary.
 *  2. The core: a negative X is negated (two's complement) after a '-'; the
 *     first power not above the value is found among 10^9..10 (or 1), and
 *     each digit is the count of subtractions before a borrow; CX := the
 *     characters written, DI restored, BX = 0 and SI = 12CEh after it.
 *  3. With signed comparisons, Width := min(Width, M), the count := min(count,
 *     M) and the length L := max(Width, count); S := L, then L - count spaces
 *     (PUSH CX; REP STOSB; POP CX) and the first count characters. DS is
 *     saved and restored around the copy (PUSH DS at BP-22h, over the call's
 *     return word).
 * At return DX = M, BX = 0, CX = 0, AX = 0 or, after padding, (pad & FF00h)
 * | 20h; DF is clear and the arithmetic flags are SUB AX,CX's. Every operand
 * is reread from RAM in source order; LES and the RETF frame are read
 * physically contiguous; offsets wrap at 16 bits, physical addresses at
 * 1 MiB. The code must sit at its original offsets, hence IP = 1366h, and
 * the power table at CS:12A6 must be the original: otherwise, or for null
 * RAM or CPU or RAM that is not exactly 1 MiB, ARGUMENT before any change. */
pr_system_leaf_status pr_system_str_1366(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu);

#endif
