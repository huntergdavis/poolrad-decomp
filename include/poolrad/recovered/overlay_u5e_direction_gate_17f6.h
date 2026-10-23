#ifndef POOLRAD_RECOVERED_OVERLAY_U5E_DIRECTION_GATE_17F6_H
#define POOLRAD_RECOVERED_OVERLAY_U5E_DIRECTION_GATE_17F6_H
#include "poolrad/recovered/overlay_u5e_direction_263d.h"

/* Complete U5E public entry 11, [17F6,1884), including the +263D chain.
 * Enter after far CALL with IP=17F6, SP in [0032,FFF4], two far-pointer
 * arguments at SS:SP+4 and +8. The first points to a record whose +108
 * far pointer selects the counter/orientation record. Exactly 1 MiB RAM;
 * CPU, layout and RAM are distinct, stable objects. Code is immutable;
 * data writes must not overlap composed code. Stack/code alias rejection
 * is atomic. Overlay loading is represented by the supplied layout.
 * max_iterations has the +263D contract (slot-search and predicate loops).
 * Non-OK dependency statuses expose its reached CPU/RAM; TRANSFER exposes
 * a changed nested return. OK means the outer RETF 8 executed, not that
 * its return address was validated. Word/LES reads are physically
 * contiguous with A20 wrapping; effective offsets and SP wrap at 16 bits.
 * Logical AF=0; IDIV preserves input flags (undefined in the native ISA).
 * Native-route reachability and strict-credit classification are separate.
 */
/* Required loaded overlay bindings are checked before CPU/RAM mutation.
 * A cold/malformed required stub returns ARGUMENT with CPU14/RAM unchanged.
 * Call words and loaded EA jumps must remain immutable during execution.
 * See docs/LOADED_STUB_GUARDS.md for the exact direct/inherited scope. */
pr_u5e_263d_status pr_u5e_direction_gate_17f6(
    uint8_t *ram, size_t ram_size, pr_u5e_263d_cpu *cpu,
    const pr_u5e_263d_layout *layout, size_t max_iterations);
#endif
