#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_INSERT_SS_PROBE_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_INSERT_SS_PROBE_H

#include <stdint.h>

#include "poolrad/recovered/overlay_u6f_party_insert_1446_dos.h"

typedef enum {
    PR_U6F_SS_PROBE_OK = 0,
    PR_U6F_SS_PROBE_ARGUMENT,
    PR_U6F_SS_PROBE_OTHER_PATH,
    PR_U6F_SS_PROBE_LOOP_LIMIT
} pr_u6f_ss_probe_status;

typedef struct {
    uint8_t selected_slot;
    uint8_t first_marked;
    uint8_t incoming_marked;
    unsigned increments;
} pr_u6f_ss_probe_report;

/* Bounded source-order probe from +0x144C through +0x1563. The caller
 * supplies the callee BP after PUSH BP / MOV BP, with the far argument at
 * SS:[BP+6/+8]. This covers only the path with one existing head, a null
 * head link before append, and a null incoming link during marking. It
 * models FillChar's eight-byte result and membership's low-byte decision,
 * but not either helper's transient stack writes or the later window/u9B
 * effects. All modeled locals, globals, and records share the supplied RAM.
 * OTHER_PATH may follow source-order writes and does not roll them back. */
pr_u6f_ss_probe_status pr_u6f_insert_ss_two_node_probe(
    const pr_u6f_insert_dos_memory *memory, uint16_t ss, uint16_t bp,
    pr_u6f_ss_probe_report *report);

#endif
