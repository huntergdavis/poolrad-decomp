#include "poolrad/recovered/overlay_u3bd_post_up_02c4_tile_call.h"

enum {
    X86_CF = 0x0001u, X86_PF = 0x0004u, X86_AF = 0x0010u,
    X86_ZF = 0x0040u, X86_SF = 0x0080u, X86_OF = 0x0800u,
    X86_ARITH = X86_CF | X86_PF | X86_AF | X86_ZF | X86_SF | X86_OF
};

static uint32_t physical(uint16_t segment, uint16_t offset)
{
    return (((uint32_t)segment << 4) + offset) & 0xFFFFFu;
}

static int read_byte(const pr_u3bd_post_up_02c4_tile_call_state *s,
                     uint16_t segment, uint16_t offset, uint8_t *value)
{
    uint32_t at = physical(segment, offset);
    if (at >= s->ram_size) return 0;
    *value = s->ram[at];
    return 1;
}

static int word_address(const pr_u3bd_post_up_02c4_tile_call_state *s,
                        uint16_t segment, uint16_t offset, uint32_t *at)
{
    uint32_t first = physical(segment, offset);
    uint32_t second = (first + 1u) & 0xFFFFFu;
    if (first >= s->ram_size || second >= s->ram_size) return 0;
    *at = first;
    return 1;
}

static int read_word(const pr_u3bd_post_up_02c4_tile_call_state *s,
                     uint16_t segment, uint16_t offset, uint16_t *value)
{
    uint32_t at;
    if (!word_address(s, segment, offset, &at)) return 0;
    *value = (uint16_t)(s->ram[at] |
                        ((uint16_t)s->ram[(at + 1u) & 0xFFFFFu] << 8));
    return 1;
}

static int read_far(const pr_u3bd_post_up_02c4_tile_call_state *s,
                    uint16_t *offset, uint16_t *segment)
{
    uint32_t at = physical(s->ds, 0x6A58u);
    unsigned i;
    for (i = 0u; i < 4u; ++i)
        if (((at + i) & 0xFFFFFu) >= s->ram_size) return 0;
    *offset = (uint16_t)(s->ram[at] |
                        ((uint16_t)s->ram[(at + 1u) & 0xFFFFFu] << 8));
    *segment = (uint16_t)(s->ram[(at + 2u) & 0xFFFFFu] |
                         ((uint16_t)s->ram[(at + 3u) & 0xFFFFFu] << 8));
    return 1;
}

static int push_word(pr_u3bd_post_up_02c4_tile_call_state *s, uint16_t value)
{
    uint16_t next_sp = (uint16_t)(s->sp - 2u);
    uint32_t at;
    if (!word_address(s, s->ss, next_sp, &at)) return 0;
    s->sp = next_sp;
    s->ram[at] = (uint8_t)value;
    s->ram[(at + 1u) & 0xFFFFFu] = (uint8_t)(value >> 8);
    return 1;
}

static int even_parity(uint8_t value)
{
    unsigned bit, ones = 0u;
    for (bit = 0u; bit < 8u; ++bit)
        ones += (value >> bit) & 1u;
    return (ones & 1u) == 0u;
}

static void set_arith(pr_u3bd_post_up_02c4_tile_call_state *s,
                      uint16_t bits, uint16_t known)
{
    s->flags = (uint16_t)((s->flags & (uint16_t)~known) | (bits & known));
    s->known_arith_flags = known;
}

static void add16(pr_u3bd_post_up_02c4_tile_call_state *s,
                  uint16_t *target, uint16_t rhs)
{
    uint16_t lhs = *target;
    uint32_t sum = (uint32_t)lhs + rhs;
    uint16_t result = (uint16_t)sum;
    uint16_t bits = 0u;
    if (sum > 0xFFFFu) bits |= X86_CF;
    if (even_parity((uint8_t)result)) bits |= X86_PF;
    if (((lhs ^ rhs ^ result) & 0x10u) != 0u) bits |= X86_AF;
    if (result == 0u) bits |= X86_ZF;
    if ((result & 0x8000u) != 0u) bits |= X86_SF;
    if (((~(lhs ^ rhs)) & (lhs ^ result) & 0x8000u) != 0u)
        bits |= X86_OF;
    *target = result;
    set_arith(s, bits, X86_ARITH);
}

static void dec_ax(pr_u3bd_post_up_02c4_tile_call_state *s)
{
    uint16_t before = s->ax;
    uint16_t after = (uint16_t)(before - 1u);
    uint16_t bits = s->flags & X86_CF;
    if (even_parity((uint8_t)after)) bits |= X86_PF;
    if (((before ^ 1u ^ after) & 0x10u) != 0u) bits |= X86_AF;
    if (after == 0u) bits |= X86_ZF;
    if ((after & 0x8000u) != 0u) bits |= X86_SF;
    if (before == 0x8000u) bits |= X86_OF;
    s->ax = after;
    set_arith(s, bits, X86_ARITH);
}

static void xor_ah_zero(pr_u3bd_post_up_02c4_tile_call_state *s)
{
    s->ax &= 0x00FFu;
    set_arith(s, X86_PF | X86_ZF, X86_ARITH & (uint16_t)~X86_AF);
}

pr_u3bd_post_up_02c4_tile_call_status pr_u3bd_post_up_02c4_tile_call(
    pr_u3bd_post_up_02c4_tile_call_state *s,
    pr_u3bd_post_up_02c4_tile_callback callback, void *callback_ctx,
    pr_u3bd_post_up_02c4_tile_call_report *report)
{
    uint16_t entry_sp, x, y, item, offset, segment;
    uint8_t column, slice, item_byte;
    uint32_t product, item_physical;
    uint16_t bits;

    if (s == NULL || s->ram == NULL)
        return PR_U3BD_POST_UP_02C4_TILE_CALL_ARGUMENT;
    entry_sp = s->sp;

    /* +02C4..+02CB: inner+2, then first argument push. */
    if (!read_word(s, s->ss, (uint16_t)(s->bp - 6u), &s->ax))
        return PR_U3BD_POST_UP_02C4_TILE_CALL_BOUNDARY;
    add16(s, &s->ax, 3u);
    dec_ax(s);
    x = s->ax;
    if (!push_word(s, x)) return PR_U3BD_POST_UP_02C4_TILE_CALL_BOUNDARY;

    /* +02CC..+02D3: outer+2, then second argument push. */
    if (!read_word(s, s->ss, (uint16_t)(s->bp - 8u), &s->ax))
        return PR_U3BD_POST_UP_02C4_TILE_CALL_BOUNDARY;
    add16(s, &s->ax, 3u);
    dec_ax(s);
    y = s->ax;
    if (!push_word(s, y)) return PR_U3BD_POST_UP_02C4_TILE_CALL_BOUNDARY;

    /* +02D4..+02ED: recompute the same piece-buffer item address. */
    if (!read_byte(s, s->ss, (uint16_t)(s->bp - 9u), &column))
        return PR_U3BD_POST_UP_02C4_TILE_CALL_BOUNDARY;
    s->ax = (uint16_t)((s->ax & 0xFF00u) | column);
    xor_ah_zero(s);
    s->cx = s->ax;
    if (!read_byte(s, s->ss, (uint16_t)(s->bp + 8u), &slice))
        return PR_U3BD_POST_UP_02C4_TILE_CALL_BOUNDARY;
    s->ax = (uint16_t)((s->ax & 0xFF00u) | slice);
    xor_ah_zero(s);
    s->dx = 0x009Cu;
    product = (uint32_t)s->ax * s->dx;
    s->ax = (uint16_t)product;
    s->dx = (uint16_t)(product >> 16);
    bits = s->dx == 0u ? 0u : X86_CF | X86_OF;
    set_arith(s, bits, X86_CF | X86_OF);
    if (!read_far(s, &offset, &segment))
        return PR_U3BD_POST_UP_02C4_TILE_CALL_BOUNDARY;
    s->di = offset;
    s->es = segment;
    add16(s, &s->di, s->ax);
    add16(s, &s->di, s->cx);
    item_physical = physical(s->es, (uint16_t)(s->di - 0x009Cu));
    if (!read_byte(s, s->es, (uint16_t)(s->di - 0x009Cu), &item_byte))
        return PR_U3BD_POST_UP_02C4_TILE_CALL_BOUNDARY;

    /* +02F2..+02FA: zero-extend item, then push item, 1, 1. */
    s->ax = (uint16_t)((s->ax & 0xFF00u) | item_byte);
    xor_ah_zero(s);
    item = s->ax;
    if (!push_word(s, item))
        return PR_U3BD_POST_UP_02C4_TILE_CALL_BOUNDARY;
    s->ax = (uint16_t)((s->ax & 0xFF00u) | 1u);
    if (!push_word(s, s->ax))
        return PR_U3BD_POST_UP_02C4_TILE_CALL_BOUNDARY;
    s->ax = (uint16_t)((s->ax & 0xFF00u) | 1u);
    if (!push_word(s, s->ax))
        return PR_U3BD_POST_UP_02C4_TILE_CALL_BOUNDARY;

    if (report != NULL) {
        report->x = x;
        report->y = y;
        report->item = item;
        report->one_a = 1u;
        report->one_b = 1u;
        report->call_sp = s->sp;
        report->item_physical = item_physical;
        report->next_ip = 0x02FBu;
    }
    /* +02FB is an external far call, reached only after every push. */
    if (callback == NULL)
        return PR_U3BD_POST_UP_02C4_TILE_CALL_CALLBACK_REQUIRED;
    if (callback(callback_ctx, x, y, item, 1u, 1u) != 0)
        return PR_U3BD_POST_UP_02C4_TILE_CALL_CALLBACK_ERROR;
    s->sp = entry_sp; /* Reached callee returns with five words cleaned. */
    if (report != NULL) report->next_ip = 0x0300u;
    return PR_U3BD_POST_UP_02C4_TILE_CALL_OK;
}
