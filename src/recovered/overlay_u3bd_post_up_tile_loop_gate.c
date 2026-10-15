#include "poolrad/recovered/overlay_u3bd_post_up_tile_loop_gate.h"

#include <string.h>

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
    unsigned bit;
    unsigned ones = 0u;
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

pr_u3bd_post_up_tile_loop_gate_status pr_u3bd_post_up_tile_loop_gate(
    pr_u3bd_post_up_tile_loop_gate_state *state,
    pr_u3bd_post_up_tile_loop_gate_report *report)
{
    pr_u3bd_post_up_tile_loop_gate_report local;
    uint16_t counter_at, lhs_at, rhs_at;
    uint16_t lhs_hi, rhs_hi;

    if (state == NULL || state->ss == NULL)
        return PR_U3BD_POST_UP_TILE_LOOP_GATE_ARGUMENT;
    counter_at = (uint16_t)(state->bp - 9u);
    lhs_at = (uint16_t)(state->bp - 6u);
    rhs_at = (uint16_t)(state->bp - 13u);
    lhs_hi = (uint16_t)(lhs_at + 1u);
    rhs_hi = (uint16_t)(rhs_at + 1u);
    if ((size_t)counter_at >= state->ss_size ||
        (size_t)lhs_at >= state->ss_size ||
        (size_t)lhs_hi >= state->ss_size ||
        (size_t)rhs_at >= state->ss_size ||
        (size_t)rhs_hi >= state->ss_size)
        return PR_U3BD_POST_UP_TILE_LOOP_GATE_BOUNDARY;

    memset(&local, 0, sizeof local);
    local.counter_before = state->ss[counter_at];
    local.lhs = word_at(state->ss, lhs_at);
    local.rhs = word_at(state->ss, rhs_at);
    /* INC [BP-9] is followed by MOV AX,[BP-6] and CMP AX,[BP-13].
     * CMP replaces every arithmetic flag produced by INC. */
    local.counter_after = (uint8_t)(local.counter_before + 1u);
    state->ss[counter_at] = local.counter_after;
    state->ax = local.lhs;
    state->flags = cmp_flags(state->flags, local.lhs, local.rhs);
    local.next_ip = local.lhs == local.rhs ? 0x030Eu : 0x0288u;
    if (report != NULL) *report = local;
    return PR_U3BD_POST_UP_TILE_LOOP_GATE_OK;
}
