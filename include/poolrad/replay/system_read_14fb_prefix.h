#ifndef POOLRAD_REPLAY_SYSTEM_READ_14FB_PREFIX_H
#define POOLRAD_REPLAY_SYSTEM_READ_14FB_PREFIX_H
#include "poolrad/replay/slot_a_string_0356_abi.h"
typedef enum {
 PR_SYSTEM_READ_STEPPED=0,PR_SYSTEM_READ_BLOCKED_DOS,
 PR_SYSTEM_READ_BLOCKED_ZERO_TAIL,PR_SYSTEM_READ_BLOCKED_SIGNATURE_ERROR,
 PR_SYSTEM_READ_BLOCKED_ERROR_TAIL,PR_SYSTEM_READ_BLOCKED_MEMORY,
 PR_SYSTEM_READ_BLOCKED_PC,PR_SYSTEM_READ_ARGUMENT
} pr_system_read_status;
typedef struct {
 unsigned valid;
 uint16_t original_segment,call_pc,return_pc,ax,bx,cx,ds,dx;
} pr_system_read_report;
/* Reached SYSTEM 131C:14FB entry -> pre-INT21h 1529, including the valid
 * 14B6 signature helper and real near CALL/RET. No DOS result or tail.
 * Requires audited original/loaded code and disjoint state/report/RAM.
 * RAM<=1MiB; spans fit one 64KiB segment; effective offsets wrap 16bits.
 * Logic AF follows observed zero. MUL follows captured DOSBox policy:
 * SF/AF retained, ZF from low word, PF from full product, CF/OF from high
 * word. These undefined flags are not a portable x86 architectural claim.
 * Blocked outcomes preserve CPU/RAM; NULL arguments also preserve report.
 * No complete runtime, loader, I/O, heap, printer or playable credit. */
pr_system_read_status pr_system_read_14fb_step(
 pr_slot_a_directory_state *state,pr_system_read_report *report);
#endif
