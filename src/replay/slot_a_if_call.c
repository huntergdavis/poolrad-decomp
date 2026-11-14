#include "poolrad/replay/slot_a_if_call.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v) { size_t p;c->sp=(uint16_t)(c->sp-2u);p=phys(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8); }
pr_if_call_status pr_slot_a_if_call(uint8_t *ram,size_t size,const uint8_t *code,size_t code_size,pr_u1c_3171_cpu *cpu)
{
    size_t p,a;uint16_t rel;unsigned i,j;
    if(!ram||size!=0x100000u||!code||code_size!=4u||!cpu||cpu->ip!=0x3021u||cpu->sp!=cpu->bp||cpu->sp<4u)return PR_IF_CALL_ARGUMENT;
    p=phys(cpu->cs,0x3021);if(p>size-4u||ram[p]!=code[0]||code[0]!=0x0eu)return PR_IF_CALL_ENTRY_CODE;
    for(i=2u;i<=4u;i+=2u){a=phys(cpu->ss,(uint16_t)(cpu->sp-i));for(j=0;j<4u;j++)if(a==p+j||((a+1u)&0xfffffu)==p+j)return PR_IF_CALL_ARGUMENT;}
    push(ram,cpu,cpu->cs);cpu->ip=0x3022u;
    rel=(uint16_t)(ram[p+2u]|((uint16_t)ram[p+3u]<<8));
    if(memcmp(ram+p+1u,code+1u,3u)||ram[p+1u]!=0xe8u||(uint16_t)(0x3025u+rel)!=0x0b52u)return PR_IF_CALL_NEAR_CODE;
    push(ram,cpu,0x3025u);cpu->ip=0x0b52u;return PR_IF_CALL_REACHED;
}
