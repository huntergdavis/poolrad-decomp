#ifndef POOLRAD_REPLAY_SLOT_A_PROBE_RETURN_19AD_ABI_H
#define POOLRAD_REPLAY_SLOT_A_PROBE_RETURN_19AD_ABI_H
#include "poolrad/recovered/slot_a_directory_fullstate.h"
typedef enum {
    PR_SLOT_A_PROBE_RETURN_STEPPED = 0,
    PR_SLOT_A_PROBE_RETURN_BLOCKED_PRESENT_CONTINUATION,
    PR_SLOT_A_PROBE_RETURN_BLOCKED_LOOP_GATE,
    PR_SLOT_A_PROBE_RETURN_BLOCKED_MEMORY,
    PR_SLOT_A_PROBE_RETURN_BLOCKED_PC,
    PR_SLOT_A_PROBE_RETURN_ARGUMENT
} pr_slot_a_probe_return_status;
/* One original instruction at 2A8E:19AD (OR AL,AL) or :19AF (JE :19EA).
 * Stops unchanged at :19B1 before allocation setup or :19EA before the
 * loop gate. Input must be actual audited return CPU/RAM; no file-probe
 * effects/results are supplied. Defined flags follow x86; undefined AF
 * is normalized to original-byte Unicorn/native zero. RAM is untouched.
 * Requires audited code identity, nonoverlapping state/RAM and nonnull
 * RAM with size <= 1 MiB. Unsupported states preserve CPU/RAM.
 * Bounded caller evidence only; no route, identity or full-C count credit. */
pr_slot_a_probe_return_status pr_slot_a_probe_return_19ad_step(
    pr_slot_a_directory_state *state);
#endif
