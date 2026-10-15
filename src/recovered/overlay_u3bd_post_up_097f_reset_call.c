#include "poolrad/recovered/overlay_u3bd_post_up_097f_reset_call.h"

enum { X86_CF = 0x0001u, X86_PF = 0x0004u, X86_AF = 0x0010u,
       X86_ZF = 0x0040u, X86_SF = 0x0080u, X86_OF = 0x0800u,
       X86_ARITH = X86_CF | X86_PF | X86_AF | X86_ZF | X86_SF | X86_OF };

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & 0xFFFFFu;
}

static int get_byte(const pr_u3bd_post_up_097f_reset_call_state *s,
                    uint16_t offset, uint8_t *out)
{
    size_t at = physical(s->ss, offset);
    if (s->ram == NULL || at >= s->ram_size) return 0;
    *out = s->ram[at];
    return 1;
}

static int get_word(const pr_u3bd_post_up_097f_reset_call_state *s,
                    uint16_t offset, uint16_t *out)
{
    uint8_t lo, hi;
    if (!get_byte(s, offset, &lo) ||
        !get_byte(s, (uint16_t)(offset + 1u), &hi)) return 0;
    *out = (uint16_t)(lo | ((uint16_t)hi << 8));
    return 1;
}

static int put_byte(pr_u3bd_post_up_097f_reset_call_state *s,
                    uint16_t offset, uint8_t value)
{
    size_t at = physical(s->ss, offset);
    if (s->ram == NULL || at >= s->ram_size) return 0;
    s->ram[at] = value;
    return 1;
}

static int put_word(pr_u3bd_post_up_097f_reset_call_state *s,
                    uint16_t offset, uint16_t value)
{
    size_t lo = physical(s->ss, offset);
    size_t hi = physical(s->ss, (uint16_t)(offset + 1u));
    if (s->ram == NULL || lo >= s->ram_size || hi >= s->ram_size) return 0;
    s->ram[lo] = (uint8_t)value;
    s->ram[hi] = (uint8_t)(value >> 8);
    return 1;
}

static int push_word(pr_u3bd_post_up_097f_reset_call_state *s,
                     uint16_t value)
{
    uint16_t next = (uint16_t)(s->sp - 2u);
    if (!put_word(s, next, value)) return 0;
    s->sp = next;
    return 1;
}

static int even_parity(uint8_t value)
{
    unsigned i, ones = 0u;
    for (i = 0u; i < 8u; ++i) ones += (value >> i) & 1u;
    return (ones & 1u) == 0u;
}

static void cmp_byte_three(pr_u3bd_post_up_097f_reset_call_state *s,
                           uint8_t lhs)
{
    uint8_t result = (uint8_t)(lhs - 3u);
    uint16_t bits = 0u;
    if (lhs < 3u) bits |= X86_CF;
    if (even_parity(result)) bits |= X86_PF;
    if (((lhs ^ 3u ^ result) & 0x10u) != 0u) bits |= X86_AF;
    if (result == 0u) bits |= X86_ZF;
    if ((result & 0x80u) != 0u) bits |= X86_SF;
    if (((lhs ^ 3u) & (lhs ^ result) & 0x80u) != 0u) bits |= X86_OF;
    s->flags = (uint16_t)((s->flags & (uint16_t)~X86_ARITH) | bits);
    s->known_arith_flags = X86_ARITH;
}

pr_u3bd_post_up_097f_reset_call_status pr_u3bd_post_up_097f_reset_call(
    pr_u3bd_post_up_097f_reset_call_state *s,
    pr_u3bd_post_up_097f_reset_call_report *report)
{
    pr_u3bd_post_up_097f_reset_call_report r = {0};
    uint16_t word;
    uint8_t byte;

    if (s == NULL || s->ip != 0x097Fu)
        return PR_U3BD_POST_UP_097F_RESET_CALL_ARGUMENT;

    if (!get_word(s, (uint16_t)(s->bp - 5u), &word))
        return PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY;
    s->ax = word; r.x = word; s->ip = 0x0982u;
    if (!put_word(s, (uint16_t)(s->bp - 0x0Du), s->ax))
        return PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY;
    s->ip = 0x0985u;

    if (!get_word(s, (uint16_t)(s->bp - 7u), &word))
        return PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY;
    s->ax = word; r.y = word; s->ip = 0x0988u;
    if (!put_word(s, (uint16_t)(s->bp - 0x0Fu), s->ax))
        return PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY;
    s->ip = 0x098Bu;

    if (!put_byte(s, (uint16_t)(s->bp - 0x10u), 0u))
        return PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY;
    s->ip = 0x098Fu;

    s->ax = 0u; /* XOR AX,AX; AF is architecturally undefined. */
    s->flags = (uint16_t)((s->flags & (uint16_t)~X86_ARITH) | X86_PF | X86_ZF);
    s->known_arith_flags = X86_ARITH & (uint16_t)~X86_AF;
    s->ip = 0x0991u;
    if (!put_word(s, (uint16_t)(s->bp - 0x12u), 0u))
        return PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY;
    s->ip = 0x0994u;

    if (!get_byte(s, (uint16_t)(s->bp - 0x10u), &byte))
        return PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY;
    cmp_byte_three(s, byte);
    s->ip = byte >= 3u ? 0x0A16u : 0x099Au;
    if (byte >= 3u) { if (report != NULL) *report = r;
                      return PR_U3BD_POST_UP_097F_RESET_CALL_OK; }

    if (!get_byte(s, (uint16_t)(s->bp - 0x0Du), &byte))
        return PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY;
    s->ax = (uint16_t)((s->ax & 0xFF00u) | byte);
    r.pushed_x = s->ax; s->ip = 0x099Du;
    if (!push_word(s, s->ax)) return PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY;
    s->ip = 0x099Eu;

    if (!get_byte(s, (uint16_t)(s->bp - 0x0Fu), &byte))
        return PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY;
    s->ax = (uint16_t)((s->ax & 0xFF00u) | byte);
    r.pushed_y = s->ax; s->ip = 0x09A1u;
    if (!push_word(s, s->ax)) return PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY;
    s->ip = 0x09A2u;

    if (!get_byte(s, (uint16_t)(s->bp - 2u), &byte))
        return PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY;
    s->ax = (uint16_t)((s->ax & 0xFF00u) | byte);
    r.query = byte; r.pushed_query = s->ax; s->ip = 0x09A5u;
    if (!push_word(s, s->ax)) return PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY;
    s->ip = 0x09A6u;

    r.pushed_cs = s->cs;
    if (!push_word(s, s->cs)) return PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY;
    s->ip = 0x09A7u;
    if (report != NULL) *report = r;
    return PR_U3BD_POST_UP_097F_RESET_CALL_OK;
}
