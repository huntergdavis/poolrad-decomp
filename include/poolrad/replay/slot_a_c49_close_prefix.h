#ifndef POOLRAD_REPLAY_SLOT_A_C49_CLOSE_PREFIX_H
#define POOLRAD_REPLAY_SLOT_A_C49_CLOSE_PREFIX_H
#include "poolrad/replay/slot_a_string_0356_abi.h"
typedef enum {
 PR_C49_CLOSE_STEPPED=0,PR_C49_CLOSE_BLOCKED_DOS,
 PR_C49_CLOSE_BLOCKED_RESERVED_TAIL,PR_C49_CLOSE_BLOCKED_SIGNATURE_ERROR,
 PR_C49_CLOSE_BLOCKED_RETURN_TAIL,PR_C49_CLOSE_BLOCKED_MEMORY,
 PR_C49_CLOSE_BLOCKED_PC,PR_C49_CLOSE_ARGUMENT
} pr_c49_close_status;
typedef struct {
 unsigned valid;
 uint16_t original_segment,call_pc,return_pc,ax,bx;
} pr_c49_close_report;
/* One contiguous actual C49 caller input -> real C4F far CALL -> bounded
 * SYSTEM131C:1491 CPU prefix/near helper -> pre-DOS14A6 stop.
 * No read/DOS result, cleanup tail, callback, journal or state replacement.
 * Requires audited code and disjoint state/report/RAM. RAM<=1MiB; every
 * host span fits one64KiB segment; effective offsets wrap16bits.
 * Blocked outcomes preserve CPU/RAM; NULL arguments also preserve report.
 * No continuous read-to-selector, full Close/loader/count/playable credit. */
pr_c49_close_status pr_slot_a_c49_close_step(
 pr_slot_a_directory_state *state,pr_c49_close_report *report);
#endif
