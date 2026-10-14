#include "poolrad/replay/slot_a_key_script_join.h"

pr_slot_a_key_join_status pr_slot_a_key_script_join(
    pr_slot_a_key_return_state *returned, uint8_t returned_al,
    const pr_slot_a_key_join_gate *gate, pr_slot_a_key_join_report *report)
{
    pr_slot_a_key_gate_state next;
    pr_slot_a_key_gate_status status;
    uint16_t entry;
    if (!returned || !gate || !report)
        return PR_SLOT_A_KEY_JOIN_ARGUMENT;
    *report = (pr_slot_a_key_join_report){0};
    report->next_pc = 0x339Au;
    if (pr_slot_a_key_return_step(returned, returned_al) !=
        PR_SLOT_A_KEY_RETURN_OK)
        return PR_SLOT_A_KEY_JOIN_ARGUMENT;

    report->return_applied = 1u;
    report->next_pc = 0x33BDu;
    next = (pr_slot_a_key_gate_state){gate->window1, gate->window1_size,
        returned->local_key_bp_minus_1, gate->exit_flag_49ff,
        gate->script_entry_49e3};
    status = pr_slot_a_key_gate_next(&next, &entry);
    if (status == PR_SLOT_A_KEY_GATE_ARGUMENT)
        return PR_SLOT_A_KEY_JOIN_ARGUMENT;
    if (status == PR_SLOT_A_KEY_GATE_OUTSIDE_SLICE)
        return PR_SLOT_A_KEY_JOIN_OUTSIDE_SLICE;

    report->dependency = (pr_slot_a_script_dependency){
        0x1Cu, 0x3458u, 0x3171u, 0x345Bu, entry};
    report->dependency_valid = 1u;
    report->next_pc = report->dependency.call_pc;
    return PR_SLOT_A_KEY_JOIN_BLOCKED_SCRIPT_3171;
}
