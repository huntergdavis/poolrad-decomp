/* START.EXE SYSTEM +120F..+1244; docs/re/SYSTEM_SEED_FULLSTATE_120F.md. */
#include "poolrad/recovered/system_seed_fullstate_120f.h"

#define MASK (PR_SYSTEM_120F_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read_word(const uint8_t *ram, uint16_t segment,
                          uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void write_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                       uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint16_t add_high_byte(uint16_t word, uint8_t value)
{
    uint8_t high = (uint8_t)((word >> 8) + value);
    return (uint16_t)((word & 0x00FFu) | ((uint16_t)high << 8));
}

static uint16_t adc_zero_flags(uint16_t incoming, uint16_t before,
                               uint16_t result, uint16_t carry)
{
    uint8_t parity = (uint8_t)result;
    uint16_t flags = (uint16_t)(incoming & (uint16_t)~ARITH_FLAGS);
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (carry != 0u && before == 0xFFFFu) flags |= 0x0001u;
    if (!(parity & 1u)) flags |= 0x0004u;
    if (carry != 0u && (before & 0x000Fu) == 0x000Fu) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if (result & 0x8000u) flags |= 0x0080u;
    if (carry != 0u && before == 0x7FFFu) flags |= 0x0800u;
    return flags;
}

static int seed_aliases_code(const pr_system_120f_cpu *cpu)
{
    unsigned seed_byte, code_offset;
    for (seed_byte = 0u; seed_byte < 4u; ++seed_byte) {
        size_t seed_at = physical(cpu->ds, (uint16_t)(0x4428u + seed_byte));
        for (code_offset = 0x120Fu; code_offset <= 0x1244u; ++code_offset)
            if (seed_at == physical(cpu->cs, (uint16_t)code_offset))
                return 1;
    }
    return 0;
}

pr_system_120f_status pr_system_seed_fullstate_120f(
    uint8_t *ram, size_t ram_size, pr_system_120f_cpu *cpu)
{
    uint16_t ax, bx, cx, dx, before_dx, carry;
    uint32_t product;

    if (ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_120F_RAM_SIZE ||
        cpu->ip != 0x120Fu || seed_aliases_code(cpu))
        return PR_SYSTEM_120F_ARGUMENT;

    /* +120F..+1218: preserve the exact read order and live CS operand. */
    ax = read_word(ram, cpu->ds, 0x4428u);
    bx = read_word(ram, cpu->ds, 0x442Au);
    cx = ax;
    product = (uint32_t)ax * read_word(ram, cpu->cs, 0x1245u);
    ax = (uint16_t)product;
    dx = (uint16_t)(product >> 16);

    /* +121D..+1235: 16-bit shifts/adds and the two 8-bit DH additions. */
    cx = (uint16_t)(cx << 1);
    cx = (uint16_t)(cx << 1);
    cx = (uint16_t)(cx << 1);
    cx = add_high_byte(cx, (uint8_t)cx);
    dx = (uint16_t)(dx + cx);
    dx = (uint16_t)(dx + bx);
    bx = (uint16_t)(bx << 1);
    bx = (uint16_t)(bx << 1);
    dx = (uint16_t)(dx + bx);
    dx = add_high_byte(dx, (uint8_t)bx);
    cx = (uint16_t)((cx & 0xFF00u) | 5u);
    bx = (uint16_t)(bx << 5);
    dx = add_high_byte(dx, (uint8_t)bx);

    /* +1237 ADD AX,1; +123A ADC DX,0 is the last FLAGS writer. */
    carry = (uint16_t)(ax == 0xFFFFu);
    ax = (uint16_t)(ax + 1u);
    before_dx = dx;
    dx = (uint16_t)(dx + carry);
    cpu->flags = adc_zero_flags(cpu->flags, before_dx, dx, carry);

    cpu->ax = ax;
    cpu->bx = bx;
    cpu->cx = cx;
    cpu->dx = dx;

    /* +123D/+1240 write low then high; +1244 RET reads live SS:SP. */
    write_word(ram, cpu->ds, 0x4428u, ax);
    write_word(ram, cpu->ds, 0x442Au, dx);
    cpu->ip = read_word(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return PR_SYSTEM_120F_RETURNED;
}
