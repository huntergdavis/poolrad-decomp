/* U25 entry 2 and +06AF; see docs/re/U25_LOADER_0000.md. */
#include "poolrad/recovered/overlay_u25_loader_0000.h"

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

static pr_u25_0000_status body_0000(
    uint8_t *ram, pr_u25_0000_cpu *cpu, uint16_t system_cs,
    size_t *remaining, size_t string_budget)
{
    (void)system_cs; (void)remaining; (void)string_budget;
    push16(ram, cpu, cpu->bp); /* +0000 */
    cpu->bp = (uint16_t)(cpu->sp); /* +0001 */
    cpu->sp = (uint16_t)(arithmetic(cpu, cpu->sp, 0x8u, 0x8000u, 1)); /* +0003 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2), (uint8_t)(0x0u)); /* +0006 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6)))); /* +000A */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 7), (uint8_t)((uint8_t)cpu->ax)); /* +000D */
    set_al(cpu, (uint8_t)(0x1u)); /* +0010 */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 7))); /* +0012 */
    if ((cpu->flags & 0x41u) != 0u) goto L_001a; /* +0015 */
    goto L_012c; /* +0017 */

L_001a:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)((uint8_t)cpu->ax)); /* +001A */
    goto L_0022; /* +001D */

L_001f:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)(increment(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)), 8u))); /* +001F */

L_0022:
    cpu->ip = 0x0022u;
    if (*remaining == 0u) return PR_U25_0000_OPERAND_LIMIT;
    --*remaining;
    cpu->ax = (uint16_t)(read16(ram, cpu->ds, (uint16_t)(18925))); /* +0022 */
    cpu->ax = (uint16_t)(increment(cpu, cpu->ax, 16u)); /* +0025 */
    load_far(ram, cpu, cpu->ds, (uint16_t)(18910), 0); /* +0026 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +002A */
    cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di + 26368)))); /* +002C */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)))); /* +0031 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +0034 */
    cpu->di = (uint16_t)(cpu->ax); /* +0036 */
    write8(ram, cpu->ds, (uint16_t)(cpu->di + 28677), (uint8_t)((uint8_t)cpu->dx)); /* +0038 */
    cpu->ax = (uint16_t)(read16(ram, cpu->ds, (uint16_t)(18925))); /* +003C */
    cpu->ax = (uint16_t)(increment(cpu, cpu->ax, 16u)); /* +003F */
    cpu->ax = (uint16_t)(increment(cpu, cpu->ax, 16u)); /* +0040 */
    load_far(ram, cpu, cpu->ds, (uint16_t)(18910), 0); /* +0041 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +0045 */
    cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di + 26368)))); /* +0047 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)))); /* +004C */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +004F */
    cpu->di = (uint16_t)(cpu->ax); /* +0051 */
    write8(ram, cpu->ds, (uint16_t)(cpu->di + 28805), (uint8_t)((uint8_t)cpu->dx)); /* +0053 */
    cpu->ax = (uint16_t)(read16(ram, cpu->ds, (uint16_t)(18925))); /* +0057 */
    cpu->ax = (uint16_t)(increment(cpu, cpu->ax, 16u)); /* +005A */
    cpu->ax = (uint16_t)(increment(cpu, cpu->ax, 16u)); /* +005B */
    write16(ram, cpu->ds, (uint16_t)(18925), (uint16_t)(cpu->ax)); /* +005C */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)))); /* +005F */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +0062 */
    cpu->di = (uint16_t)(cpu->ax); /* +0064 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(cpu->di + 28677)))); /* +0066 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x1u); /* +006A */
    if ((cpu->flags & 0x40u) != 0u) goto L_0076; /* +006C */
    cmp8(cpu, (uint8_t)cpu->ax, 0x2u); /* +006E */
    if ((cpu->flags & 0x40u) != 0u) goto L_0076; /* +0070 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x3u); /* +0072 */
    if ((cpu->flags & 0x40u) == 0u) goto L_0096; /* +0074 */

L_0076:
    write16(ram, cpu->ds, (uint16_t)(18925), (uint16_t)(increment(cpu, read16(ram, cpu->ds, (uint16_t)(18925)), 16u))); /* +0076 */
    cpu->ax = (uint16_t)(read16(ram, cpu->ds, (uint16_t)(18925))); /* +007A */
    load_far(ram, cpu, cpu->ds, (uint16_t)(18910), 0); /* +007D */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +0081 */
    cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di + 26368)))); /* +0083 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)))); /* +0088 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +008B */
    cpu->di = (uint16_t)(cpu->ax); /* +008D */
    write8(ram, cpu->ds, (uint16_t)(cpu->di + 28741), (uint8_t)((uint8_t)cpu->dx)); /* +008F */
    goto L_0121; /* +0093 */

L_0096:
    cmp8(cpu, (uint8_t)cpu->ax, 0x80u); /* +0096 */
    if ((cpu->flags & 0x40u) == 0u) goto L_00d3; /* +0098 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2), (uint8_t)(increment(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)), 8u))); /* +009A */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)))); /* +009D */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +00A0 */
    cpu->di = (uint16_t)(cpu->ax); /* +00A2 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(cpu->di + 28805)))); /* +00A4 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +00A8 */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 6), (uint16_t)(cpu->ax)); /* +00AA */
    cmp16(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 6)), 0x0u); /* +00AD */
    if ((cpu->flags & 0x41u) != 0u) goto L_00c1; /* +00B1 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 6)))); /* +00B3 */
    push16(ram, cpu, cpu->ax); /* +00B6 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)))); /* +00B7 */
    push16(ram, cpu, cpu->ax); /* +00BA */
    push16(ram, cpu, cpu->cs); /* +00BB */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, 0x00BFu);
        cpu->ip = 0x1153u;
        status = pr_u25_unpack_text_1153(ram, PR_U25_0000_RAM_SIZE, cpu, system_cs, string_budget);
        if (status == PR_U25_STRINGS_LIMIT) return PR_U25_0000_STRING_LIMIT;
        if (status == PR_U25_STRINGS_TRANSFER) return PR_U25_0000_TRANSFER;
        if (status != PR_U25_STRINGS_OK) return PR_U25_0000_CALLEE;
        if (cpu->cs != caller_cs || cpu->ip != 0x00BFu)
            return PR_U25_0000_TRANSFER;
    } /* +00BC */
    goto L_00d1; /* +00BF */

L_00c1:
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)))); /* +00C1 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +00C4 */
    cpu->di = (uint16_t)(cpu->ax); /* +00C6 */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +00C8 */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +00CA */
    write8(ram, cpu->ds, (uint16_t)(cpu->di + 28616), (uint8_t)(0x0u)); /* +00CC */

L_00d1:
    goto L_0121; /* +00D1 */

L_00d3:
    cmp8(cpu, (uint8_t)cpu->ax, 0x81u); /* +00D3 */
    if ((cpu->flags & 0x40u) == 0u) goto L_0121; /* +00D5 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2), (uint8_t)(increment(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)), 8u))); /* +00D7 */
    write16(ram, cpu->ds, (uint16_t)(18925), (uint16_t)(increment(cpu, read16(ram, cpu->ds, (uint16_t)(18925)), 16u))); /* +00DA */
    cpu->ax = (uint16_t)(read16(ram, cpu->ds, (uint16_t)(18925))); /* +00DE */
    load_far(ram, cpu, cpu->ds, (uint16_t)(18910), 0); /* +00E1 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +00E5 */
    cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di + 26368)))); /* +00E7 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)))); /* +00EC */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +00EF */
    cpu->di = (uint16_t)(cpu->ax); /* +00F1 */
    write8(ram, cpu->ds, (uint16_t)(cpu->di + 28741), (uint8_t)((uint8_t)cpu->dx)); /* +00F3 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)))); /* +00F7 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +00FA */
    cpu->di = (uint16_t)(cpu->ax); /* +00FC */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(cpu->di + 28805)))); /* +00FE */
    push16(ram, cpu, cpu->ax); /* +0102 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)))); /* +0103 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +0106 */
    cpu->di = (uint16_t)(cpu->ax); /* +0108 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(cpu->di + 28741)))); /* +010A */
    push16(ram, cpu, cpu->ax); /* +010E */
    push16(ram, cpu, cpu->cs); /* +010F */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_0000_status status;
        push16(ram, cpu, 0x0113u);
        cpu->ip = 0x06AFu;
        status = body_06af(ram, cpu, system_cs, remaining, string_budget);
        if (status != PR_U25_0000_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x0113u)
            return PR_U25_0000_TRANSFER;
    } /* +0110 */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4), (uint16_t)(cpu->ax)); /* +0113 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)))); /* +0116 */
    push16(ram, cpu, cpu->ax); /* +0119 */
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4))); /* +011A */
    push16(ram, cpu, cpu->cs); /* +011D */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, 0x0121u);
        cpu->ip = 0x12FBu;
        status = pr_u25_string_by_address_12fb(ram, PR_U25_0000_RAM_SIZE, cpu, system_cs, string_budget);
        if (status == PR_U25_STRINGS_LIMIT) return PR_U25_0000_STRING_LIMIT;
        if (status == PR_U25_STRINGS_TRANSFER) return PR_U25_0000_TRANSFER;
        if (status != PR_U25_STRINGS_OK) return PR_U25_0000_CALLEE;
        if (cpu->cs != caller_cs || cpu->ip != 0x0121u)
            return PR_U25_0000_TRANSFER;
    } /* +011E */

L_0121:
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)))); /* +0121 */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 7))); /* +0124 */
    if ((cpu->flags & 0x40u) != 0u) goto L_012c; /* +0127 */
    goto L_001f; /* +0129 */

L_012c:
    write16(ram, cpu->ds, (uint16_t)(18925), (uint16_t)(increment(cpu, read16(ram, cpu->ds, (uint16_t)(18925)), 16u))); /* +012C */
    cpu->sp = (uint16_t)(cpu->bp); /* +0130 */
    cpu->bp = (uint16_t)(pop16(ram, cpu)); /* +0132 */
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return PR_U25_0000_OK; /* +0133 */
}

pr_u25_0000_status pr_u25_load_operands_0000(
    uint8_t *ram, size_t ram_size, pr_u25_0000_cpu *cpu,
    uint16_t system_cs, size_t max_operands, size_t max_string_iterations)
{
    if (ram == NULL || cpu == NULL || ram_size != PR_U25_0000_RAM_SIZE ||
        cpu->ip != 0u || max_operands == 0u || max_string_iterations == 0u ||
        cpu->sp < 0x022Au || cpu->sp > 0xFFFAu)
        return PR_U25_0000_ARGUMENT;
    return body_0000(ram, cpu, system_cs, &max_operands, max_string_iterations);
}

pr_u25_0000_status pr_u25_word_06af(uint8_t *ram, size_t ram_size, pr_u25_0000_cpu *cpu)
{
    if (ram == NULL || cpu == NULL || ram_size != PR_U25_0000_RAM_SIZE || cpu->ip != 0x06AFu)
        return PR_U25_0000_ARGUMENT;
    return body_06af(ram, cpu, 0u, NULL, 0u);
}
