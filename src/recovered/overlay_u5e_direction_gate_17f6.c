/* Original U5E [17F6,1884); see docs/re/U5E_DIRECTION_GATE_17F6.md. */
#include "poolrad/recovered/overlay_u5e_direction_gate_17f6.h"

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

/* LES reads both words before replacing either destination register. */
static void les(context *x, uint16_t segment, uint16_t offset)
{
    size_t p = physical(segment, offset);
    x->c->di = (uint16_t)(x->ram[p] | ((uint16_t)x->ram[(p + 1u) & MASK] << 8));
    x->c->es = (uint16_t)(x->ram[(p + 2u) & MASK] |
                         ((uint16_t)x->ram[(p + 3u) & MASK] << 8));
}

static void linked_record(context *x)
{
    les(x, x->c->ss, (uint16_t)(x->c->bp + 6u));
    les(x, x->c->es, (uint16_t)(x->c->di + 0x108u));
}

static void divide_eight(pr_u5e_263d_cpu *c)
{
    int32_t value = c->ax < 0x8000u ? c->ax : (int32_t)c->ax - 65536;
    /* CWD; MOV CX,8; IDIV CX; XCHG DX,AX. C99 truncates toward zero. */
    c->cx = 8u;
    c->dx = (uint16_t)(value / 8);
    c->ax = (uint16_t)(value % 8);
    /* IDIV CX. Pinned DIVW/IDIVW (instructions.h:655-673, 743-761) overwrite all six flags: AF=SF=OF=0,
     * ZF=(rem==0 && quo odd), CF=((rem&3) in 1..2), PF=even parity of the four bytes of rem:quo. */
    {
        uint16_t rem16 = (uint16_t)(value % 8), quo16 = (uint16_t)(value / 8), fold = (uint16_t)(rem16 ^ quo16);
        fold ^= (uint16_t)(fold >> 8); fold ^= (uint16_t)(fold >> 4); fold ^= (uint16_t)(fold >> 2); fold ^= (uint16_t)(fold >> 1);
        c->flags = (uint16_t)((c->flags & ~0x08D5u) | ((rem16 == 0u && (quo16 & 1u)) ? 0x0040u : 0u) |
                                (((rem16 & 3u) >= 1u && (rem16 & 3u) <= 2u) ? 0x0001u : 0u) | ((fold & 1u) ? 0u : 0x0004u));
    }
}

static int stack_code_alias(const pr_u5e_263d_cpu *c,
                            const pr_u5e_263d_layout *layout)
{
    const uint16_t segments[] = {c->cs, c->cs, layout->slots_cs,
                                 layout->slots_cs, layout->predicate_cs};
    const uint16_t starts[] = {0x17F6u, 0x263Du, 0x0BE7u, 0x0C5Fu, 0x057Bu};
    const uint16_t ends[] = {0x1884u, 0x26B0u, 0x0C37u, 0x0CB9u, 0x0909u};
    unsigned i, j;
    for (i = 1u; i <= 50u; ++i) {
        size_t p = physical(c->ss, (uint16_t)(c->sp - i));
        for (j = 0u; j < 5u; ++j)
            if (((p - physical(segments[j], starts[j])) & MASK) <
                (size_t)(ends[j] - starts[j])) return 1;
    }
    return 0;
}

pr_u5e_263d_status pr_u5e_direction_gate_17f6(
    uint8_t *ram, size_t ram_size, pr_u5e_263d_cpu *c,
    const pr_u5e_263d_layout *layout, size_t max_iterations)
{
    context state, *x = &state;
    uint16_t caller_cs, carry;
    uint8_t value;
    pr_u5e_263d_status status;
    if (ram == NULL || c == NULL || layout == NULL ||
        ram_size != PR_U5E_263D_RAM_SIZE || c->ip != 0x17F6u ||
        c->sp < 50u || c->sp > 0xFFF4u || max_iterations == 0u)
        return PR_U5E_263D_ARGUMENT;
    if (stack_code_alias(c, layout)) return PR_U5E_263D_CODE_ALIAS;
    state.ram = ram;
    state.c = c;
    caller_cs = c->cs;
    push(x, c->bp);                         /* 17F6 */
    c->bp = c->sp;
    c->sp = arithmetic(c, c->sp, 2u, 16u, 1);
    linked_record(x);                       /* 17FC,17FF */
    carry = c->flags & 1u;
    value = r8(x, c->es, (uint16_t)(c->di + 0x0Fu));
    value = (uint8_t)arithmetic(c, value, 1u, 8u, 0);
    c->flags = (uint16_t)((c->flags & (uint16_t)~1u) | carry);
    w8(x, c->es, (uint16_t)(c->di + 0x0Fu), value);
    push(x, r16(x, c->ss, BP(8)));           /* 1808 */
    push(x, r16(x, c->ss, BP(6)));
    push(x, r16(x, c->ss, BP(12)));
    push(x, r16(x, c->ss, BP(10)));
    push(x, c->cs);
    push(x, 0x1818u);
    c->ip = 0x263Du;
    status = pr_u5e_direction_263d(ram, ram_size, c, layout, max_iterations);
    if (status != PR_U5E_263D_OK) return status;
    if (c->cs != caller_cs || c->ip != 0x1818u) return PR_U5E_263D_TRANSFER;
    w8(x, c->ss, BP(-2), (uint8_t)c->ax);    /* 1818 */
    linked_record(x);
    set_al(c, r8(x, c->es, (uint16_t)(c->di + 9u)));
    xor_ah(c);
    c->dx = c->ax;
    set_al(c, r8(x, c->ss, BP(-2)));
    xor_ah(c);
    c->ax = arithmetic(c, c->ax, c->dx, 16u, 1);
    c->ax = arithmetic(c, c->ax, 8u, 16u, 0);
    divide_eight(c);                        /* 1835..183B */
    w8(x, c->ss, BP(-1), (uint8_t)c->ax);
    (void)arithmetic(c, r8(x, c->ss, BP(-1)), 4u, 8u, 1);
    if ((c->flags & 0x41u) == 0u) {          /* unsigned JA */
        set_al(c, r8(x, c->ss, BP(-1)));
        xor_ah(c);
        c->dx = c->ax;
        c->ax = 8u;
        c->ax = arithmetic(c, c->ax, c->dx, 16u, 1);
        w8(x, c->ss, BP(-1), (uint8_t)c->ax);
    }
    set_al(c, r8(x, c->ss, BP(-1)));         /* 1854 */
    xor_ah(c);
    c->dx = c->ax;
    linked_record(x);
    set_al(c, r8(x, c->es, (uint16_t)(c->di + 0x12u)));
    xor_ah(c);
    c->ax = arithmetic(c, c->ax, c->dx, 16u, 0);
    divide_eight(c);                        /* 186B..1871 */
    linked_record(x);
    w8(x, c->es, (uint16_t)(c->di + 0x12u), (uint8_t)c->ax);
    leave(x, 8u);
    return PR_U5E_263D_OK;
}
