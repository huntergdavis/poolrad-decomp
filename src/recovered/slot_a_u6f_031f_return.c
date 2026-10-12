#include "poolrad/recovered/slot_a_u6f_031f_return.h"

#define U6F_SEG 0x2a8eu
#define ZF 0x0040u
#define ARITH_FLAGS 0x08d5u

static int byte_address(const pr_slot_a_directory_state *s,
                        uint16_t off, size_t *linear)
{
    *linear = (((size_t)s->cpu.ss << 4) + off) & 0xfffffu;
    return *linear < s->ram_size;
}

static int read_word(const pr_slot_a_directory_state *s,
                     uint16_t off, uint16_t *value)
{
    size_t low, high;
    if (!s->ram || !byte_address(s, off, &low) ||
        !byte_address(s, (uint16_t)(off + 1u), &high)) return 0;
    *value = (uint16_t)(s->ram[low] | ((uint16_t)s->ram[high] << 8));
    return 1;
}

static uint16_t cmp16_flags(uint16_t old_flags, uint16_t a, uint16_t b)
{
    uint16_t r = (uint16_t)(a - b);
    uint8_t parity = (uint8_t)r;
    uint16_t f = (uint16_t)(old_flags & (uint16_t)~ARITH_FLAGS);
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (a < b) f |= 0x0001u;
    if (!(parity & 1u)) f |= 0x0004u;
    if (((a ^ b ^ r) & 0x0010u) != 0u) f |= 0x0010u;
    if (r == 0u) f |= ZF;
    if ((r & 0x8000u) != 0u) f |= 0x0080u;
    if (((a ^ b) & (a ^ r) & 0x8000u) != 0u) f |= 0x0800u;
    return f;
}

pr_slot_a_u6f_031f_status pr_slot_a_u6f_031f_step(
    pr_slot_a_directory_state *s)
{
    uint16_t value, ip, cs;
    if (!s || s->cpu.cs != U6F_SEG || s->ram_size > 0x100000u)
        return PR_SLOT_A_U6F_031F_BOUNDARY;
    switch (s->cpu.ip) {
    case 0x031fu: /* CMP word [BP-2],0, saved error-5 path only */
    case 0x0325u: /* CMP word [BP-2],5 */
        if (!read_word(s, (uint16_t)(s->cpu.bp - 2u), &value) ||
            value != 5u) return PR_SLOT_A_U6F_031F_BOUNDARY;
        s->cpu.flags = cmp16_flags(s->cpu.flags, value,
                                   s->cpu.ip == 0x031fu ? 0u : 5u);
        s->cpu.ip = s->cpu.ip == 0x031fu ? 0x0323u : 0x0329u;
        return PR_SLOT_A_U6F_031F_OK;
    case 0x0323u: /* JE +032B; saved error 5 falls through */
        if (s->cpu.flags & ZF) return PR_SLOT_A_U6F_031F_BOUNDARY;
        s->cpu.ip = 0x0325u;
        return PR_SLOT_A_U6F_031F_OK;
    case 0x0329u: /* JNE +02B5; saved error 5 falls through */
        if (!(s->cpu.flags & ZF)) return PR_SLOT_A_U6F_031F_BOUNDARY;
        s->cpu.ip = 0x032bu;
        return PR_SLOT_A_U6F_031F_OK;
    case 0x032bu: /* MOV SP,BP */
        s->cpu.sp = s->cpu.bp;
        s->cpu.ip = 0x032du;
        return PR_SLOT_A_U6F_031F_OK;
    case 0x032du: /* POP BP */
        if (!read_word(s, s->cpu.sp, &value))
            return PR_SLOT_A_U6F_031F_BOUNDARY;
        s->cpu.bp = value;
        s->cpu.sp = (uint16_t)(s->cpu.sp + 2u);
        s->cpu.ip = 0x032eu;
        return PR_SLOT_A_U6F_031F_OK;
    case 0x032eu: /* RETF to the observed outer caller */
        if (!read_word(s, s->cpu.sp, &ip) ||
            !read_word(s, (uint16_t)(s->cpu.sp + 2u), &cs) ||
            ip != 0x19eau || cs != U6F_SEG)
            return PR_SLOT_A_U6F_031F_BOUNDARY;
        s->cpu.sp = (uint16_t)(s->cpu.sp + 4u);
        s->cpu.cs = cs;
        s->cpu.ip = ip;
        return PR_SLOT_A_U6F_031F_OK;
    default:
        return PR_SLOT_A_U6F_031F_BOUNDARY;
    }
}
