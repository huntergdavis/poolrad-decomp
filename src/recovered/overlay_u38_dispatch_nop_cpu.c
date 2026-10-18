/* Four original identities share this exact body; see U38_DISPATCH_NOP_CPU.md. */
#include "poolrad/recovered/overlay_u38_dispatch_nop_cpu.h"
#define MASK (PR_U38_NOP_RAM_SIZE-1u)
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word(const uint8_t *ram,size_t p) { return (uint16_t)(ram[p]|((uint16_t)ram[(p+1)&MASK]<<8)); }
pr_u38_nop_status pr_u38_dispatch_nop_cpu(uint8_t *ram,size_t size,pr_u38_nop_cpu *c)
{
    size_t p;
    if(ram==NULL || c==NULL || size!=PR_U38_NOP_RAM_SIZE || c->sp<2 || c->sp>0xFFFC ||
       (c->ip!=0x647 && c->ip!=0x1355 && c->ip!=0x135C && c->ip!=0x319B))return PR_U38_NOP_ARGUMENT;
    c->sp=(uint16_t)(c->sp-2u);p=physical(c->ss,c->sp);
    ram[p]=(uint8_t)c->bp;ram[(p+1)&MASK]=(uint8_t)(c->bp>>8);c->bp=c->sp;
    c->bp=word(ram,physical(c->ss,c->sp));c->sp=(uint16_t)(c->sp+2u);
    p=physical(c->ss,c->sp);c->ip=word(ram,p);c->cs=word(ram,(p+2)&MASK);
    c->sp=(uint16_t)(c->sp+14u);return PR_U38_NOP_OK;
}
