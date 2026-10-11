/* Portable C for GAME.OVR unit 0x38 private helper +0x0000 and public
 * entries 93, 94, 96 (docs/re/OVERLAY_UNIT38_HELPER_0000.md). */
#include "poolrad/recovered/overlay_unit38_helper.h"

static void put(unsigned *out, unsigned value)
{
    if (out) *out = value;
}

/* +0x0000: cmp byte [bp+6],0; je clear; mov al,[0x6817]; cmp al,[bp+6];
 * jne done; clear: mov byte [0x6818],0; mov byte [0x6817],0; done: retf 2. */
pr_u38_status pr_u38_helper_0000(pr_u38h_state *state, uint8_t key, unsigned *paths)
{
    put(paths, 0);
    if (!state) return PR_U38_ARGUMENT;
    if (key == 0 || state->b6817 == key) {
        state->b6818 = 0;
        state->b6817 = 0;
        put(paths, PR_U38_PATH_EFFECT);
    }
    return PR_U38_OK;
}

/* +0x29CB: mov al,0x34; push ax; push cs; call +0x0000. */
pr_u38_status pr_u38_entry93(pr_u38h_state *state, unsigned *paths)
{
    return pr_u38_helper_0000(state, 0x34u, paths);
}

/* Shared body of +0x29D9 (mask 2) and +0x2A11 (mask 1): mov al,[0x6819];
 * and al,mask; or al,al; je done; mov al,0; push ax; push cs; call +0x0000. */
static pr_u38_status clear_if_bit(pr_u38h_state *state, uint8_t mask, unsigned *paths)
{
    unsigned inner = 0;
    pr_u38_status status;
    put(paths, 0);
    if (!state) return PR_U38_ARGUMENT;
    if ((state->b6819 & mask) == 0) return PR_U38_OK;
    status = pr_u38_helper_0000(state, 0u, &inner);
    put(paths, inner | PR_U38_PATH_SECOND);
    return status;
}

pr_u38_status pr_u38_entry94(pr_u38h_state *state, unsigned *paths)
{
    return clear_if_bit(state, 0x02u, paths);
}

pr_u38_status pr_u38_entry96(pr_u38h_state *state, unsigned *paths)
{
    return clear_if_bit(state, 0x01u, paths);
}
