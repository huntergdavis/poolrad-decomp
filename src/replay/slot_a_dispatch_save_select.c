#include "poolrad/replay/slot_a_dispatch_save_select.h"
#include <string.h>
int pr_slot_a_dispatch_save_select(const uint8_t *ram, size_t size,
    const uint8_t *code, size_t code_size, pr_u1c_3171_cpu *cpu)
{
    size_t pos;
    if (!ram || !code || !cpu || size!=0x100000u || code_size!=0x47u || cpu->ip!=0x2f3eu || (uint8_t)cpu->ax!=9u) return 1;
    pos=(((size_t)cpu->cs<<4)+0x2f3eu)&0xfffffu;
    if (pos>size-0x47u || memcmp(ram+pos,code,0x47u)) return 1;
    /* Final CMP AL,09 yields zero, even parity, no carry/aux/sign/overflow. */
    cpu->flags=(uint16_t)((cpu->flags & (uint16_t)~0x08d5u)|0x0044u);
    cpu->ip=0x2f85u; return 0;
}
