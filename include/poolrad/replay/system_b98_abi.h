#ifndef POOLRAD_REPLAY_SYSTEM_B98_ABI_H
#define POOLRAD_REPLAY_SYSTEM_B98_ABI_H
#include "poolrad/replay/slot_a_string_0356_abi.h"
typedef enum {
    PR_SYSTEM_B98_STEPPED=0,
    PR_SYSTEM_B98_BLOCKED_FILE_PROBE,
    PR_SYSTEM_B98_BLOCKED_MEMORY,
    PR_SYSTEM_B98_BLOCKED_PC,
    PR_SYSTEM_B98_ARGUMENT
} pr_system_b98_status;
/* One original SYSTEM 131C:B98..BC1 instruction; REP is one atomic host
 * step. RETF 4 reads the actual frame and retains the destination pointer.
 * Requires audited live CPU/RAM and code identity. Host spans must fit one
 * 64 KiB segment and ram_size <= 1 MiB; unsupported spans/DF/PCs block
 * without CPU/RAM changes. Individual pointers wrap to 16 bits. XOR AF is
 * normalized to observed zero; defined arithmetic flags are computed.
 * State and RAM must not overlap. No game/full-C identity claim. */
pr_system_b98_status pr_system_b98_step(pr_slot_a_directory_state *state);
/* Reached unit-6F 1999/19A3 calls and intervening caller instructions.
 * At +19A8 block before 0802:02EB, return +19AD. Report/state/RAM must not
 * alias. All blocked outcomes preserve CPU/RAM; NULL preserves report. */
pr_system_b98_status pr_slot_a_concat_1999_step(
    pr_slot_a_directory_state *state, pr_slot_a_string_0356_report *report);
#endif
