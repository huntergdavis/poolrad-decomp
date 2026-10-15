#ifndef POOLRAD_REPLAY_SLOT_A_LOADER_0AEC_PREFIX_H
#define POOLRAD_REPLAY_SLOT_A_LOADER_0AEC_PREFIX_H
#include "poolrad/replay/slot_a_string_0425_abi.h"
typedef enum {
    PR_SLOT_A_LOADER_0AEC_STEPPED = 0,
    PR_SLOT_A_LOADER_0AEC_BLOCKED_FILE_OPEN,
    PR_SLOT_A_LOADER_0AEC_BLOCKED_PATH,
    PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY,
    PR_SLOT_A_LOADER_0AEC_BLOCKED_PC,
    PR_SLOT_A_LOADER_0AEC_ARGUMENT,
    PR_SLOT_A_LOADER_0AEC_BLOCKED_DEPENDENCY
} pr_slot_a_loader_0aec_status;
typedef pr_slot_a_string_0425_report pr_slot_a_loader_0aec_report;
/* One original instruction from actual 2A8E:19D7 near CALL :0AEC through
 * the reached nonzero-flag loader setup. Existing SYSTEM string and resident
 * 0425 adapters execute unchanged, using live call frames and real RETFs.
 * Blocks before :0C26 -> original 0802:025D, return :0C2B, including its
 * far-frame push. Alternate zero-flag paths at :0B81/:0BE7 stay blocked.
 * No allocator/record-file effects, snapshot bridge or identity/count credit.
 * Requires audited original/loaded code, nonoverlapping state/report/RAM,
 * RAM <= 1 MiB and spans within one 64 KiB segment. Individual pointers
 * wrap to 16 bits. Failed steps preserve CPU/RAM; NULL preserves report.
 * Existing helper logic AF=0 and DF contracts remain unchanged. */
pr_slot_a_loader_0aec_status pr_slot_a_loader_0aec_step(
    pr_slot_a_directory_state *state, pr_slot_a_loader_0aec_report *report);
#endif
