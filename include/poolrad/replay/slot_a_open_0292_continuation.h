#ifndef POOLRAD_REPLAY_SLOT_A_OPEN_0292_CONTINUATION_H
#define POOLRAD_REPLAY_SLOT_A_OPEN_0292_CONTINUATION_H
#include "poolrad/replay/slot_a_string_0356_abi.h"
typedef enum {
 PR_SLOT_A_OPEN_0292_STEPPED=0,PR_SLOT_A_OPEN_0292_BLOCKED_ASSIGN,
 PR_SLOT_A_OPEN_0292_BLOCKED_RESET,PR_SLOT_A_OPEN_0292_BLOCKED_MESSAGE,
 PR_SLOT_A_OPEN_0292_BLOCKED_RETRY,PR_SLOT_A_OPEN_0292_BLOCKED_CALLER,
 PR_SLOT_A_OPEN_0292_BLOCKED_MEMORY,PR_SLOT_A_OPEN_0292_BLOCKED_PC,
 PR_SLOT_A_OPEN_0292_ARGUMENT
} pr_slot_a_open_0292_status;
typedef pr_slot_a_string_0356_report pr_slot_a_open_0292_report;
/* Actual resident0292 caller continuation: separate native inputs after
 * selector, Assign and Reset. No runtime return, DOS callback, journal or
 * in-flight snapshot replacement supplied. Calls2C9/2D7/2AA block before
 * pushing their call frames. Retry289 and loaderC2B block before execution.
 * Requires audited original/loaded code and disjoint state/report/RAM;
 * spans fit one 64KiB segment and RAM<=1MiB. Pointer arithmetic wraps 16bits.
 * Blocked outcomes preserve CPU/RAM; NULL arguments also preserve report.
 * Full flags and AH are retained as original. No identity/count/playable credit. */
pr_slot_a_open_0292_status pr_slot_a_open_0292_step(
 pr_slot_a_directory_state *state,pr_slot_a_open_0292_report *report);
#endif
