#include "overlay_loaded_contracts.h"
/* Original U5E [2574,263D); see docs/re/U5E_DIRECTION_MATCH_2574.md. */
#include "poolrad/recovered/overlay_u5e_direction_match_2574.h"

#define MASK (PR_U5E_263D_RAM_SIZE - 1u)
#define ARITH 0x08D5u
#define BP(n) ((uint16_t)(c->bp + (n)))

typedef struct {
    uint8_t *ram;
    pr_u5e_263d_cpu *c;
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

static void set_al(pr_u5e_263d_cpu *c, uint8_t value)
{
    c->ax = (uint16_t)((c->ax & 0xFF00u) | value);
}

static void w16(context *x, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t p = physical(segment, offset);
    x->ram[p] = (uint8_t)value;
    x->ram[(p + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void logic16(pr_u5e_263d_cpu *c, uint16_t value)
{
    uint16_t f = parity((uint8_t)value);
    if (value == 0u) f |= 0x40u;
    if ((value & 0x8000u) != 0u) f |= 0x80u;
    c->flags = (uint16_t)((c->flags & (uint16_t)~ARITH) | f);
}

static void les(context *x, uint16_t segment, uint16_t offset)
{
    size_t p = physical(segment, offset);
    x->c->di = (uint16_t)(x->ram[p] | ((uint16_t)x->ram[(p + 1u) & MASK] << 8));
    x->c->es = (uint16_t)(x->ram[(p + 2u) & MASK] |
                         ((uint16_t)x->ram[(p + 3u) & MASK] << 8));
}

static void first_link(context *x)
{
    les(x, x->c->ss, (uint16_t)(x->c->bp + 6u));
    les(x, x->c->es, (uint16_t)(x->c->di + 0x108u));
}

static int stack_code_alias(const pr_u5e_263d_cpu *c,
                            const pr_u5e_263d_layout *layout)
{
    const uint16_t segments[] = {c->cs, layout->slots_cs,
                                 layout->slots_cs, layout->predicate_cs};
    const uint16_t starts[] = {0x2574u, 0x0BE7u, 0x0C5Fu, 0x057Bu};
    const uint16_t ends[] = {0x26B0u, 0x0C37u, 0x0CB9u, 0x0909u};
    unsigned i, j;
    for (i = 1u; i <= 60u; ++i) {
        size_t p = physical(c->ss, (uint16_t)(c->sp - i));
        for (j = 0u; j < 4u; ++j)
            if (((p - physical(segments[j], starts[j])) & MASK) <
                (size_t)(ends[j] - starts[j])) return 1;
    }
    return 0;
}

pr_u5e_263d_status pr_u5e_direction_match_2574(
    uint8_t *ram, size_t ram_size, pr_u5e_263d_cpu *c,
    const pr_u5e_263d_layout *layout, size_t max_iterations)
{
    context state, *x = &state;
    uint16_t caller_cs;
    pr_u5e_263d_status status;
    if (ram == NULL || c == NULL || layout == NULL ||
        ram_size != PR_U5E_263D_RAM_SIZE || c->ip != 0x2574u ||
        c->sp < 60u || c->sp > 0xFFF4u || max_iterations == 0u)
        return PR_U5E_263D_ARGUMENT;
    if (stack_code_alias(c, layout)) return PR_U5E_263D_CODE_ALIAS;
    /* Read the required loaded 0xEA bindings before CPU/RAM mutation. */
    if (!(pr_u5e_263d_stubs_loaded(ram, c->cs, layout->slots_cs, layout->predicate_cs)))
        return PR_U5E_263D_ARGUMENT;
    state.ram = ram;
    state.c = c;
    caller_cs = c->cs;
    push(x, c->bp);                         /* 2574 */
    c->bp = c->sp;
    c->sp = arithmetic(c, c->sp, 12u, 16u, 1);
    w8(x, c->ss, BP(-11), 0u);
    les(x, c->ss, BP(10));
    c->ax = r16(x, c->es, (uint16_t)(c->di + 0xCCu));
    c->dx = r16(x, c->es, (uint16_t)(c->di + 0xCEu));
    w16(x, c->ss, BP(-5), c->ax);
    w16(x, c->ss, BP(-3), c->dx);
    c->ax = r16(x, c->ss, BP(-5));
    c->ax |= r16(x, c->ss, BP(-3));
    logic16(c, c->ax);
    if ((c->flags & 0x40u) == 0u) {
        les(x, c->ss, BP(-5));
        set_al(c, r8(x, c->es, (uint16_t)(c->di + 0x2Eu)));
        w8(x, c->ss, BP(-10), (uint8_t)c->ax);
    }
    les(x, c->ss, BP(10));                /* 25A3 */
    c->ax = r16(x, c->es, (uint16_t)(c->di + 0xD4u));
    c->dx = r16(x, c->es, (uint16_t)(c->di + 0xD6u));
    w16(x, c->ss, BP(-9), c->ax);
    w16(x, c->ss, BP(-7), c->dx);
    les(x, c->ss, BP(10));
    (void)arithmetic(c, r8(x, c->es, (uint16_t)(c->di + 0x9Cu)), 0u, 8u, 1);
    if ((c->flags & 0x40u) != 0u ||
        ((c->flags >> 7) & 1u) != ((c->flags >> 11) & 1u)) goto gate;
    c->ax = r16(x, c->ss, BP(-9));         /* 25C1 */
    c->ax |= r16(x, c->ss, BP(-7));
    logic16(c, c->ax);
    if ((c->flags & 0x40u) == 0u) {
        les(x, c->ss, BP(-9));
        (void)arithmetic(c, r8(x, c->es, (uint16_t)(c->di + 0x2Eu)), 0x32u, 8u, 1);
        if ((c->flags & 0x40u) == 0u) goto gate;
    }
    c->ax = r16(x, c->ss, BP(-5));         /* 25D3 */
    c->ax |= r16(x, c->ss, BP(-3));
    logic16(c, c->ax);
    if ((c->flags & 0x40u) != 0u) goto accept;
    (void)arithmetic(c, r8(x, c->ss, BP(-10)), 7u, 8u, 1);
    if ((c->flags & 0x40u) != 0u) goto accept;
    (void)arithmetic(c, r8(x, c->ss, BP(-10)), 8u, 8u, 1);
    if ((c->flags & 0x40u) != 0u) goto accept;
    (void)arithmetic(c, r8(x, c->ss, BP(-10)), 0x22u, 8u, 1);
    if ((c->flags & 0x41u) != 0u) goto gate;
    (void)arithmetic(c, r8(x, c->ss, BP(-10)), 0x26u, 8u, 1);
    if ((c->flags & 1u) == 0u) goto gate;
accept:
    w8(x, c->ss, BP(-11), 1u);            /* 25F3 */
gate:
    (void)arithmetic(c, r8(x, c->ss, BP(-11)), 0u, 8u, 1);
    if ((c->flags & 0x40u) != 0u) goto reject;
    first_link(x);
    (void)arithmetic(c, r8(x, c->es, (uint16_t)(c->di + 0x0Fu)), 1u, 8u, 1);
    if ((c->flags & 0x41u) != 0u) goto reject;
    push(x, r16(x, c->ss, BP(12)));       /* 260C */
    push(x, r16(x, c->ss, BP(10)));
    push(x, r16(x, c->ss, BP(8)));
    push(x, r16(x, c->ss, BP(6)));
    push(x, c->cs);
    push(x, 0x261Cu);
    c->ip = 0x263Du;
    status = pr_u5e_direction_263d(ram, ram_size, c, layout, max_iterations);
    if (status != PR_U5E_263D_OK) return status;
    if (c->cs != caller_cs || c->ip != 0x261Cu) return PR_U5E_263D_TRANSFER;
    first_link(x);                       /* 261C */
    (void)arithmetic(c, (uint8_t)c->ax,
                     r8(x, c->es, (uint16_t)(c->di + 9u)), 8u, 1);
    if ((c->flags & 0x40u) == 0u) goto reject;
    w8(x, c->ss, BP(-1), 1u);
    goto result;
reject:
    w8(x, c->ss, BP(-1), 0u);             /* 2630 */
result:
    set_al(c, r8(x, c->ss, BP(-1)));
    leave(x, 8u);
    return PR_U5E_263D_OK;
}
