/* Full-state initializer; docs/re/U25_INIT_01B6.md. */
#include "poolrad/recovered/overlay_u25_init_01b6.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#define RAM_MASK (PR_U25_0000_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = physical(segment, offset);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t a = physical(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static uint8_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static uint16_t arithmetic(pr_u25_0000_cpu *cpu, uint16_t left,
                           uint16_t right, uint16_t sign, int subtract)
{
    uint32_t wide = subtract ? (uint32_t)left - right : (uint32_t)left + right;
    uint16_t mask = (uint16_t)(sign | (sign - 1u));
    uint16_t result = (uint16_t)(wide & mask);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (subtract ? left < right : wide > mask) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if ((result & sign) != 0u) cpu->flags |= 0x80u;
    if (((subtract ? (left ^ right) : (uint16_t)~(left ^ right)) &
         (left ^ result) & sign) != 0u) cpu->flags |= 0x800u;
    return result;
}

static void cmp8(pr_u25_0000_cpu *cpu, uint8_t left, uint8_t right)
{
    (void)arithmetic(cpu, left, right, 0x80u, 1);
}

static void cmp16(pr_u25_0000_cpu *cpu, uint16_t left, uint16_t right)
{
    (void)arithmetic(cpu, left, right, 0x8000u, 1);
}

static void set_al(pr_u25_0000_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint16_t pop16(const uint8_t *ram, pr_u25_0000_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static void push16(uint8_t *ram, pr_u25_0000_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}



static void logical(pr_u25_0000_cpu *cpu, uint16_t result, unsigned bits)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & (bits == 8u ? 0x80u : 0x8000u)) cpu->flags |= 0x80u;
}

static uint16_t increment(pr_u25_0000_cpu *cpu, uint16_t value, unsigned bits)
{
    uint16_t carry = cpu->flags & 1u;
    uint16_t result = arithmetic(cpu, value, 1u, bits == 8u ? 0x80u : 0x8000u, 0);
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~1u) | carry);
    return result;
}

static uint16_t shift(pr_u25_0000_cpu *cpu, uint16_t value, unsigned count, int right)
{
    uint16_t old = value, carry = 0, overflow = cpu->flags & 0x800u;
    unsigned i;
    count &= 31u;
    if (!count) return value;
    for (i = 0; i < count; ++i) {
        carry = (uint16_t)(right ? value & 1u : value >> 15);
        value = (uint16_t)(right ? value >> 1 : (uint32_t)value << 1);
    }
    logical(cpu, value, 16u);
    if (count == 1u)
        overflow = (right ? old & 0x8000u : (old ^ value) & 0x8000u) ? 0x800u : 0u;
    cpu->flags |= (uint16_t)(carry | overflow);
    return value;
}

/* LES/LDS read all four bytes before updating either destination register. */
static void load_far(const uint8_t *ram, pr_u25_0000_cpu *cpu,
                     uint16_t segment, uint16_t offset, int data_segment)
{
    size_t a = physical(segment, offset);
    uint16_t off = (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
    uint16_t seg = (uint16_t)(ram[(a + 2u) & RAM_MASK] |
                            ((uint16_t)ram[(a + 3u) & RAM_MASK] << 8));
    if (data_segment) { cpu->si = off; cpu->ds = seg; }
    else { cpu->di = off; cpu->es = seg; }
}



static pr_u25_0000_status body_06af(
    uint8_t *ram, pr_u25_0000_cpu *cpu, uint16_t system_cs,
    size_t *remaining, size_t string_budget)
{
    (void)system_cs; (void)remaining; (void)string_budget;
    push16(ram, cpu, cpu->bp); /* +06AF */
    cpu->bp = (uint16_t)(cpu->sp); /* +06B0 */
    cpu->sp = (uint16_t)(arithmetic(cpu, cpu->sp, 0x2u, 0x8000u, 1)); /* +06B2 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8)))); /* +06B5 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +06B8 */
    cpu->dx = (uint16_t)(cpu->ax); /* +06BA */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6)))); /* +06BC */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +06BF */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +06C1 */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, (uint8_t)cpu->cx, 0)); /* +06C3 */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 0)); /* +06C5 */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2), (uint16_t)(cpu->ax)); /* +06C7 */
    cpu->ax = (uint16_t)(read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +06CA */
    cpu->sp = (uint16_t)(cpu->bp); /* +06CD */
    cpu->bp = (uint16_t)(pop16(ram, cpu)); /* +06CF */
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U25_0000_OK; /* +06D0 */
}


pr_u25_01b6_status pr_u25_script_init_01b6(
    uint8_t *ram, size_t ram_size, pr_u25_01b6_cpu *cpu,
    uint16_t system_cs, size_t max_clear_iterations,
    size_t max_operands, size_t max_string_iterations)
{
    if (!ram || !cpu || ram_size != PR_U25_0000_RAM_SIZE ||
        cpu->ip != 0x01B6u || cpu->sp < 0x0236u || cpu->sp > 0xFFFCu ||
        !max_clear_iterations || !max_operands || !max_string_iterations)
        return PR_U25_01B6_ARGUMENT;

    push16(ram, cpu, cpu->bp); /* +01B6 */
    cpu->bp = (uint16_t)(cpu->sp); /* +01B7 */
    cpu->sp = (uint16_t)(arithmetic(cpu, cpu->sp, 4u, 0x8000u, 1)); /* +01B9 */
    write8(ram, cpu->ds, (uint16_t)(0x84e0), (uint8_t)(0u)); /* +01BC */
    write8(ram, cpu->ds, (uint16_t)(0x84e2), (uint8_t)(0u)); /* +01C1 */
    write8(ram, cpu->ds, (uint16_t)(0x84d2), (uint8_t)(0u)); /* +01C6 */
    write8(ram, cpu->ds, (uint16_t)(0x84d3), (uint8_t)(0u)); /* +01CB */
    write8(ram, cpu->ds, (uint16_t)(0x6f68), (uint8_t)(0x41u)); /* +01D0 */
    write8(ram, cpu->ds, (uint16_t)(0x6f69), (uint8_t)(9u)); /* +01D5 */
    write8(ram, cpu->ds, (uint16_t)(0x84e5), (uint8_t)(1u)); /* +01DA */
    cpu->di = (uint16_t)(0x84c8u); /* +01DF */
    push16(ram, cpu, cpu->ds); /* +01E2 */
    push16(ram, cpu, cpu->di); /* +01E3 */
    cpu->ax = (uint16_t)(2u); /* +01E4 */
    push16(ram, cpu, cpu->ax); /* +01E7 */
    set_al(cpu, (uint8_t)(0u)); /* +01E8 */
    push16(ram, cpu, cpu->ax); /* +01EA */
    {
        uint16_t caller_cs = cpu->cs;
        pr_system_leaf_cpu leaf;
        push16(ram, cpu, cpu->cs);
        push16(ram, cpu, 0x01F0u);
        cpu->cs = system_cs; cpu->ip = 0x16A8u;
        leaf.ax = cpu->ax;
        leaf.bx = cpu->bx;
        leaf.cx = cpu->cx;
        leaf.dx = cpu->dx;
        leaf.si = cpu->si;
        leaf.di = cpu->di;
        leaf.bp = cpu->bp;
        leaf.sp = cpu->sp;
        leaf.cs = cpu->cs;
        leaf.ip = cpu->ip;
        leaf.ds = cpu->ds;
        leaf.es = cpu->es;
        leaf.ss = cpu->ss;
        leaf.flags = cpu->flags;
        (void)pr_system_fillchar_16a8(ram, ram_size, &leaf);
        cpu->ax = leaf.ax;
        cpu->bx = leaf.bx;
        cpu->cx = leaf.cx;
        cpu->dx = leaf.dx;
        cpu->si = leaf.si;
        cpu->di = leaf.di;
        cpu->bp = leaf.bp;
        cpu->sp = leaf.sp;
        cpu->cs = leaf.cs;
        cpu->ip = leaf.ip;
        cpu->ds = leaf.ds;
        cpu->es = leaf.es;
        cpu->ss = leaf.ss;
        cpu->flags = leaf.flags;
        if (cpu->cs != caller_cs || cpu->ip != 0x01F0u) return PR_U25_01B6_TRANSFER;
    } /* +01EB */
    write8(ram, cpu->ds, (uint16_t)(0x6f83), (uint8_t)(8u)); /* +01F0 */
    write16(ram, cpu->ds, (uint16_t)(0x49ed), (uint16_t)(0x9900u)); /* +01F5 */
    write8(ram, cpu->ds, (uint16_t)(0x70c6), (uint8_t)(0u)); /* +01FB */
    cpu->ax = 0; logical(cpu, 0, 16u); /* +0200 */
    write16(ram, cpu->ds, (uint16_t)(0x6f70), (uint16_t)(cpu->ax)); /* +0202 */
    write16(ram, cpu->ds, (uint16_t)(0x6f72), (uint16_t)(cpu->ax)); /* +0205 */
    cpu->ax = 0; logical(cpu, 0, 16u); /* +0208 */
    write16(ram, cpu->ds, (uint16_t)(0x6f74), (uint16_t)(cpu->ax)); /* +020A */
    write16(ram, cpu->ds, (uint16_t)(0x6f76), (uint16_t)(cpu->ax)); /* +020D */
    cpu->di = (uint16_t)(0x6f78u); /* +0210 */
    push16(ram, cpu, cpu->ds); /* +0213 */
    push16(ram, cpu, cpu->di); /* +0214 */
    cpu->ax = (uint16_t)(6u); /* +0215 */
    push16(ram, cpu, cpu->ax); /* +0218 */
    set_al(cpu, (uint8_t)(0u)); /* +0219 */
    push16(ram, cpu, cpu->ax); /* +021B */
    {
        uint16_t caller_cs = cpu->cs;
        pr_system_leaf_cpu leaf;
        push16(ram, cpu, cpu->cs);
        push16(ram, cpu, 0x0221u);
        cpu->cs = system_cs; cpu->ip = 0x16A8u;
        leaf.ax = cpu->ax;
        leaf.bx = cpu->bx;
        leaf.cx = cpu->cx;
        leaf.dx = cpu->dx;
        leaf.si = cpu->si;
        leaf.di = cpu->di;
        leaf.bp = cpu->bp;
        leaf.sp = cpu->sp;
        leaf.cs = cpu->cs;
        leaf.ip = cpu->ip;
        leaf.ds = cpu->ds;
        leaf.es = cpu->es;
        leaf.ss = cpu->ss;
        leaf.flags = cpu->flags;
        (void)pr_system_fillchar_16a8(ram, ram_size, &leaf);
        cpu->ax = leaf.ax;
        cpu->bx = leaf.bx;
        cpu->cx = leaf.cx;
        cpu->dx = leaf.dx;
        cpu->si = leaf.si;
        cpu->di = leaf.di;
        cpu->bp = leaf.bp;
        cpu->sp = leaf.sp;
        cpu->cs = leaf.cs;
        cpu->ip = leaf.ip;
        cpu->ds = leaf.ds;
        cpu->es = leaf.es;
        cpu->ss = leaf.ss;
        cpu->flags = leaf.flags;
        if (cpu->cs != caller_cs || cpu->ip != 0x0221u) return PR_U25_01B6_TRANSFER;
    } /* +021C */
    load_far(ram, cpu, cpu->ds, (uint16_t)(0x49d6), 0); /* +0221 */
    write16(ram, cpu->es, (uint16_t)(cpu->di + 0x5c2), (uint16_t)(0xffu)); /* +0225 */
    load_far(ram, cpu, cpu->ds, (uint16_t)(0x49d6), 0); /* +022C */
    cpu->ax = 0; logical(cpu, 0, 16u); /* +0230 */
    write16(ram, cpu->es, (uint16_t)(cpu->di + 0x5a4), (uint16_t)(cpu->ax)); /* +0232 */
    load_far(ram, cpu, cpu->ds, (uint16_t)(0x49d6), 0); /* +0237 */
    cpu->ax = 0; logical(cpu, 0, 16u); /* +023B */
    write16(ram, cpu->es, (uint16_t)(cpu->di + 0x5a6), (uint16_t)(cpu->ax)); /* +023D */
    load_far(ram, cpu, cpu->ds, (uint16_t)(0x49d2), 0); /* +0242 */
    cpu->ax = 0; logical(cpu, 0, 16u); /* +0246 */
    write16(ram, cpu->es, (uint16_t)(cpu->di + 0x1ca), (uint16_t)(cpu->ax)); /* +0248 */
    set_al(cpu, (uint8_t)(1u)); /* +024D */
    push16(ram, cpu, cpu->ax); /* +024F */
    push16(ram, cpu, cpu->cs); /* +0250 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_0000_status status;
        push16(ram, cpu, 0x0254u);
        cpu->ip = 0x0000u;
        status = pr_u25_load_operands_0000(ram, ram_size, cpu, system_cs, max_operands, max_string_iterations);
        if (status == PR_U25_0000_OPERAND_LIMIT) return PR_U25_01B6_OPERAND_LIMIT;
        if (status == PR_U25_0000_STRING_LIMIT) return PR_U25_01B6_STRING_LIMIT;
        if (status == PR_U25_0000_TRANSFER) return PR_U25_01B6_TRANSFER;
        if (status != PR_U25_0000_OK) return PR_U25_01B6_CALLEE;
        if (cpu->cs != caller_cs || cpu->ip != 0x0254u) return PR_U25_01B6_TRANSFER;
    } /* +0251 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(0x7086)))); /* +0254 */
    push16(ram, cpu, cpu->ax); /* +0257 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(0x7046)))); /* +0258 */
    push16(ram, cpu, cpu->ax); /* +025B */
    push16(ram, cpu, cpu->cs); /* +025C */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_0000_status status;
        push16(ram, cpu, 0x0260u);
        cpu->ip = 0x06AFu;
        status = body_06af(ram, cpu, system_cs, &max_operands, max_string_iterations);
        if (status == PR_U25_0000_OPERAND_LIMIT) return PR_U25_01B6_OPERAND_LIMIT;
        if (status == PR_U25_0000_STRING_LIMIT) return PR_U25_01B6_STRING_LIMIT;
        if (status == PR_U25_0000_TRANSFER) return PR_U25_01B6_TRANSFER;
        if (status != PR_U25_0000_OK) return PR_U25_01B6_CALLEE;
        if (cpu->cs != caller_cs || cpu->ip != 0x0260u) return PR_U25_01B6_TRANSFER;
    } /* +025D */
    write16(ram, cpu->ds, (uint16_t)(0x49e3), (uint16_t)(cpu->ax)); /* +0260 */
    set_al(cpu, (uint8_t)(1u)); /* +0263 */
    push16(ram, cpu, cpu->ax); /* +0265 */
    push16(ram, cpu, cpu->cs); /* +0266 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_0000_status status;
        push16(ram, cpu, 0x026Au);
        cpu->ip = 0x0000u;
        status = pr_u25_load_operands_0000(ram, ram_size, cpu, system_cs, max_operands, max_string_iterations);
        if (status == PR_U25_0000_OPERAND_LIMIT) return PR_U25_01B6_OPERAND_LIMIT;
        if (status == PR_U25_0000_STRING_LIMIT) return PR_U25_01B6_STRING_LIMIT;
        if (status == PR_U25_0000_TRANSFER) return PR_U25_01B6_TRANSFER;
        if (status != PR_U25_0000_OK) return PR_U25_01B6_CALLEE;
        if (cpu->cs != caller_cs || cpu->ip != 0x026Au) return PR_U25_01B6_TRANSFER;
    } /* +0267 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(0x7086)))); /* +026A */
    push16(ram, cpu, cpu->ax); /* +026D */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(0x7046)))); /* +026E */
    push16(ram, cpu, cpu->ax); /* +0271 */
    push16(ram, cpu, cpu->cs); /* +0272 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_0000_status status;
        push16(ram, cpu, 0x0276u);
        cpu->ip = 0x06AFu;
        status = body_06af(ram, cpu, system_cs, &max_operands, max_string_iterations);
        if (status == PR_U25_0000_OPERAND_LIMIT) return PR_U25_01B6_OPERAND_LIMIT;
        if (status == PR_U25_0000_STRING_LIMIT) return PR_U25_01B6_STRING_LIMIT;
        if (status == PR_U25_0000_TRANSFER) return PR_U25_01B6_TRANSFER;
        if (status != PR_U25_0000_OK) return PR_U25_01B6_CALLEE;
        if (cpu->cs != caller_cs || cpu->ip != 0x0276u) return PR_U25_01B6_TRANSFER;
    } /* +0273 */
    write16(ram, cpu->ds, (uint16_t)(0x49e5), (uint16_t)(cpu->ax)); /* +0276 */
    set_al(cpu, (uint8_t)(1u)); /* +0279 */
    push16(ram, cpu, cpu->ax); /* +027B */
    push16(ram, cpu, cpu->cs); /* +027C */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_0000_status status;
        push16(ram, cpu, 0x0280u);
        cpu->ip = 0x0000u;
        status = pr_u25_load_operands_0000(ram, ram_size, cpu, system_cs, max_operands, max_string_iterations);
        if (status == PR_U25_0000_OPERAND_LIMIT) return PR_U25_01B6_OPERAND_LIMIT;
        if (status == PR_U25_0000_STRING_LIMIT) return PR_U25_01B6_STRING_LIMIT;
        if (status == PR_U25_0000_TRANSFER) return PR_U25_01B6_TRANSFER;
        if (status != PR_U25_0000_OK) return PR_U25_01B6_CALLEE;
        if (cpu->cs != caller_cs || cpu->ip != 0x0280u) return PR_U25_01B6_TRANSFER;
    } /* +027D */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(0x7086)))); /* +0280 */
    push16(ram, cpu, cpu->ax); /* +0283 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(0x7046)))); /* +0284 */
    push16(ram, cpu, cpu->ax); /* +0287 */
    push16(ram, cpu, cpu->cs); /* +0288 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_0000_status status;
        push16(ram, cpu, 0x028Cu);
        cpu->ip = 0x06AFu;
        status = body_06af(ram, cpu, system_cs, &max_operands, max_string_iterations);
        if (status == PR_U25_0000_OPERAND_LIMIT) return PR_U25_01B6_OPERAND_LIMIT;
        if (status == PR_U25_0000_STRING_LIMIT) return PR_U25_01B6_STRING_LIMIT;
        if (status == PR_U25_0000_TRANSFER) return PR_U25_01B6_TRANSFER;
        if (status != PR_U25_0000_OK) return PR_U25_01B6_CALLEE;
        if (cpu->cs != caller_cs || cpu->ip != 0x028Cu) return PR_U25_01B6_TRANSFER;
    } /* +0289 */
    write16(ram, cpu->ds, (uint16_t)(0x49e7), (uint16_t)(cpu->ax)); /* +028C */
    set_al(cpu, (uint8_t)(1u)); /* +028F */
    push16(ram, cpu, cpu->ax); /* +0291 */
    push16(ram, cpu, cpu->cs); /* +0292 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_0000_status status;
        push16(ram, cpu, 0x0296u);
        cpu->ip = 0x0000u;
        status = pr_u25_load_operands_0000(ram, ram_size, cpu, system_cs, max_operands, max_string_iterations);
        if (status == PR_U25_0000_OPERAND_LIMIT) return PR_U25_01B6_OPERAND_LIMIT;
        if (status == PR_U25_0000_STRING_LIMIT) return PR_U25_01B6_STRING_LIMIT;
        if (status == PR_U25_0000_TRANSFER) return PR_U25_01B6_TRANSFER;
        if (status != PR_U25_0000_OK) return PR_U25_01B6_CALLEE;
        if (cpu->cs != caller_cs || cpu->ip != 0x0296u) return PR_U25_01B6_TRANSFER;
    } /* +0293 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(0x7086)))); /* +0296 */
    push16(ram, cpu, cpu->ax); /* +0299 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(0x7046)))); /* +029A */
    push16(ram, cpu, cpu->ax); /* +029D */
    push16(ram, cpu, cpu->cs); /* +029E */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_0000_status status;
        push16(ram, cpu, 0x02A2u);
        cpu->ip = 0x06AFu;
        status = body_06af(ram, cpu, system_cs, &max_operands, max_string_iterations);
        if (status == PR_U25_0000_OPERAND_LIMIT) return PR_U25_01B6_OPERAND_LIMIT;
        if (status == PR_U25_0000_STRING_LIMIT) return PR_U25_01B6_STRING_LIMIT;
        if (status == PR_U25_0000_TRANSFER) return PR_U25_01B6_TRANSFER;
        if (status != PR_U25_0000_OK) return PR_U25_01B6_CALLEE;
        if (cpu->cs != caller_cs || cpu->ip != 0x02A2u) return PR_U25_01B6_TRANSFER;
    } /* +029F */
    write16(ram, cpu->ds, (uint16_t)(0x49e9), (uint16_t)(cpu->ax)); /* +02A2 */
    set_al(cpu, (uint8_t)(1u)); /* +02A5 */
    push16(ram, cpu, cpu->ax); /* +02A7 */
    push16(ram, cpu, cpu->cs); /* +02A8 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_0000_status status;
        push16(ram, cpu, 0x02ACu);
        cpu->ip = 0x0000u;
        status = pr_u25_load_operands_0000(ram, ram_size, cpu, system_cs, max_operands, max_string_iterations);
        if (status == PR_U25_0000_OPERAND_LIMIT) return PR_U25_01B6_OPERAND_LIMIT;
        if (status == PR_U25_0000_STRING_LIMIT) return PR_U25_01B6_STRING_LIMIT;
        if (status == PR_U25_0000_TRANSFER) return PR_U25_01B6_TRANSFER;
        if (status != PR_U25_0000_OK) return PR_U25_01B6_CALLEE;
        if (cpu->cs != caller_cs || cpu->ip != 0x02ACu) return PR_U25_01B6_TRANSFER;
    } /* +02A9 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(0x7086)))); /* +02AC */
    push16(ram, cpu, cpu->ax); /* +02AF */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(0x7046)))); /* +02B0 */
    push16(ram, cpu, cpu->ax); /* +02B3 */
    push16(ram, cpu, cpu->cs); /* +02B4 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_0000_status status;
        push16(ram, cpu, 0x02B8u);
        cpu->ip = 0x06AFu;
        status = body_06af(ram, cpu, system_cs, &max_operands, max_string_iterations);
        if (status == PR_U25_0000_OPERAND_LIMIT) return PR_U25_01B6_OPERAND_LIMIT;
        if (status == PR_U25_0000_STRING_LIMIT) return PR_U25_01B6_STRING_LIMIT;
        if (status == PR_U25_0000_TRANSFER) return PR_U25_01B6_TRANSFER;
        if (status != PR_U25_0000_OK) return PR_U25_01B6_CALLEE;
        if (cpu->cs != caller_cs || cpu->ip != 0x02B8u) return PR_U25_01B6_TRANSFER;
    } /* +02B5 */
    write16(ram, cpu->ds, (uint16_t)(0x49eb), (uint16_t)(cpu->ax)); /* +02B8 */
    load_far(ram, cpu, cpu->ds, (uint16_t)(0x49d2), 0); /* +02BB */
    write16(ram, cpu->es, (uint16_t)(cpu->di + 0x1cc), (uint16_t)(1u)); /* +02BF */
    cmp8(cpu, read8(ram, cpu->ds, (uint16_t)(0x49f8)), 0u); /* +02C6 */
    if (!(cpu->flags & 0x40u)) goto L_0319; /* +02CB */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 3), (uint16_t)(1u)); /* +02CD */
    goto L_02d7; /* +02D2 */
L_02d4:
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 3), (uint16_t)(increment(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 3)), 16u))); /* +02D4 */
L_02d7:
    if (!max_clear_iterations) { cpu->ip = 0x02D7u; return PR_U25_01B6_LOOP_LIMIT; }
    --max_clear_iterations;
    cpu->ax = (uint16_t)(read16(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +02D7 */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, 0x49ffu, 0x8000u, 0)); /* +02DA */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, 1u, 0)); /* +02DD */
    load_far(ram, cpu, cpu->ds, (uint16_t)(0x49d2), 0); /* +02DF */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +02E3 */
    cpu->ax = 0; logical(cpu, 0, 16u); /* +02E5 */
    write16(ram, cpu->es, (uint16_t)(cpu->di + 0x6e00), (uint16_t)(cpu->ax)); /* +02E7 */
    cmp16(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 3)), 0x20u); /* +02EC */
    if (!(cpu->flags & 0x40u)) goto L_02d4; /* +02F0 */
    cpu->ax = 0; logical(cpu, 0, 16u); /* +02F2 */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 3), (uint16_t)(cpu->ax)); /* +02F4 */
    goto L_02fc; /* +02F7 */
L_02f9:
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 3), (uint16_t)(increment(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 3)), 16u))); /* +02F9 */
L_02fc:
    if (!max_clear_iterations) { cpu->ip = 0x02FCu; return PR_U25_01B6_LOOP_LIMIT; }
    --max_clear_iterations;
    cpu->ax = (uint16_t)(read16(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +02FC */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, 0x6e79u, 0x8000u, 0)); /* +02FF */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, 1u, 0)); /* +0302 */
    load_far(ram, cpu, cpu->ds, (uint16_t)(0x49d6), 0); /* +0304 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +0308 */
    cpu->ax = 0; logical(cpu, 0, 16u); /* +030A */
    write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2a00), (uint16_t)(cpu->ax)); /* +030C */
    cmp16(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 3)), 9u); /* +0311 */
    if (!(cpu->flags & 0x40u)) goto L_02f9; /* +0315 */
    goto L_031e; /* +0317 */
L_0319:
    write8(ram, cpu->ds, (uint16_t)(0x49f8), (uint8_t)(0u)); /* +0319 */
L_031e:
    cpu->sp = (uint16_t)(cpu->bp); /* +031E */
    cpu->bp = (uint16_t)(pop16(ram, cpu)); /* +0320 */
    cpu->ip = pop16(ram, cpu); cpu->cs = pop16(ram, cpu);
    return PR_U25_01B6_OK; /* +0321 */
}
