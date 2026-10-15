#include "poolrad/recovered/overlay_u3bd_post_up_0319_exit.h"

static int read_word(const pr_u3bd_post_up_0319_exit_state *state,
                     uint16_t at, uint16_t *value)
{
    uint16_t hi = (uint16_t)(at + 1u);
    if (state->ss == NULL || (size_t)at >= state->ss_size ||
        (size_t)hi >= state->ss_size)
        return 0;
    *value = (uint16_t)(state->ss[at] | ((uint16_t)state->ss[hi] << 8));
    return 1;
}

pr_u3bd_post_up_0319_exit_status pr_u3bd_post_up_0319_exit(
    pr_u3bd_post_up_0319_exit_state *state,
    pr_u3bd_post_up_0319_exit_report *report)
{
    uint16_t saved_bp, return_ip, return_cs;

    if (state == NULL)
        return PR_U3BD_POST_UP_0319_EXIT_ARGUMENT;

    state->sp = state->bp; /* +0319 MOV SP,BP */
    if (!read_word(state, state->sp, &saved_bp))
        return PR_U3BD_POST_UP_0319_EXIT_BOUNDARY;
    state->bp = saved_bp; /* +031B POP BP */
    state->sp = (uint16_t)(state->sp + 2u);

    /* +031C RETF 8: both return words must be present before committing
     * the far transfer. The boundary leaves the preceding POP effects. */
    if (!read_word(state, state->sp, &return_ip) ||
        !read_word(state, (uint16_t)(state->sp + 2u), &return_cs))
        return PR_U3BD_POST_UP_0319_EXIT_BOUNDARY;
    state->ip = return_ip;
    state->cs = return_cs;
    state->sp = (uint16_t)(state->sp + 12u);

    if (report != NULL) {
        report->saved_bp = saved_bp;
        report->return_ip = return_ip;
        report->return_cs = return_cs;
        report->exit_sp = state->sp;
    }
    return PR_U3BD_POST_UP_0319_EXIT_OK;
}
