#include "poolrad/recovered/overlay_u3bd_post_up_026e_arm.h"

enum {
    X86_CF = 0x0001u,
    X86_PF = 0x0004u,
    X86_AF = 0x0010u,
    X86_ZF = 0x0040u,
    X86_SF = 0x0080u,
    X86_OF = 0x0800u,
    X86_INC_FLAGS = X86_PF | X86_AF | X86_ZF | X86_SF | X86_OF,
    X86_ARITH = X86_CF | X86_INC_FLAGS
};

static int word_ok(size_t size, uint16_t at)
{
    return (size_t)at < size && (size_t)(uint16_t)(at + 1u) < size;
}

static uint16_t word_at(const uint8_t *ss, uint16_t at)
{
    return (uint16_t)(ss[at] | ((uint16_t)ss[(uint16_t)(at + 1u)] << 8));
}

static void put_word(uint8_t *ss, uint16_t at, uint16_t value)
{
    ss[at] = (uint8_t)value;
    ss[(uint16_t)(at + 1u)] = (uint8_t)(value >> 8);
}

static int even_parity(uint8_t value)
{
    unsigned bit, ones = 0u;
    for (bit = 0u; bit < 8u; ++bit)
        ones += (value >> bit) & 1u;
    return (ones & 1u) == 0u;
}

static uint16_t inc_flags(uint16_t old, uint16_t before, uint16_t after)
{
    uint16_t bits = 0u;
    if (even_parity((uint8_t)after)) bits |= X86_PF;
    if (((before ^ 1u ^ after) & 0x10u) != 0u) bits |= X86_AF;
    if (after == 0u) bits |= X86_ZF;
    if ((after & 0x8000u) != 0u) bits |= X86_SF;
    if (before == 0x7FFFu) bits |= X86_OF;
    return (uint16_t)((old & (uint16_t)~X86_INC_FLAGS) | bits);
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

pr_u3bd_post_up_026e_arm_status pr_u3bd_post_up_026e_arm(
    pr_u3bd_post_up_026e_arm_state *state,
    pr_u3bd_post_up_026e_arm_report *report)
{
    uint16_t outer_at, copy_source_at, copy_target_at, input_at, inner_at;
    uint16_t outer_before, outer_after, copied_word, compared_ax, rhs;
    uint8_t source_byte;
    int less_or_equal;

    if (state == NULL || state->ss == NULL)
        return PR_U3BD_POST_UP_026E_ARM_ARGUMENT;
    outer_at = (uint16_t)(state->bp - 8u);
    copy_source_at = (uint16_t)(state->bp - 2u);
    copy_target_at = (uint16_t)(state->bp - 13u);
    input_at = (uint16_t)(state->bp + 12u);
    inner_at = (uint16_t)(state->bp - 6u);

    /* +026E INC word SS:[BP-8]. Later missing operands preserve this write. */
    if (!word_ok(state->ss_size, outer_at))
        return PR_U3BD_POST_UP_026E_ARM_BOUNDARY;
    outer_before = word_at(state->ss, outer_at);
    outer_after = (uint16_t)(outer_before + 1u);
    put_word(state->ss, outer_at, outer_after);
    state->flags = inc_flags(state->flags, outer_before, outer_after);

    /* +0271 MOV AX,SS:[BP-2]; +0274 MOV SS:[BP-13],AX. */
    if (!word_ok(state->ss_size, copy_source_at))
        return PR_U3BD_POST_UP_026E_ARM_BOUNDARY;
    copied_word = word_at(state->ss, copy_source_at);
    state->ax = copied_word;
    if (!word_ok(state->ss_size, copy_target_at))
        return PR_U3BD_POST_UP_026E_ARM_BOUNDARY;
    put_word(state->ss, copy_target_at, copied_word);

    /* +0277 MOV AL,SS:[BP+12]; +027A CBW; +027B CMP AX,[BP-13]. */
    if ((size_t)input_at >= state->ss_size)
        return PR_U3BD_POST_UP_026E_ARM_BOUNDARY;
    source_byte = state->ss[input_at];
    state->ax = (uint16_t)((state->ax & 0xFF00u) | source_byte);
    compared_ax = (uint16_t)(int16_t)(int8_t)source_byte;
    state->ax = compared_ax;
    rhs = word_at(state->ss, copy_target_at);
    state->flags = cmp_flags(state->flags, compared_ax, rhs);
    less_or_equal = (int16_t)compared_ax <= (int16_t)rhs;

    /* +027E JLE; +0280 JMP +030E; +0283 MOV [BP-6],AX; +0286 JMP +028B. */
    if (less_or_equal) {
        if (!word_ok(state->ss_size, inner_at))
            return PR_U3BD_POST_UP_026E_ARM_BOUNDARY;
        put_word(state->ss, inner_at, compared_ax);
    }
    if (report != NULL) {
        report->outer_before = outer_before;
        report->outer_after = outer_after;
        report->copied_word = copied_word;
        report->source_byte = source_byte;
        report->compared_ax = compared_ax;
        report->next_ip = less_or_equal ? 0x028Bu : 0x030Eu;
    }
    return PR_U3BD_POST_UP_026E_ARM_OK;
}
