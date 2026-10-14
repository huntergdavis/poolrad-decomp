#include "poolrad/recovered/resident_464_bitmask_frame.h"
#include "poolrad/recovered/resident_464_bitmask.h"

int pr_r464_bitmask_frame(pr_slot_a_directory_state *s)
{
    size_t a[18];
    uint16_t sp, first, second, ret_ip, ret_cs;
    uint8_t selector, result;
    unsigned i;
    if (!s || !s->ram || s->ram_size > 0x100000u || s->cpu.ip != 0u)
        return 0;
    sp = s->cpu.sp;
    if (sp < 8u || sp > 0xfff6u) return 0;
    for (i = 0; i < 18; ++i) {
        a[i] = (((size_t)s->cpu.ss << 4) + (uint16_t)(sp + i - 8u)) & 0xfffffu;
        if (a[i] >= s->ram_size) return 0;
    }
    first = (uint16_t)(s->ram[a[12]] | ((uint16_t)s->ram[a[13]] << 8));
    second = (uint16_t)(s->ram[a[14]] | ((uint16_t)s->ram[a[15]] << 8));
    selector = s->ram[a[16]];
    ret_ip = (uint16_t)(s->ram[a[8]] | ((uint16_t)s->ram[a[9]] << 8));
    ret_cs = (uint16_t)(s->ram[a[10]] | ((uint16_t)s->ram[a[11]] << 8));
    result = pr_r464_bitmask(first, second, selector);
    s->ram[a[1]] = 3; /* BP-5: final four-byte loop index */
    s->ram[a[2]] = 0; /* BP-4: exhausted bit position */
    s->ram[a[3]] = (uint8_t)((uint16_t)selector << 4);
    s->ram[a[4]] = result; /* BP-2: accumulator */
    s->ram[a[5]] = result; /* BP-1: function result */
    s->ram[a[6]] = (uint8_t)s->cpu.bp;
    s->ram[a[7]] = (uint8_t)(s->cpu.bp >> 8);
    s->cpu.ax = result;
    s->cpu.cx = 4;
    s->cpu.di = 3;
    /* Last CMP at +0066 compares index 3 with 3. */
    s->cpu.flags = (uint16_t)((s->cpu.flags & (uint16_t)~0x08d5u) | 0x0044u);
    s->cpu.sp = (uint16_t)(sp + 10u);
    s->cpu.ip = ret_ip;
    s->cpu.cs = ret_cs;
    return 1;
}
