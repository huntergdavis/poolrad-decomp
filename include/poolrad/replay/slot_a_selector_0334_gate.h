#ifndef POOLRAD_REPLAY_SLOT_A_SELECTOR_0334_GATE_H
#define POOLRAD_REPLAY_SLOT_A_SELECTOR_0334_GATE_H
#include "poolrad/replay/slot_a_string_0356_abi.h"
typedef enum {
 PR_SELECTOR_0334_STEPPED=0,PR_SELECTOR_0334_BLOCKED_LOADER,
 PR_SELECTOR_0334_BLOCKED_MEMORY,PR_SELECTOR_0334_BLOCKED_PC,
 PR_SELECTOR_0334_ARGUMENT
} pr_selector_0334_status;
typedef struct {
 unsigned valid;
 uint16_t return_segment,return_pc;
} pr_selector_0334_report;
/* Actual independently seeded1026:0334 post-IOResult input -> full AX
 * DS49CE store, caller decisions, local AL, real RETF4 -> loader2A8E:CAF.
 * Stops before loader branch; no IOResult/DOS output, journal/callback
 * or live-state replacement. Requires audited code/real frame and disjoint
 * state/report/RAM; RAM<=1MiB, spans within one64KiB segment, offsets wrap.
 * Blocked outcomes preserve CPU/RAM; NULL arguments also preserve report.
 * No continuous read/selector, full-C, count, heap/printer/playable credit. */
pr_selector_0334_status pr_slot_a_selector_0334_step(
 pr_slot_a_directory_state *state,pr_selector_0334_report *report);
#endif
