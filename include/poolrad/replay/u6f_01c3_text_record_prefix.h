#ifndef POOLRAD_REPLAY_U6F_01C3_TEXT_RECORD_PREFIX_H
#define POOLRAD_REPLAY_U6F_01C3_TEXT_RECORD_PREFIX_H

#include "poolrad/recovered/slot_a_directory_fullstate.h"

typedef enum {
    PR_U6F_01C3_STEPPED = 0,
    PR_U6F_01C3_AT_04F7,
    PR_U6F_01C3_TRANSFER,
    PR_U6F_01C3_BLOCKED_MEMORY,
    PR_U6F_01C3_BLOCKED_PC,
    PR_U6F_01C3_ARGUMENT
} pr_u6f_01c3_status;

/* U6F [01C3,01E3):32 bytes/14 starts plus unchanged SYSTEM B0B/B98
 * instruction adapters. One instruction per step; REP remains atomic.
 * AT_04F7 stops BEFORE the actual CALL at01E3: no text-record init,
 * open, DOS/UI event, later retry/list or outer return is supplied.
 * Real far frames, raw pushed words, live SS locals, source saturation
 * and forward alias-copy effects remain in guest RAM. A helper RETF
 * through a changed frame commits its real TRANSFER, never forced flow.
 *
 * Requires audited code/data identity (U6F2A8E, SYSTEM131C), disjoint
 * exclusive state/RAM and inherited bounded spans: RAM<=1MiB, operands
 * fit a segment and RAM without A20 wrap. Unsupported spans/DF/PCs
 * preserve CPU/RAM for that step. Helpers normalize undefined logic AF
 * to0; all defined flags and16CPU fields including FS/GS are retained.
 * Entry provenance is a caller obligation; seeding01C3/01E3 proves none.
 * No whole-function, native or strict credit. */
pr_u6f_01c3_status pr_u6f_01c3_text_record_step(pr_slot_a_directory_state *state);

#endif
