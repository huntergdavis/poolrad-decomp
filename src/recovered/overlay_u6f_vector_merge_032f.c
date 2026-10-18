/* GAME.OVR unit 6F:+032F..+0475, complete 327-byte call-free body. */
#include "poolrad/recovered/overlay_u6f_vector_merge_032f.h"

#define RAM_MASK (PR_U6F_032F_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at & RAM_MASK] |
                      ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
}

static uint16_t word(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void put_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                     uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static uint16_t local(const uint8_t *ram, const pr_u6f_032f_cpu *c,
                      unsigned distance)
{
    return word(ram, c->ss, (uint16_t)(c->bp - distance));
}

static void set_local(uint8_t *ram, const pr_u6f_032f_cpu *c,
                      unsigned distance, uint16_t value)
{
    put_word(ram, c->ss, (uint16_t)(c->bp - distance), value);
}

static void les_at(const uint8_t *ram, pr_u6f_032f_cpu *c, size_t at)
{
    uint16_t offset = read16_at(ram, at);
    uint16_t segment = read16_at(ram, at + 2u);
    c->di = offset;
    c->es = segment;
}

static void les_argument(const uint8_t *ram, pr_u6f_032f_cpu *c,
                         unsigned distance)
{
    les_at(ram, c, physical(c->ss, (uint16_t)(c->bp + distance)));
}

static void nested_pointer(const uint8_t *ram, pr_u6f_032f_cpu *c)
{
    les_at(ram, c, physical(c->es, (uint16_t)(c->di + 0x13u)));
}

static void set_al(pr_u6f_032f_cpu *c, uint8_t value)
{
    c->ax = (uint16_t)((c->ax & 0xFF00u) | value);
}

static void set_dl(pr_u6f_032f_cpu *c, uint8_t value)
{
    c->dx = (uint16_t)((c->dx & 0xFF00u) | value);
}

static void cmp_flags(pr_u6f_032f_cpu *c, uint16_t left, uint16_t right,
                      unsigned bits)
{
    uint16_t mask = bits == 8u ? 0xFFu : 0xFFFFu;
    uint16_t sign = bits == 8u ? 0x80u : 0x8000u;
    uint16_t result = (uint16_t)((left - right) & mask);
    uint8_t parity = (uint8_t)result;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    c->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) c->flags |= 0x0001u;
    if (!(parity & 1u)) c->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) c->flags |= 0x0010u;
    if (result == 0u) c->flags |= 0x0040u;
    if (result & sign) c->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & sign) c->flags |= 0x0800u;
}

static void inline_or(uint8_t *ram, pr_u6f_032f_cpu *c)
{
    c->ax = local(ram, c, 4u);
    les_argument(ram, c, 10u);
    c->di = (uint16_t)(c->di + c->ax);
    set_dl(c, ram[physical(c->es, (uint16_t)(c->di + 0x17u))]);
    c->ax = local(ram, c, 4u);
    les_argument(ram, c, 6u);
    c->di = (uint16_t)(c->di + c->ax);
    set_al(c, ram[physical(c->es, (uint16_t)(c->di + 0x17u))]);
    set_al(c, (uint8_t)(c->ax | c->dx));
    set_dl(c, (uint8_t)c->ax);
    c->ax = local(ram, c, 4u);
    les_argument(ram, c, 6u);
    c->di = (uint16_t)(c->di + c->ax);
    ram[physical(c->es, (uint16_t)(c->di + 0x17u))] = (uint8_t)c->dx;
}

static void pointed_and(uint8_t *ram, pr_u6f_032f_cpu *c)
{
    c->ax = local(ram, c, 4u);
    les_argument(ram, c, 10u);
    nested_pointer(ram, c);
    c->di = (uint16_t)(c->di + c->ax);
    set_dl(c, ram[physical(c->es, c->di)]);
    c->ax = local(ram, c, 4u);
    les_argument(ram, c, 6u);
    nested_pointer(ram, c);
    c->di = (uint16_t)(c->di + c->ax);
    set_al(c, ram[physical(c->es, c->di)]);
    set_al(c, (uint8_t)(c->ax & c->dx));
    set_dl(c, (uint8_t)c->ax);
    c->ax = local(ram, c, 4u);
    les_argument(ram, c, 6u);
    nested_pointer(ram, c);
    c->di = (uint16_t)(c->di + c->ax);
    ram[physical(c->es, c->di)] = (uint8_t)c->dx;
}

static void merge_loop(uint8_t *ram, pr_u6f_032f_cpu *c,
                       int do_or, int do_and)
{
    c->ax = 0u;
    cmp_flags(c, c->ax, local(ram, c, 6u), 16u);
    /* The source's unsigned 0 > bound guard cannot be true. In particular,
     * DEC of a zero length produces FFFF rather than an empty operation. */
    set_local(ram, c, 4u, c->ax);
    for (;;) {
        if (do_or) inline_or(ram, c);
        if (do_and) pointed_and(ram, c);
        c->ax = local(ram, c, 4u);
        cmp_flags(c, c->ax, local(ram, c, 6u), 16u);
        if (c->ax == local(ram, c, 6u)) break;
        set_local(ram, c, 4u, (uint16_t)(local(ram, c, 4u) + 1u));
    }
}

pr_u6f_vector_032f_status pr_u6f_vector_merge_032f(
    uint8_t *ram, size_t ram_size, pr_u6f_032f_cpu *c)
{
    uint8_t mode;
    if (ram == NULL || c == NULL || ram_size < PR_U6F_032F_RAM_SIZE)
        return PR_U6F_VECTOR_032F_ARGUMENT;
    c->sp = (uint16_t)(c->sp - 2u);
    put_word(ram, c->ss, c->sp, c->bp);
    c->bp = c->sp;
    c->sp = (uint16_t)(c->sp - 6u);
    mode = ram[physical(c->ds, 0x4A1Eu)];
    set_al(c, mode);
    if (mode == 0u || mode == 2u || mode == 1u) {
        les_argument(ram, c, 10u);
        c->ax = word(ram, c->es, (uint16_t)(c->di + 0x11u));
        c->ax = (uint16_t)(c->ax - 1u);
        set_local(ram, c, 6u, c->ax);
        merge_loop(ram, c, 1, mode != 1u);
        if (mode == 1u) {
            les_at(ram, c, physical(c->ds, 0x5EB2u));
            c->ax = word(ram, c->es, (uint16_t)(c->di + 0x11u));
            c->ax = (uint16_t)((c->ax >> 2) - 1u);
            set_local(ram, c, 2u, c->ax);
            c->ax = local(ram, c, 2u);
            set_local(ram, c, 6u, c->ax);
            merge_loop(ram, c, 0, 1);
        }
    } else {
        cmp_flags(c, mode, 1u, 8u);
    }
    c->sp = c->bp;
    c->bp = word(ram, c->ss, c->sp);
    c->sp = (uint16_t)(c->sp + 2u);
    c->ip = word(ram, c->ss, c->sp);
    c->cs = word(ram, c->ss, (uint16_t)(c->sp + 2u));
    c->sp = (uint16_t)(c->sp + 12u);
    return PR_U6F_VECTOR_032F_OK;
}
