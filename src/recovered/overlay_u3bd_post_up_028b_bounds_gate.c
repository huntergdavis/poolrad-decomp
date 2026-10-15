#include "poolrad/recovered/overlay_u3bd_post_up_028b_bounds_gate.h"

enum {
    X86_CF = 0x0001u, X86_PF = 0x0004u, X86_AF = 0x0010u,
    X86_ZF = 0x0040u, X86_SF = 0x0080u, X86_OF = 0x0800u,
    X86_ARITH = X86_CF | X86_PF | X86_AF | X86_ZF | X86_SF | X86_OF
};

static int word_ok(size_t size, uint16_t at)
{
    return (size_t)at < size && (size_t)(uint16_t)(at + 1u) < size;
}

static uint16_t word_at(const uint8_t *ss, uint16_t at)
{
    return (uint16_t)(ss[at] | ((uint16_t)ss[(uint16_t)(at + 1u)] << 8));
}

static int32_t signed_word(uint16_t value)
{
    return value < 0x8000u ? (int32_t)value : (int32_t)value - 0x10000;
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

pr_u3bd_post_up_028b_bounds_status pr_u3bd_post_up_028b_bounds_gate(
    pr_u3bd_post_up_028b_bounds_state *state,
    pr_u3bd_post_up_028b_bounds_report *report)
{
    uint16_t outer_at, inner_at, outer, inner = 0u;
    unsigned comparisons = 0u;
    uint16_t next_ip;

    if (state == NULL || state->ss == NULL)
        return PR_U3BD_POST_UP_028B_BOUNDS_ARGUMENT;
    outer_at = (uint16_t)(state->bp - 8u);
    inner_at = (uint16_t)(state->bp - 6u);

    /* +028B CMP outer,0; +028F JL +0300. */
    if (!word_ok(state->ss_size, outer_at))
        return PR_U3BD_POST_UP_028B_BOUNDS_BOUNDARY;
    outer = word_at(state->ss, outer_at);
    state->flags = cmp_flags(state->flags, outer, 0u);
    ++comparisons;
    if (signed_word(outer) < 0) goto out_of_range;

    /* +0291 CMP outer,10; +0295 JG +0300. */
    state->flags = cmp_flags(state->flags, outer, 10u);
    ++comparisons;
    if (signed_word(outer) > 10) goto out_of_range;

    /* +0297 CMP inner,0; +029B JL +0300. */
    if (!word_ok(state->ss_size, inner_at))
        return PR_U3BD_POST_UP_028B_BOUNDS_BOUNDARY;
    inner = word_at(state->ss, inner_at);
    state->flags = cmp_flags(state->flags, inner, 0u);
    ++comparisons;
    if (signed_word(inner) < 0) goto out_of_range;

    /* +029D CMP inner,10; +02A1 JG +0300; otherwise +02A3. */
    state->flags = cmp_flags(state->flags, inner, 10u);
    ++comparisons;
    next_ip = signed_word(inner) > 10 ? 0x0300u : 0x02A3u;
    goto done;

out_of_range:
    next_ip = 0x0300u;
done:
    if (report != NULL) {
        report->outer = outer;
        report->inner = inner;
        report->next_ip = next_ip;
        report->comparisons = comparisons;
    }
    return PR_U3BD_POST_UP_028B_BOUNDS_OK;
}
