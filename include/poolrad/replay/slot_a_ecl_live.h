#ifndef POOLRAD_REPLAY_SLOT_A_ECL_LIVE_H
#define POOLRAD_REPLAY_SLOT_A_ECL_LIVE_H

#include "poolrad/recovered/ecl_helpers.h"

#include <stddef.h>
#include <stdint.h>

typedef enum {
    PR_SLOT_A_ECL_BIND_OK = 0,
    PR_SLOT_A_ECL_BIND_ARGUMENT,
    PR_SLOT_A_ECL_BIND_BOUNDS,
    PR_SLOT_A_ECL_BIND_BAD_NAME,
    PR_SLOT_A_ECL_BIND_CALL_LIST
} pr_slot_a_ecl_bind_status;

/* Bind one native RAM stop immediately before unit 0x1C:+0x3171 to the
 * existing callback runner. `entry` is the word passed by the caller, not
 * the stale pre-call DS:[0x49ED]. The RAM and its script/record bytes must
 * outlive the state. Windows are copied as little-endian words; writes made
 * by handlers remain in `out`. On failure `out` is unchanged.
 *
 * This is a debugger-state adapter. It does not manufacture handler
 * callbacks or resolve a nonempty native GOSUB heap list. A caller passes
 * an explicit pr_rec_ecl_dispatch_table to pr_rec_ecl_run and must preserve
 * its UNMODELLED and HANDLER outcomes. */
pr_slot_a_ecl_bind_status pr_slot_a_ecl_bind_ram(
    const uint8_t *ram, size_t ram_size, uint16_t ds, uint16_t entry,
    pr_rec_ecl_loader_state *out);

#endif
