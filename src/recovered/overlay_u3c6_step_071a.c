/* Complete original U3C6 +071A. Raw +0C5F and arithmetic primitives adapted
 * from overlay_u5e_direction_263d.c; provenance in U3C6_STEP_071A.md. */
#include "poolrad/recovered/overlay_u3c6_step_071a.h"
#include "poolrad/recovered/overlay_u3c6_table_pair_0000.h"
#include "poolrad/recovered/overlay_u3c6_in_range_06e9.h"

#define MASK (PR_U6C_3F31_RAM_SIZE - 1u)
#define ARITH 0x08D5u
#define BP(n) ((uint16_t)(c->bp + (n)))

typedef struct {
    uint8_t *ram;
    pr_u3c6_071a_cpu *c;
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

static uint16_t arithmetic(pr_u3c6_071a_cpu *c, uint16_t a, uint16_t b,
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

static void cmp8(pr_u3c6_071a_cpu *c, uint8_t a, uint8_t b)
{
    (void)arithmetic(c, a, b, 8u, 1);
}

static void set_al(pr_u3c6_071a_cpu *c, uint8_t value)
{
    c->ax = (uint16_t)((c->ax & 0xFF00u) | value);
}

static void logic8(pr_u3c6_071a_cpu *c, uint8_t result)
{
    uint16_t f = parity(result);
    if (result == 0u) f |= 0x40u;
    if ((result & 0x80u) != 0u) f |= 0x80u;
    c->flags = (uint16_t)((c->flags & (uint16_t)~ARITH) | f);
}

static void xor_ah(pr_u3c6_071a_cpu *c)
{
    c->ax &= 0xFFu;
    logic8(c, 0u); /* XOR AH,AH, not XOR AX,AX. */
}

static void times_four(pr_u3c6_071a_cpu *c)
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
static pr_u3c6_071a_status find_slot(context *x)
{
    pr_u3c6_071a_cpu *c = x->c;
    push(x, c->bp);
    c->bp = c->sp;
    c->sp = arithmetic(c, c->sp, 4u, 16u, 1);
    w8(x, c->ss, BP(-2), 0u);
    w8(x, c->ss, BP(-1), 0u);
    w8(x, c->ss, BP(-3), 0u);
    for (;;) {
        if (!tick(x, 0x0C71u)) return PR_U3C6_071A_LIMIT;
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
    return PR_U3C6_071A_OK;
}

static int stack_overlaps_code(const pr_u3c6_071a_cpu *c)
{
    static const uint16_t starts[] = {0, 0x06E9u, 0x071Au, 0x0C5Fu};
    static const uint16_t ends[] = {0x0068u, 0x071Au, 0x07D4u, 0x0CB9u};
    unsigned i, j, k;
    for (i = 1; i <= 30; ++i) {
        size_t at = physical(c->ss, (uint16_t)(c->sp - i));
        for (k = 0; k < 2; ++k)
            for (j = 0; j < 4; ++j) {
                size_t delta = ((at + k) - physical(c->cs, starts[j])) & MASK;
                if (delta < (size_t)(ends[j] - starts[j])) return 1;
            }
    }
    return 0;
}

static void cbw(pr_u3c6_071a_cpu *c)
{
    c->ax = (uint16_t)((c->ax & 0xFFu) |
                      ((c->ax & 0x80u) ? 0xFF00u : 0u));
}

pr_u3c6_071a_status pr_u3c6_step_071a(uint8_t *ram, size_t ram_size,
                                    pr_u3c6_071a_cpu *c, size_t budget)
{
    context ctx, *x = &ctx;
    uint16_t cs;
    pr_u3c6_071a_status status;
    if (!ram || !c || ram_size != PR_U6C_3F31_RAM_SIZE ||
        c->ip != 0x071Au || stack_overlaps_code(c))
        return PR_U3C6_071A_ARGUMENT;
    ctx.ram = ram; ctx.c = c; ctx.remaining = budget; cs = c->cs;
    push(x, c->bp);
    c->bp = c->sp;
    c->sp = arithmetic(c, c->sp, 8u, 16u, 1);
    w8(x, c->ss, BP(-1), 1u);
    push(x, r16(x, c->ss, BP(10)));
    push(x, r16(x, c->ss, BP(8)));
    push(x, cs); push(x, 0x072Eu); c->ip = 0x0C5Fu;
    status = find_slot(x);
    if (status != PR_U3C6_071A_OK) return status;
    if (c->cs != cs || c->ip != 0x072Eu) return PR_U3C6_071A_TRANSFER;
    w8(x, c->ss, BP(-3), (uint8_t)c->ax);
    set_al(c, r8(x, c->ss, BP(-3))); xor_ah(c); times_four(c);
    cmp8(c, r8(x, c->ds, (uint16_t)(c->di + 0x5F2Au)), 0);
    if (c->flags & 0x40u) {
        w8(x, c->ss, BP(-1), 0);
        goto done;
    }
    w8(x, c->ss, BP(-2), 0);
    for (;;) {
        if (!tick(x, 0x0753u)) return PR_U3C6_071A_LIMIT;
        set_al(c, r8(x, c->ss, BP(-3))); xor_ah(c); times_four(c);
        set_al(c, r8(x, c->ds, (uint16_t)(c->di + 0x5F2Au)));
        push(x, c->ax);
        set_al(c, r8(x, c->ss, BP(-2))); push(x, c->ax);
        c->di = BP(-4); push(x, c->ss); push(x, c->di);
        c->di = BP(-5); push(x, c->ss); push(x, c->di);
        push(x, cs); push(x, 0x0775u); c->ip = 0;
        if (pr_u3c6_table_pair_0000(ram, ram_size, c,
                PR_U3C6_0000_RETF_WRAPPED_SS) != PR_U3C6_0000_RETURNED)
            return PR_U3C6_071A_ARGUMENT;
        if (c->cs != cs || c->ip != 0x0775u) return PR_U3C6_071A_TRANSFER;
        logic8(c, (uint8_t)c->ax);
        if (!(c->flags & 0x40u)) {
            set_al(c, r8(x, c->ss, BP(-4))); cbw(c); c->dx = c->ax;
            set_al(c, r8(x, c->ss, BP(-3))); xor_ah(c); c->di = c->ax;
            set_al(c, r8(x, c->ds, (uint16_t)(c->di + 0x604Au))); cbw(c);
            c->ax = arithmetic(c, c->ax, c->dx, 16u, 0); push(x, c->ax);
            set_al(c, r8(x, c->ss, BP(-5))); cbw(c); c->dx = c->ax;
            set_al(c, r8(x, c->ss, BP(-3))); xor_ah(c); c->di = c->ax;
            set_al(c, r8(x, c->ds, (uint16_t)(c->di + 0x6092u))); cbw(c);
            c->ax = arithmetic(c, c->ax, c->dx, 16u, 0); push(x, c->ax);
            push(x, cs); push(x, 0x07A7u); c->ip = 0x06E9u;
            if (pr_u3c6_in_range_06e9(ram, ram_size, c) != PR_U3C6_06E9_OK)
                return PR_U3C6_071A_ARGUMENT;
            if (c->cs != cs || c->ip != 0x07A7u) return PR_U3C6_071A_TRANSFER;
            logic8(c, (uint8_t)c->ax);
            if (c->flags & 0x40u) {
                w8(x, c->ss, BP(-1), 0);
                cmp8(c, r8(x, c->ss, BP(6)), 0);
                if (!(c->flags & 0x40u)) break;
            } else {
                w8(x, c->ss, BP(-1), 1);
                cmp8(c, r8(x, c->ss, BP(6)), 0);
                if (c->flags & 0x40u) break;
            }
        }
        cmp8(c, r8(x, c->ss, BP(-2)), 3);
        if (c->flags & 0x40u) break;
        inc_local(x, BP(-2));
    }
done:
    set_al(c, r8(x, c->ss, BP(-1)));
    leave(x, 6u);
    return PR_U3C6_071A_OK;
}
