#ifndef POOLRAD_REPLAY_SLOT_A_RESTORE_VIEW_JOIN_H
#define POOLRAD_REPLAY_SLOT_A_RESTORE_VIEW_JOIN_H

#include "poolrad/replay/slot_a_static_view.h"

/* Prepared file-derived inputs, not an initialized native gameplay state.
 * Preparation copies all save transfers and derives static map context. */
typedef struct {
    pr_slot_a_native_state world;
    pr_slot_a_view_context view;
} pr_slot_a_restore_view_input;

typedef enum {
    PR_SLOT_A_RESTORE_VIEW_OK = 0,
    PR_SLOT_A_RESTORE_VIEW_ARGUMENT,
    PR_SLOT_A_RESTORE_VIEW_SAVE,
    PR_SLOT_A_RESTORE_VIEW_GEO,
    PR_SLOT_A_RESTORE_VIEW_BLOCKED_CLOSE_1491
} pr_slot_a_restore_view_status;

typedef enum {
    PR_SLOT_A_RESTORE_VIEW_LIVE_SAVE_HANDLE,
    PR_SLOT_A_RESTORE_VIEW_LIVE_PASCAL_SOURCE_AND_DESTINATION
} pr_slot_a_restore_view_requirement;

/* Original code addresses, not relocated runtime segments. The caller must
 * provide real live arguments; this contract contains no invented handle,
 * local stack frame, private literal, or completed-callee token. */
typedef struct {
    uint16_t caller_unit;
    uint16_t call_pc;
    uint16_t callee_segment;
    uint16_t callee_pc;
    uint16_t return_pc;
    pr_slot_a_restore_view_requirement requirement;
    int16_t argument_bp_offset; /* SS:BP-relative handle or destination */
    uint16_t source_cs_offset;  /* Pascal source only; 0 for the close */
} pr_slot_a_restore_view_dependency;

typedef struct {
    pr_slot_a_restore_view_dependency blocked;
    uint16_t after_close_pc;
    unsigned queued_valid;
    pr_slot_a_restore_view_dependency queued;
    uint16_t after_party_prefix_pc;
} pr_slot_a_restore_view_gate_report;

/* Compose native_restore -> view_prepare using the same local candidate.
 * geo is one already unpacked block with its actual directory ID. On any
 * rejection out remains byte-identical; inputs and out must not overlap.
 * Success prepares data only: no native loader or view call has run. */
pr_slot_a_restore_view_status pr_slot_a_restore_view_prepare(
    const uint8_t *save, size_t save_size,
    const uint8_t *geo, size_t geo_size, uint8_t geo_id,
    pr_slot_a_restore_view_input *out);

/* Requires successfully prepared, unchanged inputs. The first unsupported
 * call after the ten world transfers is the save-handle close at +18E5.
 * Report it and queue the next party-loop call for a nonzero count; neither
 * call nor the intervening translated steps execute. Prepared state remains
 * unchanged. report must not overlap input. NULL arguments have no effects. */
pr_slot_a_restore_view_status pr_slot_a_restore_view_gate(
    const pr_slot_a_restore_view_input *input,
    pr_slot_a_restore_view_gate_report *report);

#endif
