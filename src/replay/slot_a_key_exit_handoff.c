#include "poolrad/replay/slot_a_key_exit_handoff.h"

#include <string.h>

static uint16_t read_word(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

pr_slot_a_key_exit_status pr_slot_a_key_exit_handoff(
    const pr_slot_a_script_dependency *dependency,
    const pr_slot_a_script_entry_image *entry,
    unsigned max_steps, pr_rec_ecl_loader_state *state,
    pr_u1c_exit_env *env, pr_slot_a_key_exit_report *report)
{
    size_t d, frame;
    pr_slot_a_ecl_bind_status bind;
    if (!report) return PR_SLOT_A_KEY_EXIT_ARGUMENT;
    memset(report, 0, sizeof *report);
    if (!dependency || !entry || !entry->ram || !state || !env)
        return PR_SLOT_A_KEY_EXIT_ARGUMENT;
    if (dependency->unit != 0x1cu || dependency->call_pc != 0x3458u ||
        dependency->callee_pc != 0x3171u || dependency->return_pc != 0x345bu ||
        dependency->entry != PR_SLOT_A_FIRST_EXIT_ENTRY)
        return PR_SLOT_A_KEY_EXIT_DEPENDENCY;
    d = (size_t)entry->ds << 4;
    frame = ((size_t)entry->ss << 4) + entry->sp;
    if (entry->sp > 0xfffau || d > entry->ram_size ||
        0x49e5u > entry->ram_size - d || frame > entry->ram_size ||
        6u > entry->ram_size - frame ||
        read_word(entry->ram + d + 0x49e3u) != dependency->entry ||
        read_word(entry->ram + frame) != dependency->return_pc ||
        read_word(entry->ram + frame + 2u) != entry->caller_cs ||
        read_word(entry->ram + frame + 4u) != dependency->entry)
        return PR_SLOT_A_KEY_EXIT_FRAME;
    report->runner_called = 1u;
    bind = pr_slot_a_first_exit_run_ram(entry->ram, entry->ram_size, entry->ds,
                                        max_steps, state, env, &report->script);
    if (bind != PR_SLOT_A_ECL_BIND_OK) return PR_SLOT_A_KEY_EXIT_BIND;
    if (report->script.run_status != PR_REC_ECL_RUN_OK ||
        !report->script.exit_reached || report->script.exit_status != PR_U1C_EXIT_OK)
        return PR_SLOT_A_KEY_EXIT_BLOCKED;
    report->projected_return_pc = dependency->return_pc;
    return PR_SLOT_A_KEY_EXIT_PROJECTED_RETURN;
}
