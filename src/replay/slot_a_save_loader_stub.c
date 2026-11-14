#include "poolrad/replay/slot_a_save_loader_stub.h"
#include <string.h>
int pr_slot_a_save_loader_stub(const uint8_t *ram, size_t size,
    const uint8_t *stub, size_t stub_size, pr_u1c_3171_cpu *cpu)
{
    size_t pos; uint16_t target;
    if (!ram || !stub || !cpu || size!=0x100000u || stub_size!=5u || cpu->ip!=0x002au) return 1;
    pos=(((size_t)cpu->cs<<4)+0x002au)&0xfffffu;
    if(pos>size-5u || memcmp(ram+pos,stub,5u) || stub[0]!=0xeau || stub[1]!=0u || stub[2]!=0u) return 1;
    target=(uint16_t)(stub[3]|((uint16_t)stub[4]<<8)); cpu->cs=target; cpu->ip=0; return 0;
}
