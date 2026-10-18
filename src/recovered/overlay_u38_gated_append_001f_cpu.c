/* Complete U38 +001F; docs/re/U38_GATED_APPEND_001F_CPU.md.
 * Live-stub composition follows U38 +1472 (PR #1005), word semantics +2DE4. */
#include "poolrad/recovered/overlay_u38_gated_append_001f_cpu.h"
#include "poolrad/recovered/overlay_u9e_list_append_0e52_cpu.h"
#include <string.h>
#define MASK (PR_U38_001F_RAM_SIZE-1u)
_Static_assert(sizeof(pr_u38_001f_cpu)==sizeof(pr_u9e_0e52_cpu),"append CPU layout");
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word_at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&MASK]<<8)); }
static uint16_t read16(const uint8_t *r,uint16_t s,uint16_t o) { return word_at(r,physical(s,o)); }
static void push(uint8_t *r,pr_u38_001f_cpu *c,uint16_t v) {
    size_t p;c->sp=(uint16_t)(c->sp-2u);p=physical(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1u)&MASK]=(uint8_t)(v>>8);
}
static void al(pr_u38_001f_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&0xFF00u)|v); }
static uint16_t cmp_zero(uint16_t flags,uint8_t v) {
    unsigned parity=v;parity^=parity>>4;parity^=parity>>2;parity^=parity>>1;
    return (uint16_t)((flags&~0x08D5u)|(v&0x80u)|(v==0?0x40u:0u)|((parity&1u)==0?4u:0u));
}
pr_u38_001f_status pr_u38_gated_append_001f_cpu(uint8_t *ram,size_t size,pr_u38_001f_cpu *c,
    unsigned max_heap_steps,size_t max_links,pr_u38_001f_diagnostic *d)
{
    pr_u38_001f_diagnostic local={-1,-1};pr_u9e_0e52_cpu inner;uint16_t caller,stub;size_t frame;uint8_t gate;
    if(d==NULL)d=&local;
    *d=local;
    if(ram==NULL || c==NULL || size!=PR_U38_001F_RAM_SIZE || c->ip!=0x001Fu || c->sp<0x40u || c->sp>0xFFF2u)
        return PR_U38_001F_ARGUMENT;
    caller=c->cs;push(ram,c,c->bp);c->bp=c->sp;c->sp=(uint16_t)(c->sp-2u);
    /* SUB SP flags are overwritten by CMP before any observable stop. */
    gate=ram[physical(c->ds,0x681F)];c->flags=cmp_zero(c->flags,gate);
    ram[physical(c->ss,(uint16_t)(c->bp-1u))]=(uint8_t)(gate==0);
    if(gate==0) {
        push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp+0xEu)));
        push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp+0xCu)));
        al(c,ram[physical(c->ss,(uint16_t)(c->bp+0xAu))]);push(ram,c,c->ax);
        push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp+6u)));
        al(c,ram[physical(c->ss,(uint16_t)(c->bp+8u))]);push(ram,c,c->ax);
        al(c,1);push(ram,c,c->ax);c->ip=0x4A;
        if(ram[physical(caller,0x4A)]!=0x9A || read16(ram,caller,0x4B)!=0x52)return PR_U38_001F_CODE;
        stub=read16(ram,caller,0x4D);
        if(ram[physical(stub,0x52)]!=0xEA || read16(ram,stub,0x53)!=0xE52)return PR_U38_001F_CODE;
        push(ram,c,caller);push(ram,c,0x4F);c->cs=read16(ram,stub,0x55);c->ip=0xE52;
        memcpy(&inner,c,sizeof inner);
        d->append_status=(int)pr_u9e_list_append_0e52_cpu(ram,size,&inner,max_heap_steps,max_links,&d->getmem_status);
        memcpy(c,&inner,sizeof inner);
        if(d->append_status!=0)return PR_U38_001F_CALLEE;
        if(c->cs!=caller || c->ip!=0x4F)return PR_U38_001F_TRANSFER;
    }
    al(c,ram[physical(c->ss,(uint16_t)(c->bp-1u))]);c->sp=c->bp;
    c->bp=read16(ram,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    frame=physical(c->ss,c->sp);c->ip=word_at(ram,frame);c->cs=word_at(ram,(frame+2u)&MASK);
    c->sp=(uint16_t)(c->sp+14u);return PR_U38_001F_OK;
}
