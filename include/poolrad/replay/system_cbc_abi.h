#ifndef POOLRAD_REPLAY_SYSTEM_CBC_ABI_H
#define POOLRAD_REPLAY_SYSTEM_CBC_ABI_H
#include "poolrad/replay/system_b98_abi.h"
/* One original CBC instruction, its B57/B98/B25 dependency instruction,
 * or the resident 1026:03FC far CALL. RETF 8 reads the real return frame.
 * Same host span/DF/alias/code-identity contract as SYSTEM B98. All blocked
 * outcomes preserve CPU/RAM at the current instruction. No game credit. */
pr_system_b98_status pr_system_cbc_step(pr_slot_a_directory_state *state);
#endif
