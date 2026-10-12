#include "poolrad/recovered/slot_a_u6f_19ea_caller.h"

#define U6F_SEG 0x2a8eu
#define ZF 0x0040u
#define ARITH_FLAGS 0x08d5u

static int read_stack_byte(const pr_slot_a_directory_state *s,
                           uint16_t displacement, uint8_t *value)
{
    uint16_t off = (uint16_t)(s->cpu.bp - displacement);
    size_t linear = (((size_t)s->cpu.ss << 4) + off) & 0xfffffu;
    if (!s->ram || linear >= s->ram_size) return 0;
    *value = s->ram[linear];
    return 1;
}

static uint16_t cmp8_flags(uint16_t old_flags, uint8_t a, uint8_t b)
{
    uint8_t r = (uint8_t)(a - b);
    uint8_t parity = r;
    uint16_t f = (uint16_t)(old_flags & (uint16_t)~ARITH_FLAGS);
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (a < b) f |= 0x0001u;
    if (!(parity & 1u)) f |= 0x0004u;
    if (((a ^ b ^ r) & 0x10u) != 0u) f |= 0x0010u;
    if (r == 0u) f |= ZF;
    if ((r & 0x80u) != 0u) f |= 0x0080u;
    if (((a ^ b) & (a ^ r) & 0x80u) != 0u) f |= 0x0800u;
    return f;
}

pr_slot_a_u6f_19ea_status pr_slot_a_u6f_19ea_step(
    pr_slot_a_directory_state *s)
{
    uint8_t value;
    if (!s || s->cpu.cs != U6F_SEG || s->ram_size > 0x100000u)
        return PR_SLOT_A_U6F_19EA_BOUNDARY;
    switch (s->cpu.ip) {
    case 0x19eau: /* MOV AL,SS:[BP-01C9] */
        if (!read_stack_byte(s, 0x01c9u, &value))
            return PR_SLOT_A_U6F_19EA_BOUNDARY;
        s->cpu.ax = (uint16_t)((s->cpu.ax & 0xff00u) | value);
        s->cpu.ip = 0x19eeu;
        return PR_SLOT_A_U6F_19EA_OK;
    case 0x19eeu: /* CMP AL,SS:[BP-0212] */
        if (!read_stack_byte(s, 0x0212u, &value))
            return PR_SLOT_A_U6F_19EA_BOUNDARY;
        s->cpu.flags = cmp8_flags(s->cpu.flags, (uint8_t)s->cpu.ax,
                                   value);
        s->cpu.ip = 0x19f2u;
        return PR_SLOT_A_U6F_19EA_OK;
    case 0x19f2u: /* JE +19F7; saved comparison is unequal */
        if (s->cpu.flags & ZF) return PR_SLOT_A_U6F_19EA_BOUNDARY;
        s->cpu.ip = 0x19f4u;
        return PR_SLOT_A_U6F_19EA_OK;
    case 0x19f4u: /* JMP +1937 */
        s->cpu.ip = 0x1937u;
        return PR_SLOT_A_U6F_19EA_OK;
    default:
        return PR_SLOT_A_U6F_19EA_BOUNDARY;
    }
}
