#ifndef POOLRAD_REPLAY_SYSTEM_CLOSE_1491_PREFIX_H
#define POOLRAD_REPLAY_SYSTEM_CLOSE_1491_PREFIX_H
#include "poolrad/replay/slot_a_string_0356_abi.h"
typedef enum {
 PR_SYSTEM_CLOSE_STEPPED=0,PR_SYSTEM_CLOSE_BLOCKED_DOS,
 PR_SYSTEM_CLOSE_BLOCKED_RESERVED_TAIL,PR_SYSTEM_CLOSE_BLOCKED_SIGNATURE_ERROR,
 PR_SYSTEM_CLOSE_BLOCKED_RETURN_TAIL,PR_SYSTEM_CLOSE_BLOCKED_MEMORY,
 PR_SYSTEM_CLOSE_BLOCKED_PC,PR_SYSTEM_CLOSE_ARGUMENT
} pr_system_close_status;
typedef struct {
 unsigned valid;
 uint16_t original_segment,call_pc,return_pc,ax,bx;
} pr_system_close_report;
/* Reached loader C4F's SYSTEM131C:1491 entry -> pre-INT21h14A6.
 * Includes real near signature CALL/RET. No DOS result or cleanup tail.
 * Requires audited original/loaded code and disjoint state/report/RAM.
 * RAM<=1MiB; every host span fits one64KiB segment. Effective offsets wrap.
 * Blocked outcomes preserve CPU/RAM; NULL arguments also preserve report.
 * No complete Close, runtime, loader, I/O, heap, printer or playable credit. */
pr_system_close_status pr_system_close_1491_step(
 pr_slot_a_directory_state *state,pr_system_close_report *report);
#endif
