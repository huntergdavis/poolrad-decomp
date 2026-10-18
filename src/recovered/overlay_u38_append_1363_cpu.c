/* Complete U38 +1363; docs/re/U38_APPEND_1363_CPU.md.
 * Ordered memory and stub binding semantics follow accepted U38 +2814. */
#include "poolrad/recovered/overlay_u38_append_1363_cpu.h"
#define MASK (PR_U38_1363_RAM_SIZE-1u)
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word_at(const uint8_t *r,size_t p) {
    return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&MASK]<<8));
}
static uint16_t read16(const uint8_t *r,uint16_t s,uint16_t o) {
    return word_at(r,physical(s,o));
}
static void push(uint8_t *r,pr_u38_1363_cpu *c,uint16_t v) {
    size_t p;c->sp=(uint16_t)(c->sp-2u);p=physical(c->ss,c->sp);
    r[p]=(uint8_t)v;r[(p+1u)&MASK]=(uint8_t)(v>>8);
}
static void al(pr_u38_1363_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&0xFF00u)|v); }
pr_u38_1363_status pr_u38_append_1363_cpu(uint8_t *ram,size_t size,
    pr_u38_1363_cpu *c,const pr_u38_1363_limits *l,pr_u38_1363_diagnostic *d)
{
    pr_u38_1363_diagnostic local={-1,-1};
    uint16_t caller,stub,segment;size_t p;
    if(d==NULL)d=&local;
    *d=local;
    if(ram==NULL || c==NULL || l==NULL || size!=PR_U38_1363_RAM_SIZE ||
       c->ip!=0x1363u || c->sp<0x0040u || c->sp>0xFFF2u)
        return PR_U38_1363_ARGUMENT;
    caller=c->cs;
    push(ram,c,c->bp);c->bp=c->sp;
    push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp+0xE)));
    push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp+0xC)));
    al(c,0x19);push(ram,c,c->ax);c->ax=1;push(ram,c,c->ax);
    al(c,0x0C);push(ram,c,c->ax);al(c,0);push(ram,c,c->ax);
    c->ip=0x1379;
    if(ram[physical(caller,0x1379)]!=0x9A || read16(ram,caller,0x137A)!=0x52)
        return PR_U38_1363_CODE;
    stub=read16(ram,caller,0x137C);
    if(ram[physical(stub,0x52)]!=0xEA || read16(ram,stub,0x53)!=0xE52)
        return PR_U38_1363_CODE;
    segment=read16(ram,stub,0x55);
    push(ram,c,caller);push(ram,c,0x137E);c->cs=segment;c->ip=0xE52;
    d->append_status=(int)pr_u9e_list_append_0e52_cpu(ram,size,c,l->max_heap_steps,l->max_links,&d->getmem_status);
    if(d->append_status!=0)return PR_U38_1363_CALLEE;
    if(c->cs!=caller || c->ip!=0x137E)return PR_U38_1363_TRANSFER;
    c->bp=read16(ram,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2);
    p=physical(c->ss,c->sp);c->ip=word_at(ram,p);c->cs=word_at(ram,(p+2)&MASK);
    c->sp=(uint16_t)(c->sp+14);
    return PR_U38_1363_OK;
}
