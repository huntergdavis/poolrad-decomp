#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_PARTY_INSERT_1446_SS_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_PARTY_INSERT_1446_SS_H

#include <stdint.h>

#include "poolrad/recovered/overlay_u6f_party_insert_1446_dos.h"

typedef struct {
    uint16_t cs;
    uint16_t ss;
    uint16_t entry_sp;  /* At +0x1446, after the caller's near CALL. */
    uint16_t caller_bp;
} pr_u6f_insert_ss_frame;

typedef struct {
    size_t append_nodes;
    size_t mark_nodes;
    uint8_t selected_slot;
    uint16_t window_word;
    uint16_t callee_bp;
    uint16_t transfer_sp;
} pr_u6f_insert_ss_report;

typedef enum {
    PR_U6F_INSERT_SS_AT_U9B_TRANSFER = 0,
    PR_U6F_INSERT_SS_ARGUMENT
} pr_u6f_insert_ss_status;

/* Source-order translation of u6F:+0x1446 through the +0x1574 far-call
 * transfer. RAM includes loaded CS code/set data, DS globals, SS stack and
 * locals, and ES records. The caller has already performed its near CALL;
 * its return IP, pushed CS word, and incoming far argument are in SS RAM.
 * This entry models both SYSTEM helpers' call-stack writes and their proved
 * behavior. It stops after pushing the u9B call frame, before executing
 * the overlay thunk or u9B callee. Finite valid lists have no host cap. */
pr_u6f_insert_ss_status pr_u6f_party_insert_1446_ss(
    const pr_u6f_insert_dos_memory *memory,
    const pr_u6f_insert_ss_frame *frame,
    pr_u6f_insert_ss_report *report);

#endif
