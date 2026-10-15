#include "poolrad/recovered/overlay_u3bd_post_up_0913_zero_branch.h"

enum {
    X86_CF = 0x0001u, X86_PF = 0x0004u, X86_AF = 0x0010u,
    X86_ZF = 0x0040u, X86_SF = 0x0080u, X86_OF = 0x0800u,
    X86_ARITH = X86_CF | X86_PF | X86_AF | X86_ZF | X86_SF | X86_OF
};

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & 0xFFFFFu;
}

static int even_parity(uint8_t value)
{
    unsigned bit, ones = 0u;
    for (bit = 0u; bit < 8u; ++bit) ones += (value >> bit) & 1u;
    return (ones & 1u) == 0u;
}

pr_u3bd_post_up_0913_zero_branch_status pr_u3bd_post_up_0913_zero_branch(
    pr_u3bd_post_up_0913_zero_branch_state *s,
    pr_u3bd_post_up_0913_zero_branch_report *report)
{
    pr_u3bd_post_up_0913_zero_branch_report local = {0};
    size_t at;
    uint16_t bits = 0u;

    if (s == NULL || s->cs != 0x2D14u || s->ip != 0x0913u)
        return PR_U3BD_POST_UP_0913_ZERO_BRANCH_ARGUMENT;
    at = physical(s->ss, (uint16_t)(s->bp - 0x15u));
    if (s->ram == NULL || at >= s->ram_size)
        return PR_U3BD_POST_UP_0913_ZERO_BRANCH_BOUNDARY;

    local.result = (uint8_t)s->ax;
    s->ram[at] = local.result; /* +0913 MOV [BP-15],AL. */
    s->ip = 0x0916u;

    /* +0916 CMP byte [BP-15],0 defines every arithmetic flag. */
    if (even_parity(s->ram[at])) bits |= X86_PF;
    if (s->ram[at] == 0u) bits |= X86_ZF;
    if ((s->ram[at] & 0x80u) != 0u) bits |= X86_SF;
    s->flags = (uint16_t)((s->flags & (uint16_t)~X86_ARITH) | bits);
    s->ip = 0x091Au;

    local.next_ip = s->ram[at] == 0u ? 0x0951u : 0x091Cu;
    s->ip = local.next_ip; /* +091A JE +0951. */
    if (report != NULL) *report = local;
    return PR_U3BD_POST_UP_0913_ZERO_BRANCH_OK;
}
