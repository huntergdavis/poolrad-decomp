#ifndef POOLRAD_REPLAY_SYSTEM_RESET_1410_ABI_H
#define POOLRAD_REPLAY_SYSTEM_RESET_1410_ABI_H
#include "poolrad/recovered/slot_a_directory_fullstate.h"
typedef enum {
    PR_SYSTEM_RESET_STEPPED=0, PR_SYSTEM_RESET_BLOCKED_DOS,
    PR_SYSTEM_RESET_BLOCKED_CLOSE, PR_SYSTEM_RESET_BLOCKED_CALLER,
    PR_SYSTEM_RESET_BLOCKED_MEMORY, PR_SYSTEM_RESET_BLOCKED_PC,
    PR_SYSTEM_RESET_ARGUMENT
} pr_system_reset_status;
typedef struct {
    unsigned valid;
    uint16_t original_segment, call_pc, callee_pc, return_pc;
    uint16_t ax, ds, dx;
} pr_system_reset_report;
/* One original CPU instruction from actual resident 031F -> SYSTEM 1410.
 * INT21 at1456 and Close1491 at1442 preserve CPU/RAM and report dependencies.
 * Independently seeded native1458 states audit the return fragment; this API
 * provides no DOS service, result injection, or composed execution across DOS.
 * Alternate Create entry 1419 is unsupported. Caller 0324 is a terminal stop.
 * Requires audited code identity, disjoint state/report/RAM, ram_size<=1MiB,
 * and each accessed host span within RAM and one 64KiB segment. Unsupported
 * instructions/spans preserve CPU/RAM; NULL arguments also preserve report.
 * Undefined XOR AF follows observed zero. No function/playable credit. */
pr_system_reset_status pr_system_reset_1410_step(
    pr_slot_a_directory_state *state, pr_system_reset_report *report);
#endif
