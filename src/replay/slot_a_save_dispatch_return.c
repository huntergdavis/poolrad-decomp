#include "poolrad/replay/slot_a_save_dispatch_return.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static uint16_t get(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8)); }
int pr_slot_a_save_dispatch_return(const uint8_t *ram,size_t size,const uint8_t *jump,
 size_t jump_size,const uint8_t *tail,size_t tail_size,pr_u1c_3171_cpu *cpu)
{
    size_t a,b; uint16_t bp,ip,cs,sp;
    if(!ram||!jump||!tail||!cpu||size!=0x100000u||jump_size!=3u||tail_size!=2u||cpu->ip!=0x2f89u||cpu->sp>0xfffau) return 1;
    a=phys(cpu->cs,0x2f89); b=phys(cpu->cs,0x316f);
    if(a>size-3u||b>size-2u||memcmp(ram+a,jump,3u)||memcmp(ram+b,tail,2u)) return 1;
    sp=cpu->sp; bp=get(ram,phys(cpu->ss,sp)); ip=get(ram,phys(cpu->ss,(uint16_t)(sp+2u))); cs=get(ram,phys(cpu->ss,(uint16_t)(sp+4u)));
    cpu->bp=bp; cpu->sp=(uint16_t)(sp+6u); cpu->ip=ip; cpu->cs=cs; return 0;
}
