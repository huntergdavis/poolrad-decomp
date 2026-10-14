#include "poolrad/recovered/resident_464_reverse_1484.h"

uint8_t pr_r464_reverse_1484(uint16_t argument)
{
    uint8_t v = (uint8_t)argument;
    v = (uint8_t)(((v & 0x55u) << 1) | ((v >> 1) & 0x55u));
    v = (uint8_t)(((v & 0x33u) << 2) | ((v >> 2) & 0x33u));
    return (uint8_t)((v << 4) | (v >> 4));
}

static size_t stack_address(const pr_slot_a_directory_state *s, uint16_t off)
{
    return (((size_t)s->cpu.ss << 4) + off) & 0xfffffu;
}

int pr_r464_reverse_1484_frame(pr_slot_a_directory_state *s)
{
    size_t a[10];
    uint16_t sp, ret_ip, ret_cs, flags;
    uint8_t result, parity;
    unsigned i;

    if (!s || !s->ram || s->ram_size > 0x100000u || s->cpu.ip != 0x1484u)
        return 0;
    sp = s->cpu.sp;
    if (sp < 4u || sp > 0xfffau) return 0;
    /* [-4,-3] locals, [-2,-1] saved BP, [0..3] return pointer,
     * [4..5] argument WORD. Validate before any stack write. */
    for (i = 0; i < 10; ++i) {
        a[i] = stack_address(s, (uint16_t)(sp + i - 4u));
        if (a[i] >= s->ram_size) return 0;
    }
    result = pr_r464_reverse_1484(s->ram[a[8]]);
    ret_ip = (uint16_t)(s->ram[a[4]] | ((uint16_t)s->ram[a[5]] << 8));
    ret_cs = (uint16_t)(s->ram[a[6]] | ((uint16_t)s->ram[a[7]] << 8));
    /* Last ADD (+14F7) adds bit 7, shifted to bit 0, to an even byte.
     * CF, AF and OF are zero; SF/ZF/PF depend on the returned byte. */
    flags = (uint16_t)(s->cpu.flags & (uint16_t)~0x08d5u);
    parity = result;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (!(parity & 1u)) flags |= 0x0004u;
    if (!result) flags |= 0x0040u;
    if (result & 0x80u) flags |= 0x0080u;

    s->ram[a[2]] = (uint8_t)s->cpu.bp;
    s->ram[a[3]] = (uint8_t)(s->cpu.bp >> 8);
    s->ram[a[0]] = result;
    s->ram[a[1]] = result;
    s->cpu.ax = result;
    s->cpu.cx = 7;
    s->cpu.flags = flags;
    s->cpu.sp = (uint16_t)(sp + 6u);
    s->cpu.ip = ret_ip;
    s->cpu.cs = ret_cs;
    return 1;
}
