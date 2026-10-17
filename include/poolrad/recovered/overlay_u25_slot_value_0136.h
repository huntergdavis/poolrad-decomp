#ifndef POOLRAD_RECOVERED_OVERLAY_U25_SLOT_VALUE_0136_H
#define POOLRAD_RECOVERED_OVERLAY_U25_SLOT_VALUE_0136_H

#include "poolrad/recovered/overlay_u25_entry1_0136.h"

typedef pr_u25_0136_cpu pr_u25_slot_value_cpu;
typedef enum {
    PR_U25_SLOT_VALUE_OK = 0,
    PR_U25_SLOT_VALUE_ARGUMENT,
    PR_U25_SLOT_VALUE_TRANSFER,
    PR_U25_SLOT_VALUE_CALLEE
} pr_u25_slot_value_status;

/* Complete U25 entry 1 [0136,01B6), including both fixed callees +06AF
 * and +0E0C and the latter's classifier/load-hook/party-index chain.
 * Enter IP=0136 after far CALL: return IP/CS and index word at SS:SP.
 * The low index byte is sign-extended independently for each table read.
 * Unmatched tags return the actual unwritten stack-local word. RETF 2.
 *
 * Exactly 1 MiB A20-off RAM, distinct from CPU, synchronous, with immutable
 * loaded U25 code. Data writes must not overlap code. All 16-bit initial
 * SP values are supported; effective offsets and SP wrap16, multi-byte
 * memory operands are contiguous physical20. RETF uses the contiguous
 * four-byte frame profile of the load-word/Unicorn audit. No external callback or
 * guessed callee result is accepted. Cyclic party lists do not terminate,
 * matching the complete load-word dependency; this API has no loop budget.
 * An altered internal return exposes TRANSFER and all reached effects;
 * CALLEE reports a rejected nested entry. Undefined flags follow the
 * composed callees' convention, without a native-route claim. Invalid
 * initial arguments reject without changing CPU/RAM.
 */
pr_u25_slot_value_status pr_u25_slot_value_0136(
    uint8_t *ram, size_t ram_size, pr_u25_slot_value_cpu *cpu);

#endif
