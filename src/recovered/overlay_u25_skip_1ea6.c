/* U25 entry 22; see docs/re/U25_SKIP_1EA6.md. */
#include "poolrad/recovered/overlay_u25_skip_1ea6.h"

#define RAM_MASK (PR_U25_1EA6_RAM_SIZE - 1u)
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

static uint16_t arithmetic(pr_u25_1ea6_cpu *cpu, uint16_t left,
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

static void cmp8(pr_u25_1ea6_cpu *cpu, uint8_t left, uint8_t right)
{
    (void)arithmetic(cpu, left, right, 0x80u, 1);
}

static void set_al(pr_u25_1ea6_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint16_t pop16(const uint8_t *ram, pr_u25_1ea6_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static void push16(uint8_t *ram, pr_u25_1ea6_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t increment(pr_u25_1ea6_cpu *cpu, uint16_t value, unsigned bits)
{
    uint16_t carry = cpu->flags & 1u;
    uint16_t result = arithmetic(cpu, value, 1u, bits == 8u ? 0x80u : 0x8000u, 0);
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~1u) | carry);
    return result;
}

/* LES/LDS read all four bytes before updating either destination register. */
static void load_far(const uint8_t *ram, pr_u25_1ea6_cpu *cpu,
                     uint16_t segment, uint16_t offset, int data_segment)
{
    size_t a = physical(segment, offset);
    uint16_t off = (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
    uint16_t seg = (uint16_t)(ram[(a + 2u) & RAM_MASK] |
                            ((uint16_t)ram[(a + 3u) & RAM_MASK] << 8));
    if (data_segment) { cpu->si = off; cpu->ds = seg; }
    else { cpu->di = off; cpu->es = seg; }
}

static pr_u25_1ea6_status call_loader(
    uint8_t *ram, pr_u25_1ea6_cpu *cpu, uint16_t return_ip,
    uint16_t system_cs, size_t operand_budget, size_t string_budget)
{
    uint16_t caller_cs = cpu->cs;
    pr_u25_0000_status status;
    push16(ram, cpu, return_ip); /* preceding PUSH CS is in the caller */
    cpu->ip = 0u;
    status = pr_u25_load_operands_0000(ram, PR_U25_1EA6_RAM_SIZE, cpu,
                                      system_cs, operand_budget, string_budget);
    switch (status) {
    case PR_U25_0000_OPERAND_LIMIT: return PR_U25_1EA6_OPERAND_LIMIT;
    case PR_U25_0000_STRING_LIMIT: return PR_U25_1EA6_STRING_LIMIT;
    case PR_U25_0000_TRANSFER: return PR_U25_1EA6_TRANSFER;
    case PR_U25_0000_OK: break;
    default: return PR_U25_1EA6_CALLEE;
    }
    if (cpu->cs != caller_cs || cpu->ip != return_ip)
        return PR_U25_1EA6_TRANSFER;
    return PR_U25_1EA6_OK;
}

pr_u25_1ea6_status pr_u25_skip_statement_1ea6(
    uint8_t *ram, size_t ram_size, pr_u25_1ea6_cpu *cpu,
    uint16_t system_cs, size_t max_operands, size_t max_string_iterations)
{
    pr_u25_1ea6_status status;
    if (ram == NULL || cpu == NULL || ram_size != PR_U25_1EA6_RAM_SIZE ||
        cpu->ip != 0x1EA6u || max_operands == 0u || max_string_iterations == 0u ||
        cpu->sp < 0x0232u || cpu->sp > 0xFFFCu)
        return PR_U25_1EA6_ARGUMENT;
    push16(ram, cpu, cpu->bp); /* +1EA6 */
    cpu->bp = (uint16_t)(cpu->sp); /* +1EA7 */
    cpu->ax = (uint16_t)(read16(ram, cpu->ds, (uint16_t)(0x49EDu))); /* +1EA9 */
    load_far(ram, cpu, cpu->ds, (uint16_t)(0x49DEu), 0); /* +1EAC */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +1EB0 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di + 0x6700u)))); /* +1EB2 */
    write8(ram, cpu->ds, (uint16_t)(0x6F7Fu), (uint8_t)((uint8_t)cpu->ax)); /* +1EB7 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(0x6F7Fu)))); /* +1EBA */
    cmp8(cpu, (uint8_t)cpu->ax, 0x1u); /* +1EBD */
    if ((cpu->flags & 0x40u) != 0u) goto L_1ef9; /* +1EBF */
    cmp8(cpu, (uint8_t)cpu->ax, 0x2u); /* +1EC1 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1ef9; /* +1EC3 */
    cmp8(cpu, (uint8_t)cpu->ax, 0xau); /* +1EC5 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1ef9; /* +1EC7 */
    cmp8(cpu, (uint8_t)cpu->ax, 0xeu); /* +1EC9 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1ef9; /* +1ECB */
    cmp8(cpu, (uint8_t)cpu->ax, 0x11u); /* +1ECD */
    if ((cpu->flags & 0x40u) != 0u) goto L_1ef9; /* +1ECF */
    cmp8(cpu, (uint8_t)cpu->ax, 0x12u); /* +1ED1 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1ef9; /* +1ED3 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x1du); /* +1ED5 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1ef9; /* +1ED7 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x20u); /* +1ED9 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1ef9; /* +1EDB */
    cmp8(cpu, (uint8_t)cpu->ax, 0x2du); /* +1EDD */
    if ((cpu->flags & 0x40u) != 0u) goto L_1ef9; /* +1EDF */
    cmp8(cpu, (uint8_t)cpu->ax, 0x32u); /* +1EE1 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1ef9; /* +1EE3 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x34u); /* +1EE5 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1ef9; /* +1EE7 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x36u); /* +1EE9 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1ef9; /* +1EEB */
    cmp8(cpu, (uint8_t)cpu->ax, 0x38u); /* +1EED */
    if ((cpu->flags & 0x40u) != 0u) goto L_1ef9; /* +1EEF */
    cmp8(cpu, (uint8_t)cpu->ax, 0x39u); /* +1EF1 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1ef9; /* +1EF3 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x3cu); /* +1EF5 */
    if ((cpu->flags & 0x40u) == 0u) goto L_1f03; /* +1EF7 */

L_1ef9:
    set_al(cpu, (uint8_t)(0x1u)); /* +1EF9 */
    push16(ram, cpu, cpu->ax); /* +1EFB */
    push16(ram, cpu, cpu->cs); /* +1EFC */
    status = call_loader(ram, cpu, 0x1F00u, system_cs,
                         max_operands, max_string_iterations);
    if (status != PR_U25_1EA6_OK) return status; /* +1EFD */
    goto L_1faf; /* +1F00 */

L_1f03:
    cmp8(cpu, (uint8_t)cpu->ax, 0x3u); /* +1F03 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f23; /* +1F05 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x8u); /* +1F07 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f23; /* +1F09 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x9u); /* +1F0B */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f23; /* +1F0D */
    cmp8(cpu, (uint8_t)cpu->ax, 0xcu); /* +1F0F */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f23; /* +1F11 */
    cmp8(cpu, (uint8_t)cpu->ax, 0xfu); /* +1F13 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f23; /* +1F15 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x10u); /* +1F17 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f23; /* +1F19 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x1fu); /* +1F1B */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f23; /* +1F1D */
    cmp8(cpu, (uint8_t)cpu->ax, 0x22u); /* +1F1F */
    if ((cpu->flags & 0x40u) == 0u) goto L_1f2d; /* +1F21 */

L_1f23:
    set_al(cpu, (uint8_t)(0x2u)); /* +1F23 */
    push16(ram, cpu, cpu->ax); /* +1F25 */
    push16(ram, cpu, cpu->cs); /* +1F26 */
    status = call_loader(ram, cpu, 0x1F2Au, system_cs,
                         max_operands, max_string_iterations);
    if (status != PR_U25_1EA6_OK) return status; /* +1F27 */
    goto L_1faf; /* +1F2A */

L_1f2d:
    cmp8(cpu, (uint8_t)cpu->ax, 0x4u); /* +1F2D */
    if ((cpu->flags & 1u) != 0u) goto L_1f35; /* +1F2F */
    cmp8(cpu, (uint8_t)cpu->ax, 0x7u); /* +1F31 */
    if ((cpu->flags & 0x41u) != 0u) goto L_1f59; /* +1F33 */

L_1f35:
    cmp8(cpu, (uint8_t)cpu->ax, 0xbu); /* +1F35 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f59; /* +1F37 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x21u); /* +1F39 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f59; /* +1F3B */
    cmp8(cpu, (uint8_t)cpu->ax, 0x28u); /* +1F3D */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f59; /* +1F3F */
    cmp8(cpu, (uint8_t)cpu->ax, 0x2au); /* +1F41 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f59; /* +1F43 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x2fu); /* +1F45 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f59; /* +1F47 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x30u); /* +1F49 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f59; /* +1F4B */
    cmp8(cpu, (uint8_t)cpu->ax, 0x35u); /* +1F4D */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f59; /* +1F4F */
    cmp8(cpu, (uint8_t)cpu->ax, 0x37u); /* +1F51 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f59; /* +1F53 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x3bu); /* +1F55 */
    if ((cpu->flags & 0x40u) == 0u) goto L_1f62; /* +1F57 */

L_1f59:
    set_al(cpu, (uint8_t)(0x3u)); /* +1F59 */
    push16(ram, cpu, cpu->ax); /* +1F5B */
    push16(ram, cpu, cpu->cs); /* +1F5C */
    status = call_loader(ram, cpu, 0x1F60u, system_cs,
                         max_operands, max_string_iterations);
    if (status != PR_U25_1EA6_OK) return status; /* +1F5D */
    goto L_1faf; /* +1F60 */

L_1f62:
    cmp8(cpu, (uint8_t)cpu->ax, 0x14u); /* +1F62 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f6a; /* +1F64 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x23u); /* +1F66 */
    if ((cpu->flags & 0x40u) == 0u) goto L_1f73; /* +1F68 */

L_1f6a:
    set_al(cpu, (uint8_t)(0x4u)); /* +1F6A */
    push16(ram, cpu, cpu->ax); /* +1F6C */
    push16(ram, cpu, cpu->cs); /* +1F6D */
    status = call_loader(ram, cpu, 0x1F71u, system_cs,
                         max_operands, max_string_iterations);
    if (status != PR_U25_1EA6_OK) return status; /* +1F6E */
    goto L_1faf; /* +1F71 */

L_1f73:
    cmp8(cpu, (uint8_t)cpu->ax, 0x2eu); /* +1F73 */
    if ((cpu->flags & 0x40u) == 0u) goto L_1f80; /* +1F75 */
    set_al(cpu, (uint8_t)(0x5u)); /* +1F77 */
    push16(ram, cpu, cpu->ax); /* +1F79 */
    push16(ram, cpu, cpu->cs); /* +1F7A */
    status = call_loader(ram, cpu, 0x1F7Eu, system_cs,
                         max_operands, max_string_iterations);
    if (status != PR_U25_1EA6_OK) return status; /* +1F7B */
    goto L_1faf; /* +1F7E */

L_1f80:
    cmp8(cpu, (uint8_t)cpu->ax, 0x1eu); /* +1F80 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1f88; /* +1F82 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x2cu); /* +1F84 */
    if ((cpu->flags & 0x40u) == 0u) goto L_1f91; /* +1F86 */

L_1f88:
    set_al(cpu, (uint8_t)(0x6u)); /* +1F88 */
    push16(ram, cpu, cpu->ax); /* +1F8A */
    push16(ram, cpu, cpu->cs); /* +1F8B */
    status = call_loader(ram, cpu, 0x1F8Fu, system_cs,
                         max_operands, max_string_iterations);
    if (status != PR_U25_1EA6_OK) return status; /* +1F8C */
    goto L_1faf; /* +1F8F */

L_1f91:
    cmp8(cpu, (uint8_t)cpu->ax, 0x27u); /* +1F91 */
    if ((cpu->flags & 0x40u) == 0u) goto L_1f9e; /* +1F93 */
    set_al(cpu, (uint8_t)(0x8u)); /* +1F95 */
    push16(ram, cpu, cpu->ax); /* +1F97 */
    push16(ram, cpu, cpu->cs); /* +1F98 */
    status = call_loader(ram, cpu, 0x1F9Cu, system_cs,
                         max_operands, max_string_iterations);
    if (status != PR_U25_1EA6_OK) return status; /* +1F99 */
    goto L_1faf; /* +1F9C */

L_1f9e:
    cmp8(cpu, (uint8_t)cpu->ax, 0x29u); /* +1F9E */
    if ((cpu->flags & 0x40u) == 0u) goto L_1fab; /* +1FA0 */
    set_al(cpu, (uint8_t)(0xeu)); /* +1FA2 */
    push16(ram, cpu, cpu->ax); /* +1FA4 */
    push16(ram, cpu, cpu->cs); /* +1FA5 */
    status = call_loader(ram, cpu, 0x1FA9u, system_cs,
                         max_operands, max_string_iterations);
    if (status != PR_U25_1EA6_OK) return status; /* +1FA6 */
    goto L_1faf; /* +1FA9 */

L_1fab:
    write16(ram, cpu->ds, (uint16_t)(0x49EDu), (uint16_t)(increment(cpu, read16(ram, cpu->ds, (uint16_t)(0x49EDu)), 16u))); /* +1FAB */

L_1faf:
    cpu->bp = (uint16_t)(pop16(ram, cpu)); /* +1FAF */
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    return PR_U25_1EA6_OK; /* +1FB0 */
}
