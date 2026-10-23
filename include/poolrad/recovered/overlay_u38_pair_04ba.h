#ifndef POOLRAD_RECOVERED_OVERLAY_U38_PAIR_04BA_H
#define POOLRAD_RECOVERED_OVERLAY_U38_PAIR_04BA_H
#include "poolrad/recovered/overlay_u9e_entry16_10e4.h"
typedef pr_u9e_10e4_cpu pr_u38_04ba_cpu;
typedef enum {
    PR_U38_04BA_OK = 0,
    PR_U38_04BA_ARGUMENT,
    PR_U38_04BA_LIMIT,
    PR_U38_04BA_TRANSFER
} pr_u38_04ba_status;
/* Complete U38 [04BA,05D6), with raw U9E +110D/+10E4 callees. Enter
 * after CALL at IP=04BA; SS:SP+4 is unused, +6 is the source far pointer,
 * +10 is the record far pointer. RETF 10 consumes the actual live frame.
 * u9e_cs is the loaded callee code segment; overlay loader/stub effects
 * are outside the API (the original relocated stubs are transparent JMPs).
 * Exactly 1 MiB A20-off RAM, CPU outside RAM, immutable code required.
 * Word and LES operands are physically consecutive after wrapped 16-bit EA.
 * RETF pops IP/CS separately with wrapped SS offsets. LES segment seams
 * FFFD..FFFF are Unicorn-only evidence, not native parity. Logical AF=0.
 * Each +0518 loop head consumes one budget unit, including the final null
 * pointer check. LIMIT retains prior effects at the unexecuted head; no
 * resume API. TRANSFER keeps unexpected callee returns. ARGUMENT is atomic
 * and screens the deepest 32-byte stack span against all three code bodies.
 * Arbitrary data aliases are live; callers must exclude all code-writing
 * aliases, including far stores by +110D. See U38_PAIR_04BA.md. */
/* Required loaded overlay bindings are checked before CPU/RAM mutation.
 * A cold/malformed required stub returns ARGUMENT with CPU14/RAM unchanged.
 * Call words and loaded EA jumps must remain immutable during execution.
 * See docs/LOADED_STUB_GUARDS.md for the exact direct/inherited scope. */
pr_u38_04ba_status pr_u38_pair_04ba(uint8_t *ram, size_t ram_size,
                                  pr_u38_04ba_cpu *cpu, uint16_t u9e_cs,
                                  size_t budget);
#endif
