#ifndef POOLRAD_RECOVERED_OVERLAY_U25_LOADER_0000_H
#define POOLRAD_RECOVERED_OVERLAY_U25_LOADER_0000_H

#include "poolrad/recovered/overlay_u25_string_cores.h"

#define PR_U25_0000_RAM_SIZE PR_U25_STRINGS_RAM_SIZE
typedef pr_u25_strings_cpu pr_u25_0000_cpu;
typedef enum {
    PR_U25_0000_OK = 0,
    PR_U25_0000_ARGUMENT,
    PR_U25_0000_OPERAND_LIMIT,
    PR_U25_0000_STRING_LIMIT,
    PR_U25_0000_TRANSFER,
    PR_U25_0000_CALLEE
} pr_u25_0000_status;

/* U25 public entry 2, [0000,0136), composed with +06AF and complete
 * +1153/+12FB string cores. Enter IP=0000 after far CALL; SS:SP contains
 * return IP/CS and the operand-count word (only its low byte is used).
 * All counts 0..255 and raw DS-table overlap are retained. RETF 2.
 *
 * RAM must be exactly 1 MiB, synchronous, distinct from CPU. Original code
 * and loaded SYSTEM binding must be immutable; data writes may not overlap
 * code. system_cs is the already-loaded SYSTEM segment.
 * Initial SP in [022A,FFFA] reserves all nested frames without offset wrap.
 * Effective offsets wrap16; word/LES bytes are contiguous physical20.
 *
 * max_operands counts reached +0022 headers; OPERAND_LIMIT exposes the
 * state before that instruction. max_string_iterations is reset for each
 * string call and uses the string-core loop headers; STRING_LIMIT exposes
 * that callee's reached state. Both budgets must be positive. Neither limit
 * substitutes a successful DOS return or claims termination.
 * Unexpected callee returns expose TRANSFER; a rejected string entry
 * exposes CALLEE. All reached effects survive these stops.
 * Logical/shift AF is zero by convention; multi-bit shift OF is retained
 * until overwritten. Native route/boundary parity and strict credit remain
 * separate. Invalid initial arguments reject atomically.
 */
pr_u25_0000_status pr_u25_load_operands_0000(
    uint8_t *ram, size_t ram_size, pr_u25_0000_cpu *cpu,
    uint16_t system_cs, size_t max_operands, size_t max_string_iterations);

/* U25 public entry 9, [06AF,06D3) (overlay stub 002B:004D): the same
 * raw-CPU body the loader runs for tag 81h, exposed for far callers such as
 * U1C +00F4. Enter IP=06AF after a far CALL with SS:SP holding return
 * IP/CS, then two argument words. PUSH BP; MOV BP,SP; SUB SP,2;
 * AX := (low byte of the word at entry SP+4) << 8 plus the low byte of the
 * word at entry SP+6; [BP-2] := AX and reload; MOV SP,BP; POP BP; RETF 4.
 * The final flags are ADD AX,DX's (logical/shift AF zero, as above). RETF
 * pops IP and CS as separate words, each wrapping SP, like the loader's
 * nested calls. RAM must be exactly 1 MiB and IP must be 06AF; otherwise
 * ARGUMENT, with nothing changed. This adds no strict credit: +06AF's
 * identity is already strict. */
pr_u25_0000_status pr_u25_word_06af(uint8_t *ram, size_t ram_size, pr_u25_0000_cpu *cpu);

#endif
