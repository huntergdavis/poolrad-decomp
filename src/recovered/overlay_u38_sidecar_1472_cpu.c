/* Complete U38 +1472; docs/re/U38_SIDECAR_1472_CPU.md.
 * Live-stub composition and word semantics follow accepted U38 +2DE4. */
#include "poolrad/recovered/overlay_u38_sidecar_1472_cpu.h"
#include "poolrad/recovered/overlay_u9e_list_append_0e52_cpu.h"
#include <string.h>
#define MASK (PR_U38_1472_RAM_SIZE-1u)
_Static_assert(sizeof(pr_u38_1472_cpu)==sizeof(pr_u9e_0e52_cpu),"append CPU layout");
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word_at(const uint8_t *ram,size_t p) { return (uint16_t)(ram[p]|((uint16_t)ram[(p+1u)&MASK]<<8)); }
static uint16_t read16(const uint8_t *ram,uint16_t s,uint16_t o) { return word_at(ram,physical(s,o)); }
static void push(uint8_t *ram,pr_u38_1472_cpu *c,uint16_t v) {
    size_t p;c->sp=(uint16_t)(c->sp-2u);p=physical(c->ss,c->sp);ram[p]=(uint8_t)v;ram[(p+1u)&MASK]=(uint8_t)(v>>8);
}
static void al(pr_u38_1472_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&0xFF00u)|v); }
pr_u38_1472_status pr_u38_sidecar_1472_cpu(uint8_t *ram,size_t size,pr_u38_1472_cpu *c,
    unsigned max_heap_steps,size_t max_links,pr_u38_1472_diagnostic *d)
{
    pr_u38_1472_diagnostic local={-1,-1};pr_u9e_0e52_cpu inner;uint16_t caller,stub;size_t frame;
    if(d==NULL)d=&local;
    *d=local;
    if(ram==NULL || c==NULL || size!=PR_U38_1472_RAM_SIZE || c->ip!=0x1472u || c->sp<0x40u || c->sp>0xFFF2u)
        return PR_U38_1472_ARGUMENT;
    caller=c->cs;push(ram,c,c->bp);c->bp=c->sp;             /* +1472 PUSH BP; MOV BP,SP */
    push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp+0xEu)));
    push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp+0xCu)));
    al(c,0x62);push(ram,c,c->ax);
    c->ax=0;c->flags=(uint16_t)((c->flags&~0x08D5u)|0x44u); /* +147E XOR AX,AX */
    push(ram,c,c->ax);al(c,0xFF);push(ram,c,c->ax);al(c,0);push(ram,c,c->ax);
    c->ip=0x1487u;
    if(ram[physical(caller,0x1487)]!=0x9Au || read16(ram,caller,0x1488)!=0x52u)return PR_U38_1472_CODE;
    stub=read16(ram,caller,0x148A);
    if(ram[physical(stub,0x52)]!=0xEAu || read16(ram,stub,0x53)!=0x0E52u)return PR_U38_1472_CODE;
    push(ram,c,caller);push(ram,c,0x148C);c->cs=read16(ram,stub,0x55);c->ip=0x0E52;
    memcpy(&inner,c,sizeof inner);
    d->append_status=(int)pr_u9e_list_append_0e52_cpu(ram,size,&inner,max_heap_steps,max_links,&d->getmem_status);
    memcpy(c,&inner,sizeof inner);
    if(d->append_status!=0)return PR_U38_1472_CALLEE;
    if(c->cs!=caller || c->ip!=0x148Cu)return PR_U38_1472_TRANSFER;
    c->bp=read16(ram,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u); /* +148C POP BP */
    frame=physical(c->ss,c->sp);c->ip=word_at(ram,frame);c->cs=word_at(ram,(frame+2u)&MASK);
    c->sp=(uint16_t)(c->sp+14u);return PR_U38_1472_OK;        /* +148D RETF0Ah */
}
