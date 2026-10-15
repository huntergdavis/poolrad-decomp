#ifndef POOLRAD_REPLAY_SLOT_A_LOADER_BLOCKREAD_ABI_H
#define POOLRAD_REPLAY_SLOT_A_LOADER_BLOCKREAD_ABI_H
#include "poolrad/replay/slot_a_string_0356_abi.h"
typedef enum {
 PR_SLOT_A_BLOCKREAD_STEPPED=0,PR_SLOT_A_BLOCKREAD_BLOCKED_READ,
 PR_SLOT_A_BLOCKREAD_BLOCKED_CLOSE,PR_SLOT_A_BLOCKREAD_BLOCKED_SIDECAR,
 PR_SLOT_A_BLOCKREAD_BLOCKED_MEMORY,PR_SLOT_A_BLOCKREAD_BLOCKED_PC,
 PR_SLOT_A_BLOCKREAD_ARGUMENT
} pr_slot_a_blockread_status;
typedef pr_slot_a_string_0356_report pr_slot_a_blockread_report;
/* Actual C2B -> runtime14FB argument setup, independent native C49 Close
 * setup, and independent native C54 word stores to sidecar boundaryC8F.
 * No runtime read/close/DOS result, callback, journal or live-state replacement.
 * Typed calls block before pushing frames. Requires audited original/loaded
 * code and disjoint state/report/RAM; every span fits one 64KiB segment and
 * RAM<=1MiB. Effective offsets wrap 16bits. Undefined XOR AF follows observed 0.
 * Blocked outcomes preserve CPU/RAM; NULL arguments preserve report.
 * No full-loader/file/heap/printer/playable or function-count credit. */
pr_slot_a_blockread_status pr_slot_a_loader_blockread_step(
 pr_slot_a_directory_state *state,pr_slot_a_blockread_report *report);
#endif
