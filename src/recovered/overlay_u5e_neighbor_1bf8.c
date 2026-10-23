/* GAME.OVR U5E [1BF8,1E16); docs/re/U5E_NEIGHBOR_1BF8.md.
 * Full-state slot helpers reused from overlay_u5e_direction_263d.c.
 * All frame/table/output accesses remain live and source ordered. */
#include "poolrad/recovered/overlay_u5e_neighbor_1bf8.h"
#include "poolrad/recovered/overlay_u3c6_04be_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"

#include <string.h>

#define MASK (PR_U5E_1BF8_RAM_SIZE - 1u)
#define ARITH 0x08D5u
#define BP(n) ((uint16_t)(x->c->bp + (n)))

typedef struct {
    uint8_t *ram;
    pr_u5e_1bf8_cpu *c;
    size_t remaining;
    uint16_t source_cs;
    const pr_u5e_1bf8_layout *layout;
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

static uint16_t arithmetic(pr_u5e_1bf8_cpu *c, uint16_t a, uint16_t b,
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

static void cmp8(pr_u5e_1bf8_cpu *c, uint8_t a, uint8_t b)
{
    (void)arithmetic(c, a, b, 8u, 1);
}

static void set_al(pr_u5e_1bf8_cpu *c, uint8_t value)
{
    c->ax = (uint16_t)((c->ax & 0xFF00u) | value);
}

static void logic8(pr_u5e_1bf8_cpu *c, uint8_t result)
{
    uint16_t f = parity(result);
    if (result == 0u) f |= 0x40u;
    if ((result & 0x80u) != 0u) f |= 0x80u;
    c->flags = (uint16_t)((c->flags & (uint16_t)~ARITH) | f);
}

static void xor_ah(pr_u5e_1bf8_cpu *c)
{
    c->ax &= 0xFFu;
    logic8(c, 0u); /* XOR AH,AH, not XOR AX,AX. */
}

static void times_four(pr_u5e_1bf8_cpu *c)
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
static pr_u5e_1bf8_status find_slot(context *x)
{
    pr_u5e_1bf8_cpu *c = x->c;
    push(x, c->bp);
    c->bp = c->sp;
    c->sp = arithmetic(c, c->sp, 4u, 16u, 1);
    w8(x, c->ss, BP(-2), 0u);
    w8(x, c->ss, BP(-1), 0u);
    w8(x, c->ss, BP(-3), 0u);
    for (;;) {
        if (!tick(x, 0x0C71u)) return PR_U5E_1BF8_LIMIT;
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
    return PR_U5E_1BF8_OK;
}

/* Entries 12/13 have identical bodies apart from their code offsets and
 * table byte. PUSH CS + near CALL creates the real far-return frame. */
static pr_u5e_1bf8_status read_slot(context *x, int second)
{
    pr_u5e_1bf8_cpu *c = x->c;
    uint16_t cs = c->cs, return_ip = second ? 0x0C1Fu : 0x0BF7u;
    pr_u5e_1bf8_status status;
    push(x, c->bp);
    c->bp = c->sp;
    c->sp = arithmetic(c, c->sp, 2u, 16u, 1);
    push(x, r16(x, c->ss, BP(8)));
    push(x, r16(x, c->ss, BP(6)));
    push(x, cs);
    push(x, return_ip);
    c->ip = 0x0C5Fu;
    status = find_slot(x);
    if (status != PR_U5E_1BF8_OK) return status;
    if (c->cs != cs || c->ip != return_ip) return PR_U5E_1BF8_TRANSFER;
    xor_ah(c);
    times_four(c);
    set_al(c, r8(x, c->ds, (uint16_t)(c->di + (second ? 0x5F28u : 0x5F27u))));
    w8(x, c->ss, BP(-1), (uint8_t)c->ax);
    set_al(c, r8(x, c->ss, BP(-1)));
    leave(x, 4u);
    return PR_U5E_1BF8_OK;
}

static void w16(context *x, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t p = physical(segment, offset);
    x->ram[p] = (uint8_t)value;
    x->ram[(p + 1u) & MASK] = (uint8_t)(value >> 8);
}

static int protected_byte(const context *x, size_t p)
{
    const uint16_t segments[] = {x->source_cs, x->layout->slots_cs,
        x->layout->slots_cs, x->layout->slots_cs, x->layout->system_cs};
    const uint16_t starts[] = {0x1BD8u, 0x04BEu, 0x0BE7u, 0x0C5Fu, 0x0DC0u};
    const uint16_t ends[] = {0x1E16u, 0x052Bu, 0x0C37u, 0x0CB9u, 0x0DE1u};
    unsigned i;
    for (i = 0u; i < 5u; ++i)
        if (((p - physical(segments[i], starts[i])) & MASK) <
            (size_t)(ends[i] - starts[i])) return 1;
    return 0;
}

static int output_word(context *x, uint16_t offset, uint16_t value, uint16_t ip)
{
    size_t p = physical(x->c->es, offset);
    x->c->ip = ip;
    if (protected_byte(x, p) || protected_byte(x, (p + 1u) & MASK)) return 0;
    w16(x, x->c->es, offset, value);
    return 1;
}

static void les(context *x, uint16_t segment, uint16_t offset)
{
    size_t p = physical(segment, offset), q = (p + 2u) & MASK;
    uint16_t di = r16(x, segment, offset);
    x->c->es = (uint16_t)(x->ram[q] | ((uint16_t)x->ram[(q + 1u) & MASK] << 8));
    x->c->di = di;
}

static void cbw(pr_u5e_1bf8_cpu *c)
{
    c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax;
}

static void logic16(pr_u5e_1bf8_cpu *c, uint16_t value)
{
    uint16_t f = parity((uint8_t)value);
    if (value == 0u) f |= 0x40u;
    if ((value & 0x8000u) != 0u) f |= 0x80u;
    c->flags = (uint16_t)((c->flags & (uint16_t)~ARITH) | f);
}

static void mul_dx(pr_u5e_1bf8_cpu *c)
{
    uint32_t value = (uint32_t)c->ax * c->dx;
    uint16_t f = c->flags & 0x90u;
    c->ax = (uint16_t)value;
    c->dx = (uint16_t)(value >> 16);
    f |= parity((uint8_t)(value ^ (value >> 8) ^ (value >> 16) ^ (value >> 24)));
    if (c->ax == 0u) f |= 0x40u;
    if (c->dx != 0u) f |= 0x801u;
    c->flags = (uint16_t)((c->flags & (uint16_t)~ARITH) | f);
}

static void shr_ax(pr_u5e_1bf8_cpu *c)
{
    uint16_t old = c->ax;
    c->ax >>= 1;
    logic16(c, c->ax);
    c->flags |= 0x10u; /* pinned: AF set for a non-zero count */
    if ((old & 1u) != 0u) c->flags |= 1u;
    if ((old & 0x8000u) != 0u) c->flags |= 0x800u; /* stored flags (FillFlags): SHRw count 1 OF = (var1 >= 8000h) */
}

static void candidate(context *x)
{
    les(x, x->c->ss, BP(6));
    les(x, x->c->es, x->c->di);
}

static void record_index(context *x)
{
    set_al(x->c, r8(x, x->c->ss, BP(-11)));
    xor_ah(x->c);
    x->c->dx = 7u;
    mul_dx(x->c);
    x->c->di = x->c->ax;
}

static pr_u5e_1bf8_status slot_call(context *x, int second)
{
    pr_u5e_1bf8_status status;
    uint16_t ret = second ? 0x1C41u : 0x1C22u;
    push(x, r16(x, x->c->ss, BP(12)));
    push(x, r16(x, x->c->ss, BP(10)));
    push(x, x->source_cs);
    push(x, ret);
    x->c->cs = x->layout->slots_cs;
    x->c->ip = second ? 0x0C0Fu : 0x0BE7u;
    status = read_slot(x, second);
    if (status != PR_U5E_1BF8_OK) return status;
    if (x->c->cs != x->source_cs || x->c->ip != ret) return PR_U5E_1BF8_TRANSFER;
    return PR_U5E_1BF8_OK;
}

/* Field-wise copies keep the public CPU types distinct without type punning. */
#define COPY_CPU(dst, src) do { \
    (dst)->ax=(src)->ax; (dst)->bx=(src)->bx; (dst)->cx=(src)->cx; \
    (dst)->dx=(src)->dx; (dst)->si=(src)->si; (dst)->di=(src)->di; \
    (dst)->bp=(src)->bp; (dst)->sp=(src)->sp; (dst)->cs=(src)->cs; \
    (dst)->ip=(src)->ip; (dst)->ds=(src)->ds; (dst)->es=(src)->es; \
    (dst)->ss=(src)->ss; (dst)->flags=(src)->flags; \
} while (0)

static pr_u5e_1bf8_status cell_call(context *x)
{
    pr_u3c6_04be_cpu cell;
    pr_u3c6_04be_cpu_status status;
    push(x, x->source_cs);
    push(x, 0x1C6Cu);
    x->c->cs = x->layout->slots_cs;
    x->c->ip = 0x04BEu;
    COPY_CPU(&cell, x->c);
    status = pr_u3c6_04be_cpu_run(x->ram, PR_U5E_1BF8_RAM_SIZE, &cell,
                                 PR_U3C6_04BE_RETF_CONTIGUOUS_PHYSICAL);
    COPY_CPU(x->c, &cell);
    if (status != PR_U3C6_04BE_CPU_RETURNED) return PR_U5E_1BF8_CALLEE;
    if (x->c->cs != x->source_cs || x->c->ip != 0x1C6Cu) return PR_U5E_1BF8_TRANSFER;
    return PR_U5E_1BF8_OK;
}

static pr_u5e_1bf8_status set_call(context *x)
{
    pr_system_leaf_cpu leaf;
    pr_system_leaf_status status;
    push(x, x->source_cs);
    push(x, 0x1D78u);
    x->c->cs = x->layout->system_cs;
    x->c->ip = 0x0DC0u;
    COPY_CPU(&leaf, x->c);
    status = pr_system_set_in_0dc0(x->ram, PR_U5E_1BF8_RAM_SIZE, &leaf);
    COPY_CPU(x->c, &leaf);
    if (status != PR_SYSTEM_LEAF_RETURNED) return PR_U5E_1BF8_CALLEE;
    if (x->c->cs != x->source_cs || x->c->ip != 0x1D78u) return PR_U5E_1BF8_TRANSFER;
    return PR_U5E_1BF8_OK;
}

pr_u5e_1bf8_status pr_u5e_neighbor_1bf8(
    uint8_t *ram, size_t ram_size, pr_u5e_1bf8_cpu *c,
    const pr_u5e_1bf8_layout *layout, size_t max_iterations)
{
    context state, *x = &state;
    pr_u5e_1bf8_status status;
    unsigned i;
    if (ram == NULL || c == NULL || layout == NULL ||
        ram_size != PR_U5E_1BF8_RAM_SIZE || c->ip != 0x1BF8u ||
        max_iterations == 0u || c->sp < 42u || c->sp > 0xFFF4u)
        return PR_U5E_1BF8_ARGUMENT;
    state.ram = ram;
    state.c = c;
    state.remaining = max_iterations;
    state.source_cs = c->cs;
    state.layout = layout;
    for (i = 1u; i <= 42u; ++i)
        if (protected_byte(x, physical(c->ss, (uint16_t)(c->sp - i))))
            return PR_U5E_1BF8_CODE_ALIAS;

    push(x, c->bp);                                                    /* 1BF8 */
    c->bp = c->sp;
    c->sp = arithmetic(c, c->sp, 14u, 16u, 1);
    c->ax = 0u;
    logic16(c, 0u);
    w16(x, c->ss, BP(-5), c->ax);
    w16(x, c->ss, BP(-3), c->ax);
    w8(x, c->ss, BP(-8), 0u);
    w8(x, c->ss, BP(-9), 0xFFu);
    w8(x, c->ss, BP(-10), 0u);
    for (;;) {
        if (!tick(x, 0x1C17u)) return PR_U5E_1BF8_LIMIT;
        status = slot_call(x, 0);                                      /* 1C17..1C1D */
        if (status != PR_U5E_1BF8_OK) return status;
        cbw(c);
        c->dx = c->ax;
        set_al(c, r8(x, c->ss, BP(-10)));
        xor_ah(c);
        c->di = c->ax;
        set_al(c, r8(x, c->ds, (uint16_t)(c->di + 0x2736u)));
        cbw(c);
        c->ax = arithmetic(c, c->ax, c->dx, 16u, 0);
        w8(x, c->ss, BP(-6), (uint8_t)c->ax);
        status = slot_call(x, 1);                                      /* 1C36..1C3C */
        if (status != PR_U5E_1BF8_OK) return status;
        cbw(c);
        c->dx = c->ax;
        set_al(c, r8(x, c->ss, BP(-10)));
        xor_ah(c);
        c->di = c->ax;
        set_al(c, r8(x, c->ds, (uint16_t)(c->di + 0x273Fu)));
        cbw(c);
        c->ax = arithmetic(c, c->ax, c->dx, 16u, 0);
        w8(x, c->ss, BP(-7), (uint8_t)c->ax);
        set_al(c, r8(x, c->ss, BP(-6)));                                /* 1C55 */
        push(x, c->ax);
        set_al(c, r8(x, c->ss, BP(-7)));
        push(x, c->ax);
        c->di = BP(-12);
        push(x, c->ss);
        push(x, c->di);
        c->di = BP(-13);
        push(x, c->ss);
        push(x, c->di);
        status = cell_call(x);
        if (status != PR_U5E_1BF8_OK) return status;

        cmp8(c, r8(x, c->ss, BP(-12)), 0u);                            /* 1C6C */
        if ((c->flags & 0x41u) == 0u) {
            set_al(c, r8(x, c->ss, BP(-12)));
            xor_ah(c);
            times_four(c);
            c->ax = r16(x, c->ds, (uint16_t)(c->di + 0x65B9u));
            c->dx = r16(x, c->ds, (uint16_t)(c->di + 0x65BBu));
            les(x, c->ss, BP(6));
            if (!output_word(x, c->di, c->ax, 0x1C8Bu)) return PR_U5E_1BF8_CODE_ALIAS;
            if (!output_word(x, (uint16_t)(c->di + 2u), c->dx, 0x1C8Eu)) return PR_U5E_1BF8_CODE_ALIAS;
            candidate(x);                                             /* 1C92 */
            set_al(c, r8(x, c->es, (uint16_t)(c->di + 0x10Eu)));
            les(x, c->ss, BP(10));
            cmp8(c, (uint8_t)c->ax, r8(x, c->es, (uint16_t)(c->di + 0x10Eu)));
            if ((c->flags & 0x40u) == 0u) goto next_neighbor;
            candidate(x);                                             /* 1CA7 */
            set_al(c, r8(x, c->es, (uint16_t)(c->di + 0x11Bu)));
            candidate(x);
            cmp8(c, (uint8_t)c->ax, r8(x, c->es, (uint16_t)(c->di + 0x32u)));
            if ((c->flags & 1u) == 0u) goto next_neighbor;
            candidate(x);
            set_al(c, r8(x, c->es, (uint16_t)(c->di + 0x11Bu)));
            cmp8(c, (uint8_t)c->ax, r8(x, c->ss, BP(-9)));
            if ((c->flags & 1u) != 0u) goto choose_candidate;
            les(x, c->ss, BP(6));                                      /* 1CCE */
            c->ax = r16(x, c->es, c->di);
            c->dx = r16(x, c->es, (uint16_t)(c->di + 2u));
            (void)arithmetic(c, c->dx, r16(x, c->ss, BP(12)), 16u, 1);
            if ((c->flags & 0x40u) == 0u) goto next_neighbor;
            (void)arithmetic(c, c->ax, r16(x, c->ss, BP(10)), 16u, 1);
            if ((c->flags & 0x40u) == 0u) goto next_neighbor;
            candidate(x);                                             /* 1CE2 */
            set_al(c, r8(x, c->es, (uint16_t)(c->di + 0x32u)));
            xor_ah(c);
            shr_ax(c);
            c->dx = c->ax;
            candidate(x);
            set_al(c, r8(x, c->es, (uint16_t)(c->di + 0x11Bu)));
            xor_ah(c);
            (void)arithmetic(c, c->ax, c->dx, 16u, 1);
            if (((c->flags & 0x80u) != 0u) == ((c->flags & 0x800u) != 0u))
                goto next_neighbor;                                   /* 1D01 JGE */
choose_candidate:
            les(x, c->ss, BP(6));                                      /* 1D03 */
            c->ax = r16(x, c->es, c->di);
            c->dx = r16(x, c->es, (uint16_t)(c->di + 2u));
            w16(x, c->ss, BP(-5), c->ax);
            w16(x, c->ss, BP(-3), c->dx);
            candidate(x);
            set_al(c, r8(x, c->es, (uint16_t)(c->di + 0x11Bu)));
            w8(x, c->ss, BP(-9), (uint8_t)c->ax);
            goto next_neighbor;
        }

        cmp8(c, r8(x, c->ss, BP(-13)), 0x1Fu);                         /* 1D24 */
        if ((c->flags & 0x40u) == 0u) goto next_neighbor;
        set_al(c, r8(x, c->ds, 0x6715u));
        w8(x, c->ss, BP(-14), (uint8_t)c->ax);
        set_al(c, 1u);
        cmp8(c, (uint8_t)c->ax, r8(x, c->ss, BP(-14)));
        if ((c->flags & 0x41u) == 0u) goto next_neighbor;
        w8(x, c->ss, BP(-11), (uint8_t)c->ax);
        for (;;) {
            if (!tick(x, 0x1D42u)) return PR_U5E_1BF8_LIMIT;
            record_index(x);
            c->ax = r16(x, c->ds, (uint16_t)(c->di + 0x66D6u));
            c->ax |= r16(x, c->ds, (uint16_t)(c->di + 0x66D8u));
            logic16(c, c->ax);
            if ((c->flags & 0x40u) != 0u) goto next_record;
            record_index(x);                                          /* 1D58 */
            les(x, c->ds, (uint16_t)(c->di + 0x66D6u));
            set_al(c, r8(x, c->es, (uint16_t)(c->di + 0x10Cu)));
            push(x, c->ax);
            c->di = 0x1BD8u;
            push(x, c->cs);
            push(x, c->di);
            status = set_call(x);
            if (status != PR_U5E_1BF8_OK) return status;
            if ((c->flags & 0x40u) == 0u) goto next_record;
            record_index(x);                                          /* 1D7A */
            set_al(c, r8(x, c->ds, (uint16_t)(c->di + 0x66DAu)));
            cmp8(c, (uint8_t)c->ax, r8(x, c->ss, BP(-6)));
            if ((c->flags & 0x40u) == 0u) goto next_record;
            record_index(x);
            set_al(c, r8(x, c->ds, (uint16_t)(c->di + 0x66DBu)));
            cmp8(c, (uint8_t)c->ax, r8(x, c->ss, BP(-7)));
            if ((c->flags & 0x40u) == 0u) goto next_record;
            set_al(c, r8(x, c->ss, BP(-11)));
            w8(x, c->ss, BP(-8), (uint8_t)c->ax);
next_record:
            set_al(c, r8(x, c->ss, BP(-11)));
            cmp8(c, (uint8_t)c->ax, r8(x, c->ss, BP(-14)));
            if ((c->flags & 0x40u) != 0u) break;
            inc_local(x, BP(-11));
        }
next_neighbor:
        cmp8(c, r8(x, c->ss, BP(-10)), 8u);                            /* 1DB2 */
        if ((c->flags & 0x40u) != 0u) break;
        inc_local(x, BP(-10));
    }
    cmp8(c, r8(x, c->ss, BP(-9)), 8u);                                 /* 1DBB */
    if ((c->flags & 1u) != 0u) goto write_best;
    cmp8(c, r8(x, c->ss, BP(-8)), 0u);
    if ((c->flags & 0x40u) == 0u) goto write_record;
write_best:
    c->ax = r16(x, c->ss, BP(-5));
    c->dx = r16(x, c->ss, BP(-3));
    les(x, c->ss, BP(6));
    if (!output_word(x, c->di, c->ax, 0x1DD0u)) return PR_U5E_1BF8_CODE_ALIAS;
    if (!output_word(x, (uint16_t)(c->di + 2u), c->dx, 0x1DD3u)) return PR_U5E_1BF8_CODE_ALIAS;
    goto done;
write_record:
    set_al(c, r8(x, c->ss, BP(-8)));
    xor_ah(c);
    c->dx = 7u;
    mul_dx(c);
    c->di = c->ax;
    c->ax = r16(x, c->ds, (uint16_t)(c->di + 0x66D6u));
    c->dx = r16(x, c->ds, (uint16_t)(c->di + 0x66D8u));
    les(x, c->ss, BP(6));
    if (!output_word(x, c->di, c->ax, 0x1DF0u)) return PR_U5E_1BF8_CODE_ALIAS;
    if (!output_word(x, (uint16_t)(c->di + 2u), c->dx, 0x1DF3u)) return PR_U5E_1BF8_CODE_ALIAS;
done:
    les(x, c->ss, BP(6));
    c->ax = r16(x, c->es, c->di);
    c->ax |= r16(x, c->es, (uint16_t)(c->di + 2u));
    logic16(c, c->ax);
    w8(x, c->ss, BP(-1), (c->flags & 0x40u) != 0u ? 0u : 1u);
    set_al(c, r8(x, c->ss, BP(-1)));
    leave(x, 8u);
    return PR_U5E_1BF8_OK;
}
