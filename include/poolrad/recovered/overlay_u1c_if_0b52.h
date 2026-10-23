#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_IF_0B52_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_IF_0B52_H

#include "poolrad/recovered/overlay_u25_skip_1ea6.h"

#define PR_U1C_0B52_RAM_SIZE PR_U25_1EA6_RAM_SIZE
typedef pr_u25_1ea6_cpu pr_u1c_0b52_cpu;
typedef enum {
    PR_U1C_0B52_OK = 0,
    PR_U1C_0B52_ARGUMENT,
    PR_U1C_0B52_OPERAND_LIMIT,
    PR_U1C_0B52_STRING_LIMIT,
    PR_U1C_0B52_TRANSFER,
    PR_U1C_0B52_CALLEE
} pr_u1c_0b52_status;

/* Complete U1C IF-family [0B52,0BC8), opcodes 16..1B. Enter after far
 * CALL with IP=0B52 and return IP/CS at SS:SP, no arguments; RETF.
 * PC increments before the latch is read. A zero selected comparison
 * byte invokes the complete U25 +1EA6 skip/loader/string chain.
 * Nonzero comparison bytes and other latch values return directly.
 *
 * RAM must be exactly 1 MiB, synchronous and distinct from CPU. Original
 * code and loaded overlay/SYSTEM bindings must remain immutable; data
 * writes must not modify code. u25_cs and system_cs are already-loaded
 * body segments; the real overlay stub is a flag/stack-neutral far JMP.
 * Initial SP in [0238,FFFC] reserves every nested frame without wrap.
 * Effective offsets wrap16; word bytes are contiguous physical20.
 *
 * Positive budgets and nested LIMIT/TRANSFER/CALLEE outcomes follow the
 * +1EA6 contract and expose all reached effects. They never synthesize a
 * DOS return or promise termination. Undefined flags follow the composed
 * loader convention. Invalid initial arguments reject atomically.
 * Native route parity and independent strict credit remain separate.
 */
/* Required loaded overlay bindings are checked before CPU/RAM mutation.
 * A cold/malformed required stub returns ARGUMENT with CPU14/RAM unchanged.
 * Call words and loaded EA jumps must remain immutable during execution.
 * See docs/LOADED_STUB_GUARDS.md for the exact direct/inherited scope. */
pr_u1c_0b52_status pr_u1c_if_0b52(
    uint8_t *ram, size_t ram_size, pr_u1c_0b52_cpu *cpu,
    uint16_t u25_cs, uint16_t system_cs,
    size_t max_operands, size_t max_string_iterations);

#endif
