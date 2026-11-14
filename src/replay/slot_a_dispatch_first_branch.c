#include "poolrad/replay/slot_a_dispatch_first_branch.h"
#include <string.h>
int pr_slot_a_dispatch_first_branch(const uint8_t *ram, size_t size,
    const uint8_t *code, size_t code_size, pr_u1c_3171_cpu *cpu)
{
    size_t pos; uint8_t value, parity; uint16_t flags;
    if (!ram || !code || !cpu || size!=0x100000u || code_size!=4u || cpu->ip!=0x2f33u) return 1;
    pos=(((size_t)cpu->cs<<4)+0x2f33u)&0xfffffu;
    if (pos>size-4u || memcmp(ram+pos,code,4u)) return 1;
    value=(uint8_t)cpu->ax; parity=value;
    parity^=(uint8_t)(parity>>4); parity^=(uint8_t)(parity>>2); parity^=(uint8_t)(parity>>1);
    flags=(uint16_t)(((parity&1u)==0u ? 4u : 0u) | (value==0 ? 0x40u : 0u) | (value&0x80u));
    cpu->flags=(uint16_t)((cpu->flags & (uint16_t)~0x08d5u)|flags);
    cpu->ip=value ? 0x2f3eu : 0x2f37u; return 0;
}
