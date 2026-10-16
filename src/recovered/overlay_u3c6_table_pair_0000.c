/* Source candidate only: docs/re/U3C6_TABLE_PAIR_0000.md. */
#include "poolrad/recovered/overlay_u3c6_table_pair_0000.h"

#define MASK (PR_U6C_3F31_RAM_SIZE - 1u)
static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4u) + offset) & MASK;
}
static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8u));
}
static void push_bp(uint8_t *ram, pr_u3c6_0000_cpu *c)
{
    size_t at;
    c->sp = (uint16_t)(c->sp - 2u);
    at = physical(c->ss, c->sp);
    ram[at] = (uint8_t)c->bp;
    ram[(at + 1u) & MASK] = (uint8_t)(c->bp >> 8u);
}
static void les(uint8_t *ram, pr_u3c6_0000_cpu *c, uint16_t offset)
{
    size_t at = physical(c->ss, offset);
    uint16_t di = word_at(ram, at);
    uint16_t es = word_at(ram, (at + 2u) & MASK);
    c->di = di;
    c->es = es;
}
static void cmp_zero(pr_u3c6_0000_cpu *c, uint8_t value)
{
    uint8_t p = value;
    uint16_t flags = 0;
    p ^= (uint8_t)(p >> 4u);
    p ^= (uint8_t)(p >> 2u);
    p ^= (uint8_t)(p >> 1u);
    if (!(p & 1u)) flags |= 4u;
    if (!value) flags |= 0x40u;
    if (value & 0x80u) flags |= 0x80u;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | flags);
}
static void table_byte(uint8_t *ram, pr_u3c6_0000_cpu *c,
                       uint16_t displacement)
{
    /* Each invocation rereads both live argument bytes. The first output
     * write can change either input, the second table byte or its pointer.
     * XOR/SHL/ADD flags are unobservable: a later CMP defines all six. */
    c->ax = ram[physical(c->ss, (uint16_t)(c->bp + 0x0Eu))];
    c->ax = (uint16_t)(c->ax << 1u);
    c->dx = c->ax;
    c->ax = ram[physical(c->ss, (uint16_t)(c->bp + 0x10u))];
    c->di = c->ax;
    c->cx = (uint16_t)((c->cx & 0xFF00u) | 3u);
    c->di = (uint16_t)((c->di << 3u) + c->dx);
    c->ax = (uint16_t)((c->ax & 0xFF00u) |
        ram[physical(c->ds, (uint16_t)(c->di + displacement))]);
}
pr_u3c6_0000_status pr_u3c6_table_pair_0000(
    uint8_t *ram, size_t ram_size, pr_u3c6_0000_cpu *c,
    pr_u3c6_0000_retf_policy policy)
{
    uint8_t value;
    size_t at;
    if (!ram || !c || ram_size != PR_U6C_3F31_RAM_SIZE || c->ip != 0u ||
        (policy != PR_U3C6_0000_RETF_UNMEASURED &&
         policy != PR_U3C6_0000_RETF_WRAPPED_SS &&
         policy != PR_U3C6_0000_RETF_CONTIGUOUS_PHYSICAL))
        return PR_U3C6_0000_ARGUMENT;
    push_bp(ram, c);
    c->bp = c->sp;
    c->sp = (uint16_t)(c->sp - 2u);
    ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = 0;
    value = ram[physical(c->ss, (uint16_t)(c->bp + 0x10u))];
    cmp_zero(c, value);
    if (value) {
        table_byte(ram, c, 0x2844u);
        les(ram, c, (uint16_t)(c->bp + 0x0Au));
        ram[physical(c->es, c->di)] = (uint8_t)c->ax;
        table_byte(ram, c, 0x2845u);
        les(ram, c, (uint16_t)(c->bp + 6u));
        ram[physical(c->es, c->di)] = (uint8_t)c->ax;
        les(ram, c, (uint16_t)(c->bp + 0x0Au));
        value = ram[physical(c->es, c->di)];
        cmp_zero(c, value);
        if (!(value & 0x80u))
            ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = 1;
    }
    c->ax = (uint16_t)((c->ax & 0xFF00u) |
        ram[physical(c->ss, (uint16_t)(c->bp - 1u))]);
    c->sp = c->bp;
    c->bp = word_at(ram, physical(c->ss, c->sp));
    c->sp = (uint16_t)(c->sp + 2u);
    c->ip = 0x0065u;
    if (policy == PR_U3C6_0000_RETF_UNMEASURED)
        return PR_U3C6_0000_RETURN_PENDING;
    at = physical(c->ss, c->sp);
    c->ip = word_at(ram, at);
    c->sp = (uint16_t)(c->sp + 2u);
    c->cs = word_at(ram, policy == PR_U3C6_0000_RETF_WRAPPED_SS ?
        physical(c->ss, c->sp) : ((at + 2u) & MASK));
    c->sp = (uint16_t)(c->sp + 14u);
    return PR_U3C6_0000_RETURNED;
}
