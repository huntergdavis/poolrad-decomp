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

#endif
