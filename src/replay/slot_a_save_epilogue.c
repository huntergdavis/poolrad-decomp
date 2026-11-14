#include "poolrad/replay/slot_a_save_epilogue.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static uint16_t get(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&0xfffffu]<<8)); }
int pr_slot_a_save_epilogue(const uint8_t *ram,size_t size,const uint8_t *jump,
 size_t jump_size,const uint8_t *tail,size_t tail_size,pr_u1c_3171_cpu *cpu)
{
    size_t a,b; uint16_t bp,ip,cs,sp;
    if(!ram||!jump||!tail||!cpu||size!=0x100000u||jump_size!=2u||tail_size!=4u||cpu->ip!=0x02cdu||cpu->bp>0xfffau) return 1;
    a=phys(cpu->cs,0x02cd); b=phys(cpu->cs,0x02dc);
    if(a>size-2u||b>size-4u||memcmp(ram+a,jump,2u)||memcmp(ram+b,tail,4u)) return 1;
    sp=cpu->bp; bp=get(ram,phys(cpu->ss,sp)); ip=get(ram,phys(cpu->ss,(uint16_t)(sp+2u))); cs=get(ram,phys(cpu->ss,(uint16_t)(sp+4u)));
    cpu->bp=bp; cpu->sp=(uint16_t)(sp+6u); cpu->ip=ip; cpu->cs=cs; return 0;
}
