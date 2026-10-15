#include "poolrad/recovered/overlay_u3bd_post_up_0938_control.h"

enum {
    X86_CF = 0x0001u, X86_PF = 0x0004u, X86_AF = 0x0010u,
    X86_ZF = 0x0040u, X86_SF = 0x0080u, X86_OF = 0x0800u,
    X86_ARITH = X86_CF | X86_PF | X86_AF | X86_ZF | X86_SF | X86_OF,
    X86_INC = X86_PF | X86_AF | X86_ZF | X86_SF | X86_OF
};

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & 0xFFFFFu;
}

static int get_byte(const pr_u3bd_post_up_0938_control_state *s,
                    uint16_t segment, uint16_t offset, uint8_t *out)
{
    size_t at = physical(segment, offset);
    if (s->ram == NULL || at >= s->ram_size) return 0;
    *out = s->ram[at];
    return 1;
}

static int get_word(const pr_u3bd_post_up_0938_control_state *s,
                    uint16_t segment, uint16_t offset, uint16_t *out)
{
    uint8_t lo, hi;
    if (!get_byte(s, segment, offset, &lo) ||
        !get_byte(s, segment, (uint16_t)(offset + 1u), &hi)) return 0;
    *out = (uint16_t)(lo | ((uint16_t)hi << 8));
    return 1;
}

static int put_byte(pr_u3bd_post_up_0938_control_state *s,
                    uint16_t segment, uint16_t offset, uint8_t value)
{
    size_t at = physical(segment, offset);
    if (s->ram == NULL || at >= s->ram_size) return 0;
    s->ram[at] = value;
    return 1;
}

static int put_word(pr_u3bd_post_up_0938_control_state *s,
                    uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t lo = physical(segment, offset);
    size_t hi = physical(segment, (uint16_t)(offset + 1u));
    if (s->ram == NULL || lo >= s->ram_size || hi >= s->ram_size) return 0;
    s->ram[lo] = (uint8_t)value;
    s->ram[hi] = (uint8_t)(value >> 8);
    return 1;
}

static int even_parity(uint8_t value)
{
    unsigned bit, ones = 0u;
    for (bit = 0u; bit < 8u; ++bit) ones += (value >> bit) & 1u;
    return (ones & 1u) == 0u;
}

static void set_arith(pr_u3bd_post_up_0938_control_state *s,
                      uint16_t bits, uint16_t known)
{
    s->flags = (uint16_t)((s->flags & (uint16_t)~known) | (bits & known));
    s->known_arith_flags = known;
}

static void inc_byte_flags(pr_u3bd_post_up_0938_control_state *s,
                           uint8_t before, uint8_t after)
{
    uint16_t bits = 0u;
    if (even_parity(after)) bits |= X86_PF;
    if (((before ^ 1u ^ after) & 0x10u) != 0u) bits |= X86_AF;
    if (after == 0u) bits |= X86_ZF;
    if ((after & 0x80u) != 0u) bits |= X86_SF;
    if (before == 0x7Fu) bits |= X86_OF;
    s->flags = (uint16_t)((s->flags & (uint16_t)~X86_INC) | bits);
    s->known_arith_flags = (uint16_t)((s->known_arith_flags & X86_CF) | X86_INC);
}

static void sub_word_flags(pr_u3bd_post_up_0938_control_state *s,
                           uint16_t lhs, uint16_t rhs, uint16_t result)
{
    uint16_t bits = 0u;
    if (lhs < rhs) bits |= X86_CF;
    if (even_parity((uint8_t)result)) bits |= X86_PF;
    if (((lhs ^ rhs ^ result) & 0x10u) != 0u) bits |= X86_AF;
    if (result == 0u) bits |= X86_ZF;
    if ((result & 0x8000u) != 0u) bits |= X86_SF;
    if (((lhs ^ rhs) & (lhs ^ result) & 0x8000u) != 0u) bits |= X86_OF;
    set_arith(s, bits, X86_ARITH);
}

static void add_word_flags(pr_u3bd_post_up_0938_control_state *s,
                           uint16_t lhs, uint16_t rhs, uint16_t result)
{
    uint16_t bits = 0u;
    if ((uint32_t)lhs + rhs > 0xFFFFu) bits |= X86_CF;
    if (even_parity((uint8_t)result)) bits |= X86_PF;
    if (((lhs ^ rhs ^ result) & 0x10u) != 0u) bits |= X86_AF;
    if (result == 0u) bits |= X86_ZF;
    if ((result & 0x8000u) != 0u) bits |= X86_SF;
    if (((uint16_t)~(lhs ^ rhs) & (lhs ^ result) & 0x8000u) != 0u)
        bits |= X86_OF;
    set_arith(s, bits, X86_ARITH);
}

static void cmp_byte_flags(pr_u3bd_post_up_0938_control_state *s,
                           uint8_t lhs, uint8_t rhs)
{
    uint8_t result = (uint8_t)(lhs - rhs);
    uint16_t bits = 0u;
    if (lhs < rhs) bits |= X86_CF;
    if (even_parity(result)) bits |= X86_PF;
    if (((lhs ^ rhs ^ result) & 0x10u) != 0u) bits |= X86_AF;
    if (result == 0u) bits |= X86_ZF;
    if ((result & 0x80u) != 0u) bits |= X86_SF;
    if (((lhs ^ rhs) & (lhs ^ result) & 0x80u) != 0u) bits |= X86_OF;
    set_arith(s, bits, X86_ARITH);
}

static uint16_t cbw(uint8_t al)
{
    return (uint16_t)(al | ((al & 0x80u) != 0u ? 0xFF00u : 0u));
}

pr_u3bd_post_up_0938_control_status pr_u3bd_post_up_0938_control(
    pr_u3bd_post_up_0938_control_state *s,
    pr_u3bd_post_up_0938_control_report *report)
{
    pr_u3bd_post_up_0938_control_report local = {0};
    uint8_t byte;
    uint16_t word, result;

    if (s == NULL) return PR_U3BD_POST_UP_0938_CONTROL_ARGUMENT;
    s->ip = 0x0951u; /* +0938 JMP; +093A art arm is skipped. */

    if (!get_byte(s, s->ss, (uint16_t)(s->bp - 0x10u), &byte))
        return PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY;
    local.loop_before = byte;
    local.loop_after = (uint8_t)(byte + 1u);
    if (!put_byte(s, s->ss, (uint16_t)(s->bp - 0x10u), local.loop_after))
        return PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY;
    inc_byte_flags(s, byte, local.loop_after);
    s->ip = 0x0954u;

    if (!get_word(s, s->ss, (uint16_t)(s->bp - 0x12u), &word))
        return PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY;
    local.step_before = word;
    result = (uint16_t)(word - 2u);
    if (!put_word(s, s->ss, (uint16_t)(s->bp - 0x12u), result))
        return PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY;
    local.step_after = result;
    sub_word_flags(s, word, 2u, result);
    s->ip = 0x0958u;

    if (!get_byte(s, s->ss, (uint16_t)(s->bp - 1u), &byte))
        return PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY;
    local.index = byte;
    s->ax = (uint16_t)((s->ax & 0xFF00u) | byte); /* MOV AL */
    s->ax &= 0x00FFu; /* +095B XOR AH,AH */
    set_arith(s, X86_PF | X86_ZF, X86_ARITH & (uint16_t)~X86_AF);
    s->di = s->ax;
    s->ip = 0x095Fu;

    if (!get_byte(s, s->ds, (uint16_t)(s->di + 0x2736u), &byte))
        return PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY;
    local.dir_x = byte;
    s->ax = cbw(byte); /* +095F MOV AL; +0963 CBW. */
    s->ip = 0x0964u;
    if (!get_word(s, s->ss, (uint16_t)(s->bp - 0x0Du), &word))
        return PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY;
    local.x_before = word;
    result = (uint16_t)(s->ax + word);
    s->ax = result;
    add_word_flags(s, cbw(byte), word, result);
    s->ip = 0x0967u;
    if (!put_word(s, s->ss, (uint16_t)(s->bp - 0x0Du), result))
        return PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY;
    local.x_after = result;
    s->ip = 0x096Au;

    if (!get_byte(s, s->ss, (uint16_t)(s->bp - 1u), &byte))
        return PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY;
    s->ax = (uint16_t)((s->ax & 0xFF00u) | byte);
    s->ax &= 0x00FFu; /* +096D XOR AH,AH */
    set_arith(s, X86_PF | X86_ZF, X86_ARITH & (uint16_t)~X86_AF);
    s->di = s->ax;
    s->ip = 0x0971u;

    if (!get_byte(s, s->ds, (uint16_t)(s->di + 0x273Fu), &byte))
        return PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY;
    local.dir_y = byte;
    s->ax = cbw(byte); /* +0971 MOV AL; +0975 CBW. */
    s->ip = 0x0976u;
    if (!get_word(s, s->ss, (uint16_t)(s->bp - 0x0Fu), &word))
        return PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY;
    local.y_before = word;
    result = (uint16_t)(s->ax + word);
    s->ax = result;
    add_word_flags(s, cbw(byte), word, result);
    s->ip = 0x0979u;
    if (!put_word(s, s->ss, (uint16_t)(s->bp - 0x0Fu), result))
        return PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY;
    local.y_after = result;
    s->ip = 0x08FDu; /* +097C JMP */

    if (!get_byte(s, s->ss, (uint16_t)(s->bp - 0x10u), &byte))
        return PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY;
    cmp_byte_flags(s, byte, 3u);
    local.next_ip = byte >= 3u ? 0x097Fu : 0x0903u;
    s->ip = local.next_ip; /* +0901 JAE */
    if (report != NULL) *report = local;
    return PR_U3BD_POST_UP_0938_CONTROL_OK;
}
