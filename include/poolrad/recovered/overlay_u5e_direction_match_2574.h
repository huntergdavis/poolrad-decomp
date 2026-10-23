#ifndef POOLRAD_RECOVERED_OVERLAY_U5E_DIRECTION_MATCH_2574_H
#define POOLRAD_RECOVERED_OVERLAY_U5E_DIRECTION_MATCH_2574_H
#include "poolrad/recovered/overlay_u5e_direction_263d.h"

/* Complete U5E [2574,263D), including the +263D direction chain.
 * Enter after far CALL at IP=2574, SP in [003C,FFF4], with far-pointer
 * arguments at SS:SP+4 and +8. Exactly 1 MiB RAM; CPU, layout and RAM
 * are distinct stable objects. Code is immutable and data writes must
 * not overlap composed code. Initial argument and reserved stack/code
 * alias rejection is atomic. Loaded overlay segments come from layout.
 * max_iterations has the +263D slot-search/predicate-loop contract;
 * non-OK callee results expose its reached CPU/RAM. TRANSFER means a
 * changed nested return. OK means the outer RETF 8 executed; AL holds
 * the predicate byte. Effective offsets/SP wrap at 16 bits; word/LES
 * operand bytes are contiguous with A20 wrap. Logical AF is chosen zero.
 * Native reachability and ownership/strict-credit review are separate.
 */
/* Required loaded overlay bindings are checked before CPU/RAM mutation.
 * A cold/malformed required stub returns ARGUMENT with CPU14/RAM unchanged.
 * Call words and loaded EA jumps must remain immutable during execution.
 * See docs/LOADED_STUB_GUARDS.md for the exact direct/inherited scope. */
pr_u5e_263d_status pr_u5e_direction_match_2574(
    uint8_t *ram, size_t ram_size, pr_u5e_263d_cpu *cpu,
    const pr_u5e_263d_layout *layout, size_t max_iterations);
#endif
