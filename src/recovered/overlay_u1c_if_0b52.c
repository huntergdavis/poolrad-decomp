#include "overlay_loaded_contracts.h"
/* Complete U1C IF-family; see docs/re/U1C_IF_0B52.md. */
#include "poolrad/recovered/overlay_u1c_if_0b52.h"

#define RAM_MASK (PR_U1C_0B52_RAM_SIZE - 1u)
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

static uint16_t arithmetic(pr_u1c_0b52_cpu *cpu, uint16_t left,
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

static void cmp8(pr_u1c_0b52_cpu *cpu, uint8_t left, uint8_t right)
{
    (void)arithmetic(cpu, left, right, 0x80u, 1);
}

static void set_al(pr_u1c_0b52_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint16_t pop16(const uint8_t *ram, pr_u1c_0b52_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static void push16(uint8_t *ram, pr_u1c_0b52_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t increment(pr_u1c_0b52_cpu *cpu, uint16_t value, unsigned bits)
{
    uint16_t carry = cpu->flags & 1u;
    uint16_t result = arithmetic(cpu, value, 1u, bits == 8u ? 0x80u : 0x8000u, 0);
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~1u) | carry);
    return result;
}


static pr_u1c_0b52_status call_skip(
    uint8_t *ram, pr_u1c_0b52_cpu *cpu, uint16_t return_ip,
    uint16_t u25_cs, uint16_t system_cs,
    size_t operand_budget, size_t string_budget)
{
    uint16_t caller_cs = cpu->cs;
    pr_u25_1ea6_status status;
    push16(ram, cpu, caller_cs);
    push16(ram, cpu, return_ip);
    cpu->cs = u25_cs; /* loaded stub's far JMP has no additional effects */
    cpu->ip = 0x1EA6u;
    status = pr_u25_skip_statement_1ea6(ram, PR_U1C_0B52_RAM_SIZE, cpu,
                                       system_cs, operand_budget, string_budget);
    switch (status) {
    case PR_U25_1EA6_OPERAND_LIMIT: return PR_U1C_0B52_OPERAND_LIMIT;
    case PR_U25_1EA6_STRING_LIMIT: return PR_U1C_0B52_STRING_LIMIT;
    case PR_U25_1EA6_TRANSFER: return PR_U1C_0B52_TRANSFER;
    case PR_U25_1EA6_OK: break;
    default: return PR_U1C_0B52_CALLEE;
    }
    if (cpu->cs != caller_cs || cpu->ip != return_ip)
        return PR_U1C_0B52_TRANSFER;
    return PR_U1C_0B52_OK;
}

pr_u1c_0b52_status pr_u1c_if_0b52(
    uint8_t *ram, size_t ram_size, pr_u1c_0b52_cpu *cpu,
    uint16_t u25_cs, uint16_t system_cs,
    size_t max_operands, size_t max_string_iterations)
{
    pr_u1c_0b52_status status;
    if (ram == NULL || cpu == NULL || ram_size != PR_U1C_0B52_RAM_SIZE ||
        cpu->ip != 0x0B52u || max_operands == 0u || max_string_iterations == 0u ||
        cpu->sp < 0x0238u || cpu->sp > 0xFFFCu)
        return PR_U1C_0B52_ARGUMENT;
    {
        static const uint16_t sites[] = {0x0B67u, 0x0B79u, 0x0B8Bu, 0x0B9Du, 0x0BAFu, 0x0BC1u};
        size_t i;
        /* Every listed call must resolve through its live loaded 0xEA stub. */
        for (i = 0; i < sizeof sites / sizeof sites[0]; ++i)
            if (!pr_overlay_stub_matches(ram, cpu->cs, sites[i], 0x008Eu, 0x1EA6u, u25_cs))
                return PR_U1C_0B52_ARGUMENT;
    }
    push16(ram, cpu, cpu->bp); /* +0B52 */
    cpu->bp = (uint16_t)(cpu->sp); /* +0B53 */
    write16(ram, cpu->ds, (uint16_t)(0x49EDu), (uint16_t)(increment(cpu, read16(ram, cpu->ds, (uint16_t)(0x49EDu)), 16u))); /* +0B55 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(0x6F7Fu)))); /* +0B59 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x16u); /* +0B5C */
    if ((cpu->flags & 0x40u) == 0u) goto L_0b6e; /* +0B5E */
    cmp8(cpu, read8(ram, cpu->ds, (uint16_t)(0x6F78u)), 0x0u); /* +0B60 */
    if ((cpu->flags & 0x40u) == 0u) goto L_0b6c; /* +0B65 */
    status = call_skip(ram, cpu, 0x0B6Cu, u25_cs, system_cs,
                       max_operands, max_string_iterations);
    if (status != PR_U1C_0B52_OK) return status; /* +0B67 */

L_0b6c:
    goto L_0bc6; /* +0B6C */

L_0b6e:
    cmp8(cpu, (uint8_t)cpu->ax, 0x17u); /* +0B6E */
    if ((cpu->flags & 0x40u) == 0u) goto L_0b80; /* +0B70 */
    cmp8(cpu, read8(ram, cpu->ds, (uint16_t)(0x6F79u)), 0x0u); /* +0B72 */
    if ((cpu->flags & 0x40u) == 0u) goto L_0b7e; /* +0B77 */
    status = call_skip(ram, cpu, 0x0B7Eu, u25_cs, system_cs,
                       max_operands, max_string_iterations);
    if (status != PR_U1C_0B52_OK) return status; /* +0B79 */

L_0b7e:
    goto L_0bc6; /* +0B7E */

L_0b80:
    cmp8(cpu, (uint8_t)cpu->ax, 0x18u); /* +0B80 */
    if ((cpu->flags & 0x40u) == 0u) goto L_0b92; /* +0B82 */
    cmp8(cpu, read8(ram, cpu->ds, (uint16_t)(0x6F7Au)), 0x0u); /* +0B84 */
    if ((cpu->flags & 0x40u) == 0u) goto L_0b90; /* +0B89 */
    status = call_skip(ram, cpu, 0x0B90u, u25_cs, system_cs,
                       max_operands, max_string_iterations);
    if (status != PR_U1C_0B52_OK) return status; /* +0B8B */

L_0b90:
    goto L_0bc6; /* +0B90 */

L_0b92:
    cmp8(cpu, (uint8_t)cpu->ax, 0x19u); /* +0B92 */
    if ((cpu->flags & 0x40u) == 0u) goto L_0ba4; /* +0B94 */
    cmp8(cpu, read8(ram, cpu->ds, (uint16_t)(0x6F7Bu)), 0x0u); /* +0B96 */
    if ((cpu->flags & 0x40u) == 0u) goto L_0ba2; /* +0B9B */
    status = call_skip(ram, cpu, 0x0BA2u, u25_cs, system_cs,
                       max_operands, max_string_iterations);
    if (status != PR_U1C_0B52_OK) return status; /* +0B9D */

L_0ba2:
    goto L_0bc6; /* +0BA2 */

L_0ba4:
    cmp8(cpu, (uint8_t)cpu->ax, 0x1au); /* +0BA4 */
    if ((cpu->flags & 0x40u) == 0u) goto L_0bb6; /* +0BA6 */
    cmp8(cpu, read8(ram, cpu->ds, (uint16_t)(0x6F7Cu)), 0x0u); /* +0BA8 */
    if ((cpu->flags & 0x40u) == 0u) goto L_0bb4; /* +0BAD */
    status = call_skip(ram, cpu, 0x0BB4u, u25_cs, system_cs,
                       max_operands, max_string_iterations);
    if (status != PR_U1C_0B52_OK) return status; /* +0BAF */

L_0bb4:
    goto L_0bc6; /* +0BB4 */

L_0bb6:
    cmp8(cpu, (uint8_t)cpu->ax, 0x1bu); /* +0BB6 */
    if ((cpu->flags & 0x40u) == 0u) goto L_0bc6; /* +0BB8 */
    cmp8(cpu, read8(ram, cpu->ds, (uint16_t)(0x6F7Du)), 0x0u); /* +0BBA */
    if ((cpu->flags & 0x40u) == 0u) goto L_0bc6; /* +0BBF */
    status = call_skip(ram, cpu, 0x0BC6u, u25_cs, system_cs,
                       max_operands, max_string_iterations);
    if (status != PR_U1C_0B52_OK) return status; /* +0BC1 */

L_0bc6:
    cpu->bp = (uint16_t)(pop16(ram, cpu)); /* +0BC6 */
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    return PR_U1C_0B52_OK; /* +0BC7 */
}
