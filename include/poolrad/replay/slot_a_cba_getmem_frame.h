#ifndef POOLRAD_REPLAY_SLOT_A_CBA_GETMEM_FRAME_H
#define POOLRAD_REPLAY_SLOT_A_CBA_GETMEM_FRAME_H
#include "poolrad/replay/system_getmem_23f_abi.h"
/* Two INDEPENDENT live inputs: CBA far CALL -> SYSTEM prefix -> pre-core245;
 * actual first core return248 -> wrapper RETF2 -> pre-callerCBF. Reuses the
 * merged SYSTEM23F wrapper, but never enters core308 or supplies its output.
 * Only CBA,23F,240,242,245,248,249,24B,24E,251,E2,CBF are accepted.
 * CBF returns BLOCKED_CALLER;245 reports original AF8:308 near call/return248;
 * E2 blocks runtime error. Unknown real returns remain BLOCKED_PC.
 * Requires original/loaded code identity, actual CPU/RAM, and nonoverlapping
 * state/report/RAM. RAM <=1 MiB; spans fit one segment and host image;
 * effective 16-bit offsets wrap, unsupported multi-byte spans block atomically.
 * Blocked CPU/RAM preserved; NULL also preserves report. No allocator effects,
 * snapshot bridge, new function identity/count or continuous replay claim. */
pr_system_getmem_status pr_slot_a_cba_getmem_step(
 pr_slot_a_directory_state *state,pr_system_getmem_report *report);
#endif
