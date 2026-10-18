/* Complete U38 +2814; docs/re/U38_LOOKUP_APPEND_2814_CPU.md.
 * Raw composition follows merged U38 +1472 and U86 +044E. */
#include "poolrad/recovered/overlay_u38_lookup_append_2814_cpu.h"
#include "poolrad/recovered/overlay_u9e_list_append_0e52_cpu.h"
#include "poolrad/recovered/resident_00ba_sidecar_lookup_2199_cpu.h"
#include <string.h>
#define MASK (PR_U38_2814_RAM_SIZE-1u)
_Static_assert(sizeof(pr_u38_2814_cpu)==sizeof(pr_u9e_0e52_cpu),"append CPU layout");
_Static_assert(sizeof(pr_u38_2814_cpu)==sizeof(pr_res_2199_cpu),"lookup CPU layout");
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word_at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&MASK]<<8)); }
static uint16_t read16(const uint8_t *r,uint16_t s,uint16_t o) { return word_at(r,physical(s,o)); }
static void push(uint8_t *r,pr_u38_2814_cpu *c,uint16_t v) {
    size_t p;c->sp=(uint16_t)(c->sp-2u);p=physical(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1u)&MASK]=(uint8_t)(v>>8);
}
static void al(pr_u38_2814_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&0xFF00u)|v); }
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
static void sub4(pr_u38_2814_cpu *c) {
    uint16_t a=c->sp,r=(uint16_t)(a-4u);c->flags&=(uint16_t)~0x08D5u;
    if(a<4)c->flags|=1;
    if(parity((uint8_t)r))c->flags|=4;
    if((a^4u^r)&0x10u)c->flags|=0x10;
    if(r==0)c->flags|=0x40;
    if(r&0x8000)c->flags|=0x80;
    if((a^4u)&(a^r)&0x8000u)c->flags|=0x800;
    c->sp=r;
}
static void or_al(pr_u38_2814_cpu *c) {
    uint8_t v=(uint8_t)c->ax;c->flags&=(uint16_t)~0x08D5u;
    if(parity(v))c->flags|=4;
    if(v==0)c->flags|=0x40;
    if(v&0x80)c->flags|=0x80;
}
static pr_u38_2814_status call(uint8_t *ram,size_t size,pr_u38_2814_cpu *c,
    const pr_u38_2814_limits *l,pr_u38_2814_diagnostic *d,unsigned append)
{
    uint16_t caller=c->cs,site=c->ip,offset=append?0x52:0x2199,segment;
    if(ram[physical(caller,site)]!=0x9A || read16(ram,caller,(uint16_t)(site+1))!=offset)return PR_U38_2814_CODE;
    segment=read16(ram,caller,(uint16_t)(site+3));
    if(append) {
        if(ram[physical(segment,offset)]!=0xEA || read16(ram,segment,(uint16_t)(offset+1))!=0xE52)return PR_U38_2814_CODE;
        segment=read16(ram,segment,(uint16_t)(offset+3));offset=0xE52;
    }
    push(ram,c,caller);push(ram,c,(uint16_t)(site+5));c->cs=segment;c->ip=offset;
    d->callee=(int)append;d->getmem_status=-1;
    if(append) {
        pr_u9e_0e52_cpu inner;memcpy(&inner,c,sizeof inner);
        d->status=(int)pr_u9e_list_append_0e52_cpu(ram,size,&inner,l->max_heap_steps,l->max_links,&d->getmem_status);memcpy(c,&inner,sizeof inner);
    } else {
        pr_res_2199_cpu inner;memcpy(&inner,c,sizeof inner);
        d->status=(int)pr_res_00ba_2199_cpu(ram,size,&inner,l->max_lookup_loopbacks);memcpy(c,&inner,sizeof inner);
    }
    if(d->status!=0)return PR_U38_2814_CALLEE;
    if(c->cs!=caller || c->ip!=(uint16_t)(site+5))return PR_U38_2814_TRANSFER;
    return PR_U38_2814_OK;
}
pr_u38_2814_status pr_u38_lookup_append_2814_cpu(uint8_t *ram,size_t size,pr_u38_2814_cpu *c,
    const pr_u38_2814_limits *l,pr_u38_2814_diagnostic *d)
{
    pr_u38_2814_diagnostic local={-1,-1,-1};pr_u38_2814_status status;size_t p;
    if(d==NULL)d=&local;
    *d=local;
    if(ram==NULL || c==NULL || l==NULL || size!=PR_U38_2814_RAM_SIZE || c->ip!=0x2814 || c->sp<0x40 || c->sp>0xFFF2)
        return PR_U38_2814_ARGUMENT;
    push(ram,c,c->bp);c->bp=c->sp;sub4(c);
    for(unsigned n=0;n<2;++n) {
        push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp+0xE)));push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp+0xC)));
        al(c,n==0?0x62:0x3B);push(ram,c,c->ax);c->di=(uint16_t)(c->bp-4);push(ram,c,c->ss);push(ram,c,c->di);
        c->ip=n==0?0x2828:0x283F;status=call(ram,size,c,l,d,0);
        if(status!=0)return status;
        or_al(c);
        if((uint8_t)c->ax!=0)goto done;
    }
    push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp+0xE)));push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp+0xC)));
    al(c,0x3B);push(ram,c,c->ax);c->ax=3;push(ram,c,c->ax);al(c,0xFF);push(ram,c,c->ax);al(c,1);push(ram,c,c->ax);
    c->ip=0x285B;status=call(ram,size,c,l,d,1);
    if(status!=0)return status;
done:
    c->sp=c->bp;c->bp=read16(ram,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2);
    p=physical(c->ss,c->sp);c->ip=word_at(ram,p);c->cs=word_at(ram,(p+2)&MASK);c->sp=(uint16_t)(c->sp+14);
    return PR_U38_2814_OK;
}
