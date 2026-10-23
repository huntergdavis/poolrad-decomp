/* Original U3C2 [01A0,02CC); see docs/re/U3C2_LINE_INIT_01A0.md. */
#include "poolrad/recovered/overlay_u3c2_line_init_01a0.h"

#define MASK (PR_U3C2_0000_RAM_SIZE - 1u)
#define ARITH 0x08D5u
#define BP(n) ((uint16_t)(c->bp + (n)))

typedef struct {
    uint8_t *ram;
    pr_u3c2_01a0_cpu *c;
} context;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
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

static uint16_t arithmetic(pr_u3c2_01a0_cpu *c, uint16_t a, uint16_t b,
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

static void w16(context *x, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t p = physical(segment, offset);
    x->ram[p] = (uint8_t)value;
    x->ram[(p + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void logic16(pr_u3c2_01a0_cpu *c, uint16_t value)
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

static void record(context *x)
{
    les(x, x->c->ss, (uint16_t)(x->c->bp - 4u));
}

static uint16_t record16(context *x, uint16_t offset)
{
    record(x);
    return r16(x, x->c->es, (uint16_t)(x->c->di + offset));
}

static void record_write(context *x, uint16_t offset, uint16_t value)
{
    record(x);
    w16(x, x->c->es, (uint16_t)(x->c->di + offset), value);
}

static void record_byte(context *x, uint16_t offset, uint8_t value)
{
    record(x);
    w8(x, x->c->es, (uint16_t)(x->c->di + offset), value);
}

static void absolute(context *x)
{
    pr_u3c2_01a0_cpu *c = x->c;
    c->dx = (c->ax & 0x8000u) != 0u ? 0xFFFFu : 0u; /* CWD */
    c->ax ^= c->dx;
    logic16(c, c->ax);
    c->ax = arithmetic(c, c->ax, c->dx, 16u, 1);
}

static void shift_ax(pr_u3c2_01a0_cpu *c)
{
    uint16_t old = c->ax;
    c->ax = (uint16_t)(old << 1);
    logic16(c, c->ax);
    c->flags |= 0x10u; /* pinned: SHL sets AF for a non-zero count */
    if ((old & 0x8000u) != 0u) c->flags |= 1u;
    if (((old ^ c->ax) & 0x8000u) != 0u) c->flags |= 0x800u;
}

static pr_u3c2_01a0_status sign_call(context *x, uint16_t return_ip)
{
    uint16_t cs = x->c->cs;
    push(x, x->c->ax);
    push(x, cs);
    push(x, return_ip);
    x->c->ip = 0u;
    if (pr_u3c2_sign_entry1_0000(x->ram, PR_U3C2_0000_RAM_SIZE, x->c) != PR_U3C2_0000_OK)
        return PR_U3C2_01A0_CALLEE;
    if (x->c->cs != cs || x->c->ip != return_ip) return PR_U3C2_01A0_TRANSFER;
    return PR_U3C2_01A0_OK;
}

static int stack_code_alias(const pr_u3c2_01a0_cpu *c)
{
    unsigned i;
    for (i = 1u; i <= 16u; ++i) {
        size_t p = physical(c->ss, (uint16_t)(c->sp - i));
        if (((p - physical(c->cs, 0u)) & MASK) < 0x2Eu ||
            ((p - physical(c->cs, 0x01A0u)) & MASK) < 0x12Cu) return 1;
    }
    return 0;
}

pr_u3c2_01a0_status pr_u3c2_line_init_01a0(
    uint8_t *ram, size_t ram_size, pr_u3c2_01a0_cpu *c)
{
    context state, *x = &state;
    pr_u3c2_01a0_status status;
    if (ram == NULL || c == NULL || ram_size != PR_U3C2_0000_RAM_SIZE ||
        c->ip != 0x01A0u || c->sp < 16u || c->sp > 0xFFF8u)
        return PR_U3C2_01A0_ARGUMENT;
    if (stack_code_alias(c)) return PR_U3C2_01A0_CODE_ALIAS;
    state.ram = ram;
    state.c = c;
    push(x, c->bp);                          /* 01A0 */
    c->bp = c->sp;
    c->sp = arithmetic(c, c->sp, 4u, 16u, 1);
    les(x, c->ss, BP(6));
    w16(x, c->ss, BP(-4), c->di);
    w16(x, c->ss, BP(-2), c->es);
    record_byte(x, 0x18u, 0u);              /* 01AF */
    c->ax = record16(x, 4u);
    c->ax = arithmetic(c, c->ax, record16(x, 0u), 16u, 1);
    absolute(x);
    record_write(x, 0x0Au, c->ax);
    c->ax = record16(x, 6u);
    c->ax = arithmetic(c, c->ax, record16(x, 2u), 16u, 1);
    absolute(x);
    record_write(x, 0x0Cu, c->ax);
    c->ax = record16(x, 4u);                /* 01EA */
    c->ax = arithmetic(c, c->ax, record16(x, 0u), 16u, 1);
    status = sign_call(x, 0x01FCu);
    if (status != PR_U3C2_01A0_OK) return status;
    record_byte(x, 0x12u, (uint8_t)c->ax);
    c->ax = record16(x, 6u);
    c->ax = arithmetic(c, c->ax, record16(x, 2u), 16u, 1);
    status = sign_call(x, 0x0216u);
    if (status != PR_U3C2_01A0_OK) return status;
    record_byte(x, 0x13u, (uint8_t)c->ax);
    c->ax = record16(x, 0u);                /* 021D */
    record_write(x, 0x14u, c->ax);
    c->ax = record16(x, 2u);
    record_write(x, 0x16u, c->ax);
    c->ax = record16(x, 0x0Au);
    (void)arithmetic(c, c->ax, record16(x, 0x0Cu), 16u, 1);
    if ((c->flags & 0x40u) == 0u &&
        ((c->flags >> 7) & 1u) == ((c->flags >> 11) & 1u)) {
        c->ax = record16(x, 0x0Cu);          /* 0248 */
        shift_ax(c);
        c->ax = arithmetic(c, c->ax, record16(x, 0x0Au), 16u, 1);
        record_write(x, 8u, c->ax);
        c->ax = record16(x, 0x0Cu);
        c->ax = arithmetic(c, c->ax, record16(x, 0x0Au), 16u, 1);
        shift_ax(c);
        record_write(x, 0x0Eu, c->ax);
        c->ax = record16(x, 0x0Cu);
        shift_ax(c);
        record_write(x, 0x10u, c->ax);
    } else {
        c->ax = record16(x, 0x0Au);          /* 0288 */
        shift_ax(c);
        c->ax = arithmetic(c, c->ax, record16(x, 0x0Cu), 16u, 1);
        record_write(x, 8u, c->ax);
        c->ax = record16(x, 0x0Au);
        c->ax = arithmetic(c, c->ax, record16(x, 0x0Cu), 16u, 1);
        shift_ax(c);
        record_write(x, 0x0Eu, c->ax);
        c->ax = record16(x, 0x0Au);
        shift_ax(c);
        record_write(x, 0x10u, c->ax);
    }
    leave(x, 4u);
    return PR_U3C2_01A0_OK;
}
