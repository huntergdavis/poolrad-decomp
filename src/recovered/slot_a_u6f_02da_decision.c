#include "poolrad/recovered/slot_a_u6f_02da_decision.h"

#define U6F_SEG 0x2a8eu
#define ZF 0x0040u
#define ARITH_FLAGS 0x08d5u

static int word_address(const pr_slot_a_directory_state *s,
                        size_t *low, size_t *high)
{
    uint16_t off = (uint16_t)(s->cpu.bp - 2u);
    *low = (((size_t)s->cpu.ss << 4) + off) & 0xfffffu;
    *high = (((size_t)s->cpu.ss << 4) +
             (uint16_t)(off + 1u)) & 0xfffffu;
    return *low < s->ram_size && *high < s->ram_size;
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

pr_slot_a_u6f_02da_status pr_slot_a_u6f_02da_step(
    pr_slot_a_directory_state *s)
{
    size_t low, high;
    uint16_t value;

    if (!s || s->cpu.cs != U6F_SEG || s->ram_size > 0x100000u)
        return PR_SLOT_A_U6F_02DA_BOUNDARY;
    switch (s->cpu.ip) {
    case 0x02dau: /* MOV [BP-2],AX */
        if (!s->ram || !word_address(s, &low, &high))
            return PR_SLOT_A_U6F_02DA_BOUNDARY;
        s->ram[low] = (uint8_t)s->cpu.ax;
        s->ram[high] = (uint8_t)(s->cpu.ax >> 8);
        s->cpu.ip = 0x02ddu;
        return PR_SLOT_A_U6F_02DA_OK;
    case 0x02ddu: /* CMP word [BP-2],0 */
    case 0x02e3u: /* CMP word [BP-2],5 */
        if (!s->ram || !word_address(s, &low, &high))
            return PR_SLOT_A_U6F_02DA_BOUNDARY;
        value = (uint16_t)(s->ram[low] |
                           ((uint16_t)s->ram[high] << 8));
        s->cpu.flags = cmp16_flags(s->cpu.flags, value,
                                   s->cpu.ip == 0x02ddu ? 0u : 5u);
        s->cpu.ip = s->cpu.ip == 0x02ddu ? 0x02e1u : 0x02e7u;
        return PR_SLOT_A_U6F_02DA_OK;
    case 0x02e1u: /* JE +031F */
        s->cpu.ip = (s->cpu.flags & ZF) ? 0x031fu : 0x02e3u;
        return PR_SLOT_A_U6F_02DA_OK;
    case 0x02e7u: /* JE +031F */
        s->cpu.ip = (s->cpu.flags & ZF) ? 0x031fu : 0x02e9u;
        return PR_SLOT_A_U6F_02DA_OK;
    default:
        return PR_SLOT_A_U6F_02DA_BOUNDARY;
    }
}
