#include "overlay_loaded_contracts.h"
/* GAME.OVR U5E [263D,26B0); see docs/re/U5E_263D_RECON.md.
 * Slot-reader semantics also documented by overlay_u3c6_1298_cpu.c.
 * All source data, including stack locals, remains in shared RAM. */
#include "poolrad/recovered/overlay_u5e_direction_263d.h"
#include "poolrad/recovered/overlay_u3c2_entry4_057b.h"

#include <string.h>

#define MASK (PR_U5E_263D_RAM_SIZE - 1u)
#define ARITH 0x08D5u
#define BP(n) ((uint16_t)(c->bp + (n)))

typedef struct {
    uint8_t *ram;
    pr_u5e_263d_cpu *c;
    size_t remaining;
} context;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t r8(const context *x, uint16_t segment, uint16_t offset)
{
    return x->ram[physical(segment, offset)];
}

static uint16_t r16(const context *x, uint16_t segment, uint16_t offset)
{
    size_t p = physical(segment, offset);
    return (uint16_t)(x->ram[p] | ((uint16_t)x->ram[(p + 1u) & MASK] << 8));
}

static void w8(context *x, uint16_t segment, uint16_t offset, uint8_t value)
{
    x->ram[physical(segment, offset)] = value;
}

static void push(context *x, uint16_t value)
{
    size_t p;
    x->c->sp = (uint16_t)(x->c->sp - 2u);
    p = physical(x->c->ss, x->c->sp);
    x->ram[p] = (uint8_t)value;
    x->ram[(p + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint16_t pop(context *x)
{
    uint16_t value = r16(x, x->c->ss, x->c->sp);
    x->c->sp = (uint16_t)(x->c->sp + 2u);
    return value;
}

static void leave(context *x, uint16_t arguments)
{
    x->c->sp = x->c->bp;
    x->c->bp = pop(x);
    x->c->ip = pop(x);
    x->c->cs = pop(x);
    x->c->sp = (uint16_t)(x->c->sp + arguments);
}

static uint16_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u ? 4u : 0u;
}

static uint16_t arithmetic(pr_u5e_263d_cpu *c, uint16_t a, uint16_t b,
                           unsigned bits, int subtract)
{
    uint16_t sign = bits == 8u ? 0x80u : 0x8000u;
    uint16_t mask = (uint16_t)(sign | (sign - 1u));
    uint32_t wide = subtract ? (uint32_t)a - b : (uint32_t)a + b;
    uint16_t result = (uint16_t)(wide & mask);
    uint16_t f = parity((uint8_t)result);
    if (subtract ? a < b : wide > mask) f |= 1u;
    if (((a ^ b ^ result) & 0x10u) != 0u) f |= 0x10u;
    if (result == 0u) f |= 0x40u;
    if ((result & sign) != 0u) f |= 0x80u;
    if (((subtract ? (a ^ b) : (uint16_t)~(a ^ b)) &
         (a ^ result) & sign) != 0u) f |= 0x800u;
    c->flags = (uint16_t)((c->flags & (uint16_t)~ARITH) | f);
    return result;
}

static void cmp8(pr_u5e_263d_cpu *c, uint8_t a, uint8_t b)
{
    (void)arithmetic(c, a, b, 8u, 1);
}

static void set_al(pr_u5e_263d_cpu *c, uint8_t value)
{
    c->ax = (uint16_t)((c->ax & 0xFF00u) | value);
}

static void logic8(pr_u5e_263d_cpu *c, uint8_t result)
{
    uint16_t f = parity(result);
    if (result == 0u) f |= 0x40u;
    if ((result & 0x80u) != 0u) f |= 0x80u;
    c->flags = (uint16_t)((c->flags & (uint16_t)~ARITH) | f);
}

static void xor_ah(pr_u5e_263d_cpu *c)
{
    c->ax &= 0xFFu;
    logic8(c, 0u); /* XOR AH,AH, not XOR AX,AX. */
}

static void times_four(pr_u5e_263d_cpu *c)
{
    unsigned i;
    c->di = c->ax;
    for (i = 0; i < 2u; ++i) {
        uint16_t old = c->di;
        c->di = (uint16_t)(old << 1);
        c->flags = (uint16_t)((c->flags & (uint16_t)~ARITH) |
                              parity((uint8_t)c->di));
        c->flags |= 0x10u; /* pinned: SHL sets AF for a non-zero count */
        if ((old & 0x8000u) != 0u) c->flags |= 1u;
        if (c->di == 0u) c->flags |= 0x40u;
        if ((c->di & 0x8000u) != 0u) c->flags |= 0x80u;
        if (((old ^ c->di) & 0x8000u) != 0u) c->flags |= 0x800u;
    }
}

static void inc_local(context *x, uint16_t offset)
{
    uint16_t carry = x->c->flags & 1u;
    uint8_t old = r8(x, x->c->ss, offset);
    w8(x, x->c->ss, offset,
       (uint8_t)arithmetic(x->c, old, 1u, 8u, 0));
    x->c->flags = (uint16_t)((x->c->flags & (uint16_t)~1u) | carry);
}

static int tick(context *x, uint16_t ip)
{
    x->c->ip = ip;
    if (x->remaining == 0u) return 0;
    --x->remaining;
    return 1;
}

/* U3C6 +0C5F: byte-indexed far-pointer lookup, including count zero's
 * full 256-slot traversal and exact word-pair equality. */
static pr_u5e_263d_status find_slot(context *x)
{
    pr_u5e_263d_cpu *c = x->c;
    push(x, c->bp);
    c->bp = c->sp;
    c->sp = arithmetic(c, c->sp, 4u, 16u, 1);
    w8(x, c->ss, BP(-2), 0u);
    w8(x, c->ss, BP(-1), 0u);
    w8(x, c->ss, BP(-3), 0u);
    for (;;) {
        if (!tick(x, 0x0C71u)) return PR_U5E_263D_LIMIT;
        inc_local(x, BP(-3));
        set_al(c, r8(x, c->ss, BP(-3)));
        xor_ah(c);
        times_four(c);
        c->ax = r16(x, c->ds, (uint16_t)(c->di + 0x65B9u));
        c->dx = r16(x, c->ds, (uint16_t)(c->di + 0x65BBu));
        (void)arithmetic(c, c->dx, r16(x, c->ss, BP(8)), 16u, 1);
        if ((c->flags & 0x40u) != 0u) {
            (void)arithmetic(c, c->ax, r16(x, c->ss, BP(6)), 16u, 1);
            if ((c->flags & 0x40u) != 0u) w8(x, c->ss, BP(-2), 1u);
        }
        cmp8(c, r8(x, c->ss, BP(-2)), 0u);
        if ((c->flags & 0x40u) == 0u) break;
        set_al(c, r8(x, c->ss, BP(-3)));
        cmp8(c, (uint8_t)c->ax, r8(x, c->ds, 0x5F2Au));
        if ((c->flags & 0x40u) != 0u) break;
    }
    cmp8(c, r8(x, c->ss, BP(-2)), 0u);
    if ((c->flags & 0x40u) == 0u) {
        set_al(c, r8(x, c->ss, BP(-3)));
        w8(x, c->ss, BP(-1), (uint8_t)c->ax);
    }
    set_al(c, r8(x, c->ss, BP(-1)));
    leave(x, 4u);
    return PR_U5E_263D_OK;
}

/* Entries 12/13 have identical bodies apart from their code offsets and
 * table byte. PUSH CS + near CALL creates the real far-return frame. */
static pr_u5e_263d_status read_slot(context *x, int second)
{
    pr_u5e_263d_cpu *c = x->c;
    uint16_t cs = c->cs, return_ip = second ? 0x0C1Fu : 0x0BF7u;
    pr_u5e_263d_status status;
    push(x, c->bp);
    c->bp = c->sp;
    c->sp = arithmetic(c, c->sp, 2u, 16u, 1);
    push(x, r16(x, c->ss, BP(8)));
    push(x, r16(x, c->ss, BP(6)));
    push(x, cs);
    push(x, return_ip);
    c->ip = 0x0C5Fu;
    status = find_slot(x);
    if (status != PR_U5E_263D_OK) return status;
    if (c->cs != cs || c->ip != return_ip) return PR_U5E_263D_TRANSFER;
    xor_ah(c);
    times_four(c);
    set_al(c, r8(x, c->ds, (uint16_t)(c->di + (second ? 0x5F28u : 0x5F27u))));
    w8(x, c->ss, BP(-1), (uint8_t)c->ax);
    set_al(c, r8(x, c->ss, BP(-1)));
    leave(x, 4u);
    return PR_U5E_263D_OK;
}

static int stack_overlaps_code(const pr_u5e_263d_cpu *c,
                               const pr_u5e_263d_layout *layout)
{
    const uint16_t segments[] = {c->cs, layout->slots_cs,
                                 layout->slots_cs, layout->predicate_cs};
    const uint16_t starts[] = {0x263Du, 0x0BE7u, 0x0C5Fu, 0x057Bu};
    const uint16_t ends[] = {0x26B0u, 0x0C37u, 0x0CB9u, 0x0909u};
    unsigned i, j;
    /* Deepest frame is the nested slot search, 34 bytes below entry SP.
     * All writes, including predicate argument FF->8, fall in this span. */
    for (i = 1u; i <= 34u; ++i) {
        size_t p = physical(c->ss, (uint16_t)(c->sp - i));
        for (j = 0; j < 4u; ++j)
            if (((p - physical(segments[j], starts[j])) & MASK) <
                (size_t)(ends[j] - starts[j])) return 1;
    }
    return 0;
}

pr_u5e_263d_status pr_u5e_direction_263d(
    uint8_t *ram, size_t ram_size, pr_u5e_263d_cpu *c,
    const pr_u5e_263d_layout *layout, size_t max_iterations)
{
    context state, *x = &state;
    uint16_t source_cs;
    unsigned i;
    pr_u5e_263d_status status;
    if (ram == NULL || c == NULL || layout == NULL ||
        ram_size != PR_U5E_263D_RAM_SIZE || c->ip != 0x263Du ||
        max_iterations == 0u || c->sp < 34u || c->sp > 0xFFF4u)
        return PR_U5E_263D_ARGUMENT;
    if (stack_overlaps_code(c, layout)) return PR_U5E_263D_CODE_ALIAS;
    /* Read the required loaded 0xEA bindings before CPU/RAM mutation. */
    if (!(pr_u5e_263d_stubs_loaded(ram, c->cs, layout->slots_cs, layout->predicate_cs)))
        return PR_U5E_263D_ARGUMENT;
    state.ram = ram;
    state.c = c;
    state.remaining = max_iterations;
    source_cs = c->cs;

    push(x, c->bp);
    c->bp = c->sp;
    c->sp = arithmetic(c, c->sp, 6u, 16u, 1);
    for (i = 0u; i < 4u; ++i) {
        uint16_t argument = i < 2u ? 10u : 6u;
        uint16_t return_ip = (uint16_t)(0x264Eu + 14u * i);
        push(x, r16(x, c->ss, BP(argument + 2u)));
        push(x, r16(x, c->ss, BP(argument)));
        push(x, source_cs);
        push(x, return_ip);
        c->cs = layout->slots_cs;
        c->ip = (i & 1u) != 0u ? 0x0C0Fu : 0x0BE7u;
        status = read_slot(x, (int)(i & 1u));
        if (status != PR_U5E_263D_OK) return status;
        if (c->cs != source_cs || c->ip != return_ip) return PR_U5E_263D_TRANSFER;
        w8(x, c->ss, (uint16_t)(c->bp - 3u - i), (uint8_t)c->ax);
    }
    w8(x, c->ss, BP(-2), 0u);
    for (;;) {
        /* Carry the predicate's entire CPU/RAM result across the far return. */
        pr_u3c2_057b_cpu predicate;
        if (!tick(x, 0x267Fu)) return PR_U5E_263D_LIMIT;
        for (i = 3u; i <= 6u; ++i) {
            set_al(c, r8(x, c->ss, (uint16_t)(c->bp - i)));
            push(x, c->ax);
        }
        set_al(c, r8(x, c->ss, BP(-2)));
        push(x, c->ax);
        push(x, source_cs);
        push(x, 0x2698u);
        c->cs = layout->predicate_cs;
        c->ip = 0x057Bu;
        _Static_assert(sizeof predicate == sizeof *c, "predicate CPU layout");
        memcpy(&predicate, c, sizeof predicate);
        if (pr_u3c2_entry4_057b(ram, ram_size, &predicate) != 0) {
            memcpy(c, &predicate, sizeof *c);
            return PR_U5E_263D_CALLEE;
        }
        memcpy(c, &predicate, sizeof *c);
        if (c->cs != source_cs || c->ip != 0x2698u) return PR_U5E_263D_TRANSFER;
        logic8(c, (uint8_t)c->ax);
        if ((c->flags & 0x40u) == 0u) break;
        inc_local(x, BP(-2));
    }
    set_al(c, r8(x, c->ss, BP(-2)));
    w8(x, c->ss, BP(-1), (uint8_t)c->ax);
    set_al(c, r8(x, c->ss, BP(-1)));
    leave(x, 8u);
    return PR_U5E_263D_OK;
}
