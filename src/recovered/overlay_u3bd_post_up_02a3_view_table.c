#include "poolrad/recovered/overlay_u3bd_post_up_02a3_view_table.h"

enum {
    X86_CF = 0x0001u, X86_PF = 0x0004u, X86_AF = 0x0010u,
    X86_ZF = 0x0040u, X86_SF = 0x0080u, X86_OF = 0x0800u,
    X86_ARITH = X86_CF | X86_PF | X86_AF | X86_ZF | X86_SF | X86_OF
};

static uint32_t physical(uint16_t segment, uint16_t offset)
{
    return (((uint32_t)segment << 4) + offset) & 0xFFFFFu;
}

static int read_byte(const pr_u3bd_post_up_02a3_view_table_state *s,
                     uint16_t segment, uint16_t offset, uint8_t *value)
{
    uint32_t at = physical(segment, offset);
    if (at >= s->ram_size) return 0;
    *value = s->ram[at];
    return 1;
}

static int read_far(const pr_u3bd_post_up_02a3_view_table_state *s,
                    uint16_t *offset, uint16_t *segment)
{
    uint32_t at = physical(s->ds, 0x6A58u);
    uint8_t bytes[4];
    unsigned i;
    for (i = 0u; i < 4u; ++i) {
        uint32_t address = (at + i) & 0xFFFFFu;
        if (address >= s->ram_size) return 0;
        bytes[i] = s->ram[address];
    }
    *offset = (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
    *segment = (uint16_t)(bytes[2] | ((uint16_t)bytes[3] << 8));
    return 1;
}

static int even_parity(uint8_t value)
{
    unsigned bit, ones = 0u;
    for (bit = 0u; bit < 8u; ++bit)
        ones += (value >> bit) & 1u;
    return (ones & 1u) == 0u;
}

static void set_arith(pr_u3bd_post_up_02a3_view_table_state *s,
                      uint16_t bits, uint16_t known)
{
    s->flags = (uint16_t)((s->flags & (uint16_t)~known) | (bits & known));
    s->known_arith_flags = known;
}

static void add_di(pr_u3bd_post_up_02a3_view_table_state *s, uint16_t rhs)
{
    uint16_t lhs = s->di;
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
    s->di = result;
    set_arith(s, bits, X86_ARITH);
}

pr_u3bd_post_up_02a3_view_table_status
pr_u3bd_post_up_02a3_view_table(
    pr_u3bd_post_up_02a3_view_table_state *s,
    pr_u3bd_post_up_02a3_view_table_report *report)
{
    uint8_t column, slice, item;
    uint16_t offset, segment, item_offset;
    uint32_t product, item_physical;
    uint16_t bits;

    if (s == NULL || s->ram == NULL)
        return PR_U3BD_POST_UP_02A3_VIEW_TABLE_ARGUMENT;

    /* +02A3 MOV AL,[BP-9]; +02A6 XOR AH,AH; +02A8 MOV CX,AX. */
    if (!read_byte(s, s->ss, (uint16_t)(s->bp - 9u), &column))
        return PR_U3BD_POST_UP_02A3_VIEW_TABLE_BOUNDARY;
    s->ax = (uint16_t)((s->ax & 0xFF00u) | column);
    s->ax = column;
    set_arith(s, X86_PF | X86_ZF, X86_ARITH & (uint16_t)~X86_AF);
    s->cx = s->ax;

    /* +02AA MOV AL,[BP+8]; +02AD XOR AH,AH; +02AF MOV DX,009C. */
    if (!read_byte(s, s->ss, (uint16_t)(s->bp + 8u), &slice))
        return PR_U3BD_POST_UP_02A3_VIEW_TABLE_BOUNDARY;
    s->ax = (uint16_t)((s->ax & 0xFF00u) | slice);
    s->ax = slice;
    set_arith(s, X86_PF | X86_ZF, X86_ARITH & (uint16_t)~X86_AF);
    s->dx = 0x009Cu;

    /* +02B2 MUL DX. Only CF and OF are defined after unsigned MUL. */
    product = (uint32_t)s->ax * s->dx;
    s->ax = (uint16_t)product;
    s->dx = (uint16_t)(product >> 16);
    bits = s->dx == 0u ? 0u : X86_CF | X86_OF;
    set_arith(s, bits, X86_CF | X86_OF);

    /* +02B4 LES DI,[6A58]. A truncated far operand stops before LES. */
    if (!read_far(s, &offset, &segment))
        return PR_U3BD_POST_UP_02A3_VIEW_TABLE_BOUNDARY;
    s->di = offset;
    s->es = segment;
    /* +02B8 ADD DI,AX; +02BA ADD DI,CX. */
    add_di(s, s->ax);
    add_di(s, s->cx);

    /* +02BC CMP byte ES:[DI-009C],0; +02C2 JBE +0300. */
    item_offset = (uint16_t)(s->di - 0x009Cu);
    item_physical = physical(s->es, item_offset);
    if (!read_byte(s, s->es, item_offset, &item))
        return PR_U3BD_POST_UP_02A3_VIEW_TABLE_BOUNDARY;
    bits = 0u;
    if (even_parity(item)) bits |= X86_PF;
    if (item == 0u) bits |= X86_ZF;
    if ((item & 0x80u) != 0u) bits |= X86_SF;
    set_arith(s, bits, X86_ARITH);
    if (report != NULL) {
        report->column = column;
        report->slice = slice;
        report->item = item;
        report->item_offset = item_offset;
        report->item_physical = item_physical;
        report->next_ip = item == 0u ? 0x0300u : 0x02C4u;
    }
    return PR_U3BD_POST_UP_02A3_VIEW_TABLE_OK;
}
