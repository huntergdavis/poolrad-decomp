#include "poolrad/replay/slot_a_skip_loader_entry.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v) { size_t p;c->sp=(uint16_t)(c->sp-2u);p=phys(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
pr_skip_loader_entry_status pr_slot_a_skip_loader_entry(uint8_t *ram,size_t size,const uint8_t *code,size_t code_size,pr_u1c_3171_cpu *cpu)
{
    size_t p,a;uint16_t rel;unsigned i,j;
    if(!ram||size!=0x100000u||!code||code_size!=7u||!cpu||cpu->ip!=0x1f23u||cpu->sp<6u)return PR_SKIP_LOADER_ARGUMENT;
    p=phys(cpu->cs,0x1f23);if(p>size-7u||memcmp(ram+p,code,4u)||code[0]!=0xb0u||code[1]!=2u||code[2]!=0x50u||code[3]!=0x0eu)return PR_SKIP_LOADER_SETUP_CODE;
    for(i=2u;i<=6u;i+=2u){a=phys(cpu->ss,(uint16_t)(cpu->sp-i));for(j=0;j<7u;j++)if(a==p+j||((a+1u)&0xfffffu)==p+j)return PR_SKIP_LOADER_ARGUMENT;}
    cpu->ax=(uint16_t)((cpu->ax&0xff00u)|2u);push(ram,cpu,cpu->ax);push(ram,cpu,cpu->cs);cpu->ip=0x1f27u;
    rel=(uint16_t)(ram[p+5u]|((uint16_t)ram[p+6u]<<8));
    if(memcmp(ram+p+4u,code+4u,3u)||ram[p+4u]!=0xe8u||(uint16_t)(0x1f2au+rel)!=0u)return PR_SKIP_LOADER_CALL_CODE;
    push(ram,cpu,0x1f2au);cpu->ip=0;return PR_SKIP_LOADER_REACHED;
}
