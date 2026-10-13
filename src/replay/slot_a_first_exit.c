#include "poolrad/replay/slot_a_first_exit.h"

#include "poolrad/recovered/overlay_u1c_and_or_0def.h"

#include <string.h>

typedef struct {
    pr_u1c_exit_env *env;
    pr_slot_a_first_exit_report *report;
} route_ctx;

static uint16_t word(const uint8_t *at)
{
    return (uint16_t)((uint16_t)at[0] | ((uint16_t)at[1] << 8));
}

static pr_rec_ecl_handler_status route_handler(pr_rec_ecl_loader_state *state,
                                               void *opaque)
{
    route_ctx *ctx = opaque;
    pr_slot_a_first_exit_report *r = ctx->report;
    if (r->callback_count >= PR_SLOT_A_FIRST_EXIT_TRACE_MAX)
        return PR_REC_ECL_HANDLER_BOUNDS;
    r->callback_pc[r->callback_count++] = state->pc;
    switch (state->latch) {
    case 0x09u: return pr_rec_ecl_op_save(state, NULL);
    case 0x03u: return pr_rec_ecl_op_compare(state);
    case 0x16u: case 0x17u: case 0x18u:
    case 0x19u: case 0x1Au: case 0x1Bu:
        return pr_rec_ecl_op_if(state);
    case 0x01u: return pr_rec_ecl_op_goto(state);
    case 0x2Fu: return pr_u1c_op_and_or(state, NULL, NULL);
    case 0x00u:
        r->exit_reached = 1u;
        r->exit_status = pr_u1c_op_exit(state, ctx->env, &r->exit_report);
        switch (r->exit_status) {
        case PR_U1C_EXIT_OK: return PR_REC_ECL_HANDLER_OK;
        case PR_U1C_EXIT_HOOK: return PR_REC_ECL_HANDLER_HOOK;
        case PR_U1C_EXIT_ARGUMENT: return PR_REC_ECL_HANDLER_ARGUMENT;
        default: return PR_REC_ECL_HANDLER_UNRESOLVED;
        }
    default: return PR_REC_ECL_HANDLER_UNRESOLVED;
    }
}

pr_slot_a_ecl_bind_status pr_slot_a_first_exit_run_ram(
    const uint8_t *ram, size_t ram_size, uint16_t ds,
    unsigned max_steps, pr_rec_ecl_loader_state *state,
    pr_u1c_exit_env *env, pr_slot_a_first_exit_report *report)
{
    pr_rec_ecl_loader_state bound;
    pr_u1c_exit_env live_env;
    pr_rec_ecl_dispatch_table table = {0};
    route_ctx ctx;
    const uint8_t *d;
    unsigned i;
    pr_slot_a_ecl_bind_status status;
    if (!report) return PR_SLOT_A_ECL_BIND_ARGUMENT;
    memset(report, 0, sizeof *report);
    report->bind_status = PR_SLOT_A_ECL_BIND_ARGUMENT;
    report->run_status = PR_REC_ECL_RUN_ARGUMENT;
    if (!ram || !state || !env) return PR_SLOT_A_ECL_BIND_ARGUMENT;
    status = pr_slot_a_ecl_bind_ram(ram, ram_size, ds,
                                    PR_SLOT_A_FIRST_EXIT_ENTRY, &bound);
    report->bind_status = status;
    if (status != PR_SLOT_A_ECL_BIND_OK) return status;
    d = ram + ((size_t)ds << 4);
    memset(&live_env, 0, sizeof live_env);
    live_env.b4430 = d[0x4430u];
    live_env.w4436 = word(d + 0x4436u);
    live_env.w4438 = word(d + 0x4438u);
    live_env.w5d92 = word(d + 0x5D92u);
    live_env.w5d94 = word(d + 0x5D94u);
    live_env.b84de = d[0x84DEu];

    ctx.env = &live_env;
    ctx.report = report;
    table.ctx = &ctx;
    table.handler[0x09u] = route_handler;
    table.handler[0x03u] = route_handler;
    table.handler[0x01u] = route_handler;
    table.handler[0x2Fu] = route_handler;
    table.handler[0x00u] = route_handler;
    for (i = 0x16u; i <= 0x1Bu; ++i)
        table.handler[i] = route_handler;
    report->run_status = pr_rec_ecl_run(&bound, &table,
                                       PR_SLOT_A_FIRST_EXIT_ENTRY,
                                       max_steps, &report->steps,
                                       &report->handler_status);
    *state = bound;
    *env = live_env;
    return PR_SLOT_A_ECL_BIND_OK;
}
