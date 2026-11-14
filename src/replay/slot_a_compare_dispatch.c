#include "poolrad/replay/slot_a_compare_dispatch.h"
#include "poolrad/replay/slot_a_dispatch_prologue.h"
#include "poolrad/replay/slot_a_dispatch_first_branch.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v)
{ size_t p; c->sp=(uint16_t)(c->sp-2u); p=phys(c->ss,c->sp); r[p]=(uint8_t)v; r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
uint16_t pr_slot_a_compare_dispatch(uint8_t *ram,size_t size,pr_u1c_3171_cpu *cpu,
 const uint8_t *prologue,const uint8_t *first_branch,const uint8_t *select)
{
    size_t pos,p1,p2; unsigned i;
    if(!ram||!cpu||!prologue||!first_branch||!select||size!=0x100000u||cpu->cs!=0x3582u||cpu->ip!=0x31a4u) return 0x31a4u;
    pos=phys(cpu->cs,0x31a4); if(pos>size-4u||ram[pos]!=0x0eu||ram[pos+1]!=0xe8u||ram[pos+2]!=0x85u||ram[pos+3]!=0xfdu) return 0x31a4u;
    p1=phys(cpu->ss,(uint16_t)(cpu->sp-2u)); p2=phys(cpu->ss,(uint16_t)(cpu->sp-4u));
    for(i=0;i<4u;i++) if(pos+i==p1||pos+i==((p1+1u)&0xfffffu)||pos+i==p2||pos+i==((p2+1u)&0xfffffu)) return 0x31a4u;
    push(ram,cpu,cpu->cs); push(ram,cpu,0x31a8u); cpu->ip=0x2f2du;
    if(pr_slot_a_dispatch_prologue(ram,size,prologue,6u,cpu)) return cpu->ip;
    if(pr_slot_a_dispatch_first_branch(ram,size,first_branch,4u,cpu)) return cpu->ip;
    if(cpu->ip!=0x2f3eu || (uint8_t)cpu->ax!=3u) return cpu->ip;
    pos=phys(cpu->cs,0x2f3e); if(pos>size-26u||memcmp(ram+pos,select,26u)) return cpu->ip;
    cpu->flags=(uint16_t)((cpu->flags&(uint16_t)~0x08d5u)|0x0044u); cpu->ip=0x2f58u; return cpu->ip;
}
