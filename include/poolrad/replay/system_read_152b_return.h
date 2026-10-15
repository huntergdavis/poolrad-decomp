#ifndef POOLRAD_REPLAY_SYSTEM_READ_152B_RETURN_H
#define POOLRAD_REPLAY_SYSTEM_READ_152B_RETURN_H
#include "poolrad/replay/slot_a_string_0356_abi.h"
typedef enum {
 PR_SYSTEM_READ_RETURN_STEPPED=0,PR_SYSTEM_READ_RETURN_BLOCKED_LOADER,
 PR_SYSTEM_READ_RETURN_BLOCKED_IO_CALLER,PR_SYSTEM_READ_RETURN_BLOCKED_DIVIDE,
 PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY,PR_SYSTEM_READ_RETURN_BLOCKED_PC,
 PR_SYSTEM_READ_RETURN_ARGUMENT
} pr_system_read_return_status;
typedef struct {
 unsigned valid;
 uint16_t return_segment,return_pc;
} pr_system_read_return_report;
/* Independent actual post-DOS131C:152B -> real RETF0E -> loader2A8E:C49.
 * Separate later IOResult04A2 input -> real RETF -> resident1026:0334.
 * No state replacement, DOS output injection or join through the native gap.
 * Requires audited code, observed frames, disjoint state/report/RAM.
 * RAM<=1MiB; host spans fit one64KiB segment; effective offsets wrap16bits.
 * Logic AF follows observed zero. Undefined DIV flags use captured DOSBox:
 * AF/SF/OF=0; ZF=(remainder==0 && quotient odd); CF=(remainder%4 is1 or2);
 * PF=even parity of quotient XOR remainder (all16bits). No general x86 claim.
 * Divide by zero/quotient overflow stops before any CPU/RAM effect.
 * Blocked outcomes preserve CPU/RAM; NULL arguments also preserve report.
 * No full runtime/loader, DOS/sidecar/heap/printer or playable credit. */
pr_system_read_return_status pr_system_read_152b_step(
 pr_slot_a_directory_state *state,pr_system_read_return_report *report);
#endif
