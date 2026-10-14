#include "poolrad/replay/slot_a_restore_view_join.h"

#include <string.h>

pr_slot_a_restore_view_status pr_slot_a_restore_view_prepare(
    const uint8_t *save, size_t save_size,
    const uint8_t *geo, size_t geo_size, uint8_t geo_id,
    pr_slot_a_restore_view_input *out)
{
    pr_slot_a_restore_view_input candidate;
    if (!save || !geo || !out)
        return PR_SLOT_A_RESTORE_VIEW_ARGUMENT;
    memset(&candidate, 0, sizeof candidate);
    if (pr_slot_a_native_restore(save, save_size, &candidate.world) !=
        PR_SLOT_A_NATIVE_OK)
        return PR_SLOT_A_RESTORE_VIEW_SAVE;
    if (pr_slot_a_view_prepare(&candidate.world, geo, geo_size, geo_id,
                              &candidate.view) != PR_SLOT_A_VIEW_OK)
        return PR_SLOT_A_RESTORE_VIEW_GEO;
    *out = candidate;
    return PR_SLOT_A_RESTORE_VIEW_OK;
}

pr_slot_a_restore_view_status pr_slot_a_restore_view_gate(
    const pr_slot_a_restore_view_input *input,
    pr_slot_a_restore_view_gate_report *report)
{
    if (!input || !report)
        return PR_SLOT_A_RESTORE_VIEW_ARGUMENT;
    *report = (pr_slot_a_restore_view_gate_report){0};
    report->blocked = (pr_slot_a_restore_view_dependency){
        0x6Fu, 0x18E5u, 0x0AF8u, 0x1491u, 0x18EAu,
        PR_SLOT_A_RESTORE_VIEW_LIVE_SAVE_HANDLE, -0x1C8, 0u};
    report->after_close_pc = 0x18EAu;
    report->after_party_prefix_pc = 0x19F7u;
    if (input->world.character_count != 0u) {
        report->queued_valid = 1u;
        report->queued = (pr_slot_a_restore_view_dependency){
            0x6Fu, 0x1965u, 0x0AF8u, 0x0B0Bu, 0x196Au,
            PR_SLOT_A_RESTORE_VIEW_LIVE_PASCAL_SOURCE_AND_DESTINATION,
            -0x214, 0x15E7u};
        report->after_party_prefix_pc = report->queued.call_pc;
    }
    return PR_SLOT_A_RESTORE_VIEW_BLOCKED_CLOSE_1491;
}
