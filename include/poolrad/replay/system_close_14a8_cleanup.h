#ifndef POOLRAD_REPLAY_SYSTEM_CLOSE_14A8_CLEANUP_H
#define POOLRAD_REPLAY_SYSTEM_CLOSE_14A8_CLEANUP_H
#include "poolrad/replay/slot_a_string_0356_abi.h"
typedef enum {
 PR_SYSTEM_CLOSE_CLEANUP_STEPPED=0,PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_LOADER,
 PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_MEMORY,PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_PC,
 PR_SYSTEM_CLOSE_CLEANUP_ARGUMENT
} pr_system_close_cleanup_status;
typedef struct {
 unsigned valid;
 uint16_t return_segment,return_pc;
} pr_system_close_cleanup_report;
/* Independent actual post-DOS131C:14A8 input -> real RETF4 -> loader2A8E:C54.
 * C never supplies a DOS result or replaces an in-flight state across DOS.
 * Requires audited code, observed return frame and disjoint state/report/RAM.
 * RAM<=1MiB; spans fit one64KiB segment; effective offsets wrap16bits.
 * Blocked outcomes preserve CPU/RAM; NULL arguments also preserve report.
 * DOS, full Close/loader, heap, sidecar, printer and playable credit excluded. */
pr_system_close_cleanup_status pr_system_close_14a8_step(
 pr_slot_a_directory_state *state,pr_system_close_cleanup_report *report);
#endif
