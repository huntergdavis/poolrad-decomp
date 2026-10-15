#include "poolrad/recovered/overlay_u3bd_post_up_outer_loop_gate.h"

enum {
    X86_CF = 0x0001u,
    X86_PF = 0x0004u,
    X86_AF = 0x0010u,
    X86_ZF = 0x0040u,
    X86_SF = 0x0080u,
    X86_OF = 0x0800u,
    X86_ARITH = X86_CF | X86_PF | X86_AF | X86_ZF | X86_SF | X86_OF
};

static uint16_t word_at(const uint8_t *ss, uint16_t at)
{
    return (uint16_t)(ss[at] | ((uint16_t)ss[(uint16_t)(at + 1u)] << 8));
}

static int even_parity(uint8_t value)
{
    unsigned bit, ones = 0u;
    for (bit = 0u; bit < 8u; ++bit)
        ones += (value >> bit) & 1u;
    return (ones & 1u) == 0u;
}

static uint16_t cmp_flags(uint16_t old, uint16_t lhs, uint16_t rhs)
{
    uint16_t diff = (uint16_t)(lhs - rhs);
    uint16_t bits = 0u;
    if (lhs < rhs) bits |= X86_CF;
    if (even_parity((uint8_t)diff)) bits |= X86_PF;
    if (((lhs ^ rhs ^ diff) & 0x10u) != 0u) bits |= X86_AF;
    if (diff == 0u) bits |= X86_ZF;
    if ((diff & 0x8000u) != 0u) bits |= X86_SF;
    if (((lhs ^ rhs) & (lhs ^ diff) & 0x8000u) != 0u)
        bits |= X86_OF;
    return (uint16_t)((old & (uint16_t)~X86_ARITH) | bits);
}

pr_u3bd_post_up_outer_loop_gate_status pr_u3bd_post_up_outer_loop_gate(
    pr_u3bd_post_up_outer_loop_gate_state *state,
    pr_u3bd_post_up_outer_loop_gate_report *report)
{
    uint16_t lhs_at, rhs_at, lhs_hi, rhs_hi, lhs, rhs;

    if (state == NULL || state->ss == NULL)
        return PR_U3BD_POST_UP_OUTER_LOOP_GATE_ARGUMENT;
    lhs_at = (uint16_t)(state->bp - 8u);
    rhs_at = (uint16_t)(state->bp - 11u);
    lhs_hi = (uint16_t)(lhs_at + 1u);
    rhs_hi = (uint16_t)(rhs_at + 1u);
    if ((size_t)lhs_at >= state->ss_size ||
        (size_t)lhs_hi >= state->ss_size ||
        (size_t)rhs_at >= state->ss_size ||
        (size_t)rhs_hi >= state->ss_size)
        return PR_U3BD_POST_UP_OUTER_LOOP_GATE_BOUNDARY;

    lhs = word_at(state->ss, lhs_at);
    rhs = word_at(state->ss, rhs_at);
    state->ax = lhs;
    state->flags = cmp_flags(state->flags, lhs, rhs);
    if (report != NULL) {
        report->lhs = lhs;
        report->rhs = rhs;
        report->next_ip = lhs == rhs ? 0x0319u : 0x026Eu;
    }
    return PR_U3BD_POST_UP_OUTER_LOOP_GATE_OK;
}
