#include "poolrad/recovered/slot_a_u6f_1937_precall.h"

#define U6F_SEG 0x2a8eu
#define ARITH_FLAGS 0x08d5u

static int address(const pr_slot_a_directory_state *s, uint16_t off,
                   size_t *linear)
{
    *linear = (((size_t)s->cpu.ss << 4) + off) & 0xfffffu;
    return s->ram && *linear < s->ram_size;
}

static int read_byte(const pr_slot_a_directory_state *s, uint16_t off,
                     uint8_t *value)
{
    size_t linear;
    if (!address(s, off, &linear)) return 0;
    *value = s->ram[linear];
    return 1;
}

static int push_word(pr_slot_a_directory_state *s, uint16_t value)
{
    uint16_t off = (uint16_t)(s->cpu.sp - 2u);
    size_t low, high;
    if (!address(s, off, &low) ||
        !address(s, (uint16_t)(off + 1u), &high)) return 0;
    s->ram[low] = (uint8_t)value;
    s->ram[high] = (uint8_t)(value >> 8);
    s->cpu.sp = off;
    return 1;
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return !(value & 1u);
}

static uint16_t inc8_flags(uint16_t old, uint8_t before, uint8_t after)
{
    uint16_t f = (uint16_t)(old & (uint16_t)~(ARITH_FLAGS & ~1u));
    if (even_parity(after)) f |= 0x0004u;
    if ((before & 0x0fu) == 0x0fu) f |= 0x0010u;
    if (after == 0u) f |= 0x0040u;
    if (after & 0x80u) f |= 0x0080u;
    if (before == 0x7fu) f |= 0x0800u;
    return f;
}

pr_slot_a_u6f_1937_status pr_slot_a_u6f_1937_step(
    pr_slot_a_directory_state *s)
{
    size_t linear;
    uint8_t value;
    uint16_t pushed, next;
    uint32_t product;

    if (!s || s->cpu.cs != U6F_SEG || s->ram_size > 0x100000u)
        return PR_SLOT_A_U6F_1937_BOUNDARY;
    switch (s->cpu.ip) {
    case 0x1937u: /* INC byte SS:[BP-01C9] */
        if (!address(s, (uint16_t)(s->cpu.bp - 0x01c9u), &linear))
            return PR_SLOT_A_U6F_1937_BOUNDARY;
        value = s->ram[linear];
        s->ram[linear] = (uint8_t)(value + 1u);
        s->cpu.flags = inc8_flags(s->cpu.flags, value, s->ram[linear]);
        s->cpu.ip = 0x193bu;
        return PR_SLOT_A_U6F_1937_OK;
    case 0x193bu: s->cpu.di = (uint16_t)(s->cpu.bp - 0x0414u);
                  s->cpu.ip = 0x193fu; return PR_SLOT_A_U6F_1937_OK;
    case 0x1941u: s->cpu.di = (uint16_t)(s->cpu.bp - 0x0314u);
                  s->cpu.ip = 0x1945u; return PR_SLOT_A_U6F_1937_OK;
    case 0x1947u: /* MOV AL,SS:[BP-01C9] */
        if (!read_byte(s, (uint16_t)(s->cpu.bp - 0x01c9u), &value))
            return PR_SLOT_A_U6F_1937_BOUNDARY;
        s->cpu.ax = (uint16_t)((s->cpu.ax & 0xff00u) | value);
        s->cpu.ip = 0x194bu;
        return PR_SLOT_A_U6F_1937_OK;
    case 0x194bu: /* XOR AH,AH; AF is undefined, normalized to zero. */
        s->cpu.ax &= 0x00ffu;
        s->cpu.flags = (uint16_t)((s->cpu.flags &
                                    (uint16_t)~ARITH_FLAGS) | 0x0044u);
        s->cpu.ip = 0x194du;
        return PR_SLOT_A_U6F_1937_OK;
    case 0x194du: s->cpu.dx = 0x0029u; s->cpu.ip = 0x1950u;
                  return PR_SLOT_A_U6F_1937_OK;
    case 0x1950u: /* MUL DX: pinned MULW (SF/AF kept, ZF from AX alone, PF over DX:AX, CF=OF=(DX!=0)) */
        product = (uint32_t)s->cpu.ax * s->cpu.dx;
        s->cpu.ax = (uint16_t)product;
        s->cpu.dx = (uint16_t)(product >> 16);
        {
            uint16_t w = (uint16_t)(s->cpu.ax ^ s->cpu.dx);
            w ^= (uint16_t)(w >> 8); w ^= (uint16_t)(w >> 4); w ^= (uint16_t)(w >> 2); w ^= (uint16_t)(w >> 1);
            s->cpu.flags &= (uint16_t)~0x0845u;
            if (s->cpu.dx) s->cpu.flags |= 0x0801u;
            if (s->cpu.ax == 0u) s->cpu.flags |= 0x0040u;
            if ((w & 1u) == 0u) s->cpu.flags |= 0x0004u;
        }
        s->cpu.ip = 0x1952u;
        return PR_SLOT_A_U6F_1937_OK;
    case 0x1952u: s->cpu.di = s->cpu.ax; s->cpu.ip = 0x1954u;
                  return PR_SLOT_A_U6F_1937_OK;
    case 0x1954u: s->cpu.di = (uint16_t)(s->cpu.bp + s->cpu.di - 0x0171u);
                  s->cpu.ip = 0x1958u; return PR_SLOT_A_U6F_1937_OK;
    case 0x195au: s->cpu.di = (uint16_t)(s->cpu.bp - 0x0214u);
                  s->cpu.ip = 0x195eu; return PR_SLOT_A_U6F_1937_OK;
    case 0x1960u: s->cpu.di = 0x15e7u; s->cpu.ip = 0x1963u;
                  return PR_SLOT_A_U6F_1937_OK;
    case 0x193fu: case 0x1945u: case 0x1958u: case 0x195eu:
        pushed = s->cpu.ss;
        next = (uint16_t)(s->cpu.ip + 1u);
        break;
    case 0x1940u: case 0x1946u: case 0x1959u: case 0x195fu:
    case 0x1964u:
        pushed = s->cpu.di;
        next = (uint16_t)(s->cpu.ip + 1u);
        break;
    case 0x1963u:
        pushed = s->cpu.cs;
        next = 0x1964u;
        break;
    default:
        return PR_SLOT_A_U6F_1937_BOUNDARY;
    }
    if (!push_word(s, pushed)) return PR_SLOT_A_U6F_1937_BOUNDARY;
    s->cpu.ip = next;
    return PR_SLOT_A_U6F_1937_OK;
}
