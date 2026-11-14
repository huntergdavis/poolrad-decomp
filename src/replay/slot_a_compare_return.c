#include "poolrad/replay/slot_a_compare_return.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static uint16_t get(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8)); }
int pr_slot_a_compare_return(const uint8_t *ram,size_t size,const uint8_t *tail,size_t tail_size,pr_u1c_3171_cpu *cpu)
{
    size_t p;uint16_t sp,bp,ip,cs;
    if(!ram||!tail||!cpu||size!=0x100000u||tail_size!=4u||cpu->ip!=0x019fu||cpu->bp>0xfffau)return 1;
    p=phys(cpu->cs,0x019f);if(p>size-4u||memcmp(ram+p,tail,4u))return 1;
    sp=cpu->bp;bp=get(ram,phys(cpu->ss,sp));ip=get(ram,phys(cpu->ss,(uint16_t)(sp+2u)));cs=get(ram,phys(cpu->ss,(uint16_t)(sp+4u)));
    cpu->bp=bp;cpu->sp=(uint16_t)(sp+6u);cpu->ip=ip;cpu->cs=cs;return 0;
}
