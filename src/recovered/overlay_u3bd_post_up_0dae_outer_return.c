#include "poolrad/recovered/overlay_u3bd_post_up_0dae_outer_return.h"

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & 0xFFFFFu;
}

static int get_word(const pr_u3bd_post_up_0dae_state *s,
                    uint16_t offset, uint16_t *out)
{
    size_t lo = physical(s->ss, offset);
    size_t hi = physical(s->ss, (uint16_t)(offset + 1u));
    if (s->ram == NULL || lo >= s->ram_size || hi >= s->ram_size) return 0;
    *out = (uint16_t)(s->ram[lo] | ((uint16_t)s->ram[hi] << 8));
    return 1;
}

pr_u3bd_post_up_0dae_status pr_u3bd_post_up_0dae_outer_return(
    pr_u3bd_post_up_0dae_state *s)
{
    uint16_t saved_bp, return_ip, return_cs;

    if (s == NULL || s->ip != 0x0DAEu)
        return PR_U3BD_POST_UP_0DAE_ARGUMENT;

    /* +0DAE MOV SP,BP. */
    s->sp = s->bp;
    s->ip = 0x0DB0u;

    /* +0DB0 POP BP. */
    if (!get_word(s, s->sp, &saved_bp))
        return PR_U3BD_POST_UP_0DAE_BOUNDARY;
    s->bp = saved_bp;
    s->sp = (uint16_t)(s->sp + 2u);
    s->ip = 0x0DB1u;

    /* +0DB1 RETF 6: pop far destination, then discard three words. */
    if (!get_word(s, s->sp, &return_ip) ||
        !get_word(s, (uint16_t)(s->sp + 2u), &return_cs))
        return PR_U3BD_POST_UP_0DAE_BOUNDARY;
    s->sp = (uint16_t)(s->sp + 10u);
    s->ip = return_ip;
    s->cs = return_cs;
    return PR_U3BD_POST_UP_0DAE_OK;
}
