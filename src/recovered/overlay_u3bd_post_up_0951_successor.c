#include "poolrad/recovered/overlay_u3bd_post_up_0951_successor.h"

pr_u3bd_post_up_0938_control_status pr_u3bd_post_up_0951_successor(
    pr_u3bd_post_up_0938_control_state *state,
    pr_u3bd_post_up_0938_control_report *report)
{
    if (state == NULL || state->ip != 0x0951u)
        return PR_U3BD_POST_UP_0938_CONTROL_ARGUMENT;

    /* The reviewed +0938 model starts its shared instruction body by
     * setting IP to +0951. That assignment is idempotent at this entry. */
    return pr_u3bd_post_up_0938_control(state, report);
}
