#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_SIDECAR_GROUPS_02FA_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_SIDECAR_GROUPS_02FA_CPU_H
#include "poolrad/recovered/overlay_u9e_sidecar_0160_cpu.h"

/* U9E [02FA,0ABB), RETF6: member offset, member segment, group word.
 * Groups 0..19 call their original sequence of +0160 sidecar types; other
 * low-byte values return. Every argument reload and partial AX write is live.
 * The composed +0160 contract, statuses, layout and diagnostic are reused.
 * A handler hit propagates PENDING at U9E001E before the indirect CALL;
 * no handler effects or subsequent group calls are synthesized.
 * Exact1MiB A20off, initial IP02FA, SP[0196,FFF6]; all other +0160
 * restrictions apply. Each +0160 call gets fresh supplied limits.
 * Original caller/callee code is immutable; reached near CALL bindings are
 * checked before execution. Data/stack aliases remain live.
 * ARGUMENT leaves RAM/CPU unchanged and resets optional diagnostic.
 * This is partial composition, not full-chain closure or a credit claim. */
/* Required loaded overlay bindings are checked before CPU/RAM mutation.
 * A cold/malformed required stub returns ARGUMENT with CPU14/RAM unchanged.
 * Call words and loaded EA jumps must remain immutable during execution.
 * See docs/LOADED_STUB_GUARDS.md for the exact direct/inherited scope. */
pr_u9e_0160_status pr_u9e_sidecar_groups_02fa_cpu(uint8_t *ram,size_t size,
    pr_u9e_0160_cpu *cpu,const pr_u9e_0160_layout *layout,
    const pr_u9e_0160_limits *limits,pr_u9e_0160_diagnostic *diagnostic);
#endif
