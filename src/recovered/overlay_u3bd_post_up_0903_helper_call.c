#include "poolrad/recovered/overlay_u3bd_post_up_0903_helper_call.h"

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & 0xFFFFFu;
}

static int read_byte(const pr_u3bd_post_up_0903_helper_call_state *s,
                     uint16_t offset, uint8_t *value)
{
    size_t at = physical(s->ss, offset);
    if (s->ram == NULL || at >= s->ram_size) return 0;
    *value = s->ram[at];
    return 1;
}

static int push_word(pr_u3bd_post_up_0903_helper_call_state *s, uint16_t value)
{
    uint16_t next_sp = (uint16_t)(s->sp - 2u);
    size_t lo = physical(s->ss, next_sp);
    size_t hi = physical(s->ss, (uint16_t)(next_sp + 1u));
    if (s->ram == NULL || lo >= s->ram_size || hi >= s->ram_size) return 0;
    s->sp = next_sp;
    s->ram[lo] = (uint8_t)value;
    s->ram[hi] = (uint8_t)(value >> 8);
    return 1;
}

pr_u3bd_post_up_0903_helper_call_status pr_u3bd_post_up_0903_helper_call(
    pr_u3bd_post_up_0903_helper_call_state *s,
    pr_u3bd_post_up_0903_helper_call_report *report)
{
    pr_u3bd_post_up_0903_helper_call_report local = {0};
    uint8_t value;

    if (s == NULL || s->cs != 0x2D14u || s->ip != 0x0903u)
        return PR_U3BD_POST_UP_0903_HELPER_ARGUMENT;

    if (!read_byte(s, (uint16_t)(s->bp - 0x0Du), &value))
        return PR_U3BD_POST_UP_0903_HELPER_BOUNDARY;
    local.x = value;
    s->ax = (uint16_t)((s->ax & 0xFF00u) | value);
    s->ip = 0x0906u;
    if (!push_word(s, s->ax)) return PR_U3BD_POST_UP_0903_HELPER_BOUNDARY;
    s->ip = 0x0907u;

    if (!read_byte(s, (uint16_t)(s->bp - 0x0Fu), &value))
        return PR_U3BD_POST_UP_0903_HELPER_BOUNDARY;
    local.y = value;
    s->ax = (uint16_t)((s->ax & 0xFF00u) | value);
    s->ip = 0x090Au;
    if (!push_word(s, s->ax)) return PR_U3BD_POST_UP_0903_HELPER_BOUNDARY;
    s->ip = 0x090Bu;

    if (!read_byte(s, (uint16_t)(s->bp - 1u), &value))
        return PR_U3BD_POST_UP_0903_HELPER_BOUNDARY;
    local.selector = value;
    s->ax = (uint16_t)((s->ax & 0xFF00u) | value);
    s->ip = 0x090Eu;
    if (!push_word(s, s->ax)) return PR_U3BD_POST_UP_0903_HELPER_BOUNDARY;
    s->ip = 0x090Fu;

    if (!push_word(s, s->cs)) return PR_U3BD_POST_UP_0903_HELPER_BOUNDARY;
    s->ip = 0x0910u;
    local.return_ip = 0x0913u;
    if (!push_word(s, local.return_ip))
        return PR_U3BD_POST_UP_0903_HELPER_BOUNDARY;
    s->ip = 0x047Bu;
    if (report != NULL) *report = local;
    return PR_U3BD_POST_UP_0903_HELPER_CALLEE_047B;
}
