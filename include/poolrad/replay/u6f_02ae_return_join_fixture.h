#ifndef POOLRAD_REPLAY_U6F_02AE_RETURN_JOIN_FIXTURE_H
#define POOLRAD_REPLAY_U6F_02AE_RETURN_JOIN_FIXTURE_H

#include "poolrad/replay/u6f_01b8_entry_abi.h"

/* Report order/mask bits: AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS. */
typedef struct {
    uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags;
} pr_u6f_02ae_join_cpu14;

typedef struct {
    pr_u6f_02ae_join_cpu14 before, after;
    uint16_t changed_mask;
    uint16_t fs_before,fs_after,gs_before,gs_after;
    uint16_t frame_ss,frame_offset,saved_caller_bp,return_ip,return_cs;
    size_t ram_changed_bytes;
} pr_u6f_02ae_return_join_report;

typedef enum {
    PR_U6F_02AE_JOINED_01C3 = 0,
    PR_U6F_02AE_JOIN_TRANSFER,
    PR_U6F_02AE_JOIN_BLOCKED_MEMORY,
    PR_U6F_02AE_JOIN_BLOCKED_PC,
    PR_U6F_02AE_JOIN_ARGUMENT
} pr_u6f_02ae_return_join_status;

/* Diagnostic fixture for ONLY the actual02AE [032B,032F) return tail:
 * MOV SP,BP; POP BP; RETF. Reuses the unchanged merged entry/02AE stepper.
 * Input must be an audited state at2A8E:032B; prior DOS/UI completion and
 * entry provenance are caller obligations, not manufactured by this API.
 *
 * Stages all three read-only-RAM steps before committing CPU/report.
 * Blocked/argument outcomes preserve CPU, RAM and report for the entire
 * fixture. Successful join honors the actual saved BP/return frame;
 * a changed live target commits TRANSFER, not a fabricated01C3 join.
 * Report mask covers14 fields; FS/GS remain explicit. RAM changed bytes
 * is0 for this store-free tail, not for the preceding whole callee.
 * Requires disjoint/exclusive state/report/RAM, RAM<=1MiB and inherited
 * bounded segment/linear spans. No source instruction is retranslated,
 * no DOS/UI event supplied and no native/whole/strict credit claimed. */
pr_u6f_02ae_return_join_status pr_u6f_02ae_return_join_fixture(
    pr_slot_a_directory_state *state, pr_u6f_02ae_return_join_report *report);

#endif
