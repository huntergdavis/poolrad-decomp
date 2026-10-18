/* U9E +0026 removal / FreeMem, with an explicit dynamic-handler boundary.
 * docs/re/U9E_LIST_RELEASE_0026_CPU.md. No strict full-chain claim. */
#include "poolrad/recovered/overlay_u9e_list_release_0026_cpu.h"
#include "poolrad/recovered/system_freemem_0254_cpu.h"
#include <string.h>
#define MASK (PR_U9E_0026_RAM_SIZE-1u)
_Static_assert(sizeof(pr_u9e_0026_cpu)==sizeof(pr_system_freemem_cpu),"CPU layout");
typedef struct { uint8_t *r; pr_u9e_0026_cpu *c; } machine;
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word_at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static uint16_t rd(machine *m,uint16_t s,uint16_t o) { return word_at(m->r,physical(s,o)); }
static uint8_t byte_at(machine *m,uint16_t s,uint16_t o) { return m->r[physical(s,o)]; }
static void wr(machine *m,uint16_t s,uint16_t o,uint16_t v) {
    size_t p=physical(s,o);m->r[p]=(uint8_t)v;m->r[(p+1)&MASK]=(uint8_t)(v>>8);
}
static uint16_t local(machine *m,int d) { return (uint16_t)(m->c->bp+d); }
static uint16_t lr(machine *m,int d) { return rd(m,m->c->ss,local(m,d)); }
static uint8_t lb(machine *m,int d) { return byte_at(m,m->c->ss,local(m,d)); }
static void lw(machine *m,int d,uint16_t v) { wr(m,m->c->ss,local(m,d),v); }
static void push(machine *m,uint16_t v) { m->c->sp=(uint16_t)(m->c->sp-2u);wr(m,m->c->ss,m->c->sp,v); }
static void les(machine *m,int d) {
    size_t p=physical(m->c->ss,local(m,d));m->c->di=word_at(m->r,p);m->c->es=word_at(m->r,(p+2)&MASK);
}
static uint16_t er(machine *m,unsigned d) { return rd(m,m->c->es,(uint16_t)(m->c->di+d)); }
static void ew(machine *m,unsigned d,uint16_t v) { wr(m,m->c->es,(uint16_t)(m->c->di+d),v); }
static void al(machine *m,uint8_t v) { m->c->ax=(uint16_t)((m->c->ax&0xFF00u)|v); }
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
static void szp(pr_u9e_0026_cpu *c,uint16_t r,uint16_t sign) {
    if(parity((uint8_t)r))c->flags|=4;
    if(r==0)c->flags|=0x40;
    if(r&sign)c->flags|=0x80;
}
static void sub(pr_u9e_0026_cpu *c,uint16_t a,uint16_t b,uint16_t sign) {
    uint16_t r=(uint16_t)((a-b)&(uint16_t)((sign<<1)-1u));c->flags&=(uint16_t)~0x08D5u;
    if(a<b)c->flags|=1;
    if((a^b^r)&0x10)c->flags|=0x10;
    if((a^b)&(a^r)&sign)c->flags|=0x800;
    szp(c,r,sign);
}
static void logic(pr_u9e_0026_cpu *c,uint16_t r) { c->flags&=(uint16_t)~0x08D5u;szp(c,r,0x8000); }
static void or_local(machine *m,int d) { m->c->ax|=lr(m,d);logic(m->c,m->c->ax); }
static void shl_di(pr_u9e_0026_cpu *c) {
    uint16_t before=c->di;c->di=(uint16_t)(before<<1);logic(c,c->di);c->flags|=0x10; /* normal/386 SHL: undefined AF is set */
    if(before&0x8000)c->flags|=1;
    if((before^c->di)&0x8000)c->flags|=0x800;
}
/* This is the exact +0000 prefix, not an implementation of its handlers. */
static pr_u9e_0026_status dispatch(machine *m,pr_u9e_0026_diagnostic *d) {
    pr_u9e_0026_cpu *c=m->c;size_t p;
    c->ip=0xA1;
    if(byte_at(m,c->cs,0xA1)!=0xE8 || rd(m,c->cs,0xA2)!=0xFF5C)return PR_U9E_0026_CODE;
    push(m,0xA4);c->ip=0;
    push(m,c->bp);c->bp=c->sp;
    push(m,lr(m,14));push(m,lr(m,12));push(m,lr(m,10));push(m,lr(m,8));
    al(m,lb(m,6));push(m,c->ax);al(m,lb(m,16));c->ax&=0xFFu;
    logic(c,0);c->di=c->ax;shl_di(c);shl_di(c);c->ip=0x1E;
    p=physical(c->ds,(uint16_t)(c->di+0x6828));
    d->handler_offset=word_at(m->r,p);d->handler_segment=word_at(m->r,(p+2)&MASK);
    return PR_U9E_0026_PENDING;
}
pr_u9e_0026_status pr_u9e_list_release_0026_cpu(uint8_t *ram,size_t size,
    pr_u9e_0026_cpu *c,const pr_u9e_0026_limits *l,pr_u9e_0026_diagnostic *d)
{
    pr_u9e_0026_diagnostic fallback={-1,0,0};machine m={ram,c};unsigned loops=0;size_t p;
    uint16_t caller,segment;pr_system_freemem_cpu inner;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || c==NULL || l==NULL || size!=PR_U9E_0026_RAM_SIZE || c->ip!=0x26 || c->sp<0x40 || c->sp>0xFFF2)
        return PR_U9E_0026_ARGUMENT;
    push(&m,c->bp);c->bp=c->sp;sub(c,c->sp,8,0x8000);c->sp=(uint16_t)(c->sp-8);
    c->ax=lr(&m,6);c->dx=lr(&m,8);lw(&m,-8,c->ax);lw(&m,-6,c->dx);
    c->ax=lr(&m,-8);or_local(&m,-6);
    if(!(c->flags&0x40))goto chosen;
    les(&m,12);c->ax=er(&m,0x7F);c->dx=er(&m,0x81);lw(&m,-8,c->ax);lw(&m,-6,c->dx);
search:
    c->ax=lr(&m,-8);or_local(&m,-6);
    if(c->flags&0x40)goto chosen;
    les(&m,-8);al(&m,byte_at(&m,c->es,c->di));sub(c,(uint8_t)c->ax,lb(&m,10),0x80);
    if(c->flags&0x40)goto chosen;
    les(&m,-8);c->ax=er(&m,5);c->dx=er(&m,7);lw(&m,-8,c->ax);lw(&m,-6,c->dx);
    c->ip=0x52;if(loops==l->max_list_loopbacks)return PR_U9E_0026_LIMIT;++loops;goto search;
chosen:
    c->ax=lr(&m,-8);or_local(&m,-6);
    if(c->flags&0x40)goto done;
    les(&m,-8);sub(c,byte_at(&m,c->es,(uint16_t)(c->di+4)),0,0x80);
    if(!(c->flags&0x40)) {
        al(&m,lb(&m,10));push(&m,c->ax);push(&m,lr(&m,14));push(&m,lr(&m,12));
        push(&m,lr(&m,-6));push(&m,lr(&m,-8));al(&m,1);push(&m,c->ax);push(&m,c->cs);
        return dispatch(&m,d);
    }
    les(&m,12);c->ax=er(&m,0x7F);c->dx=er(&m,0x81);sub(c,c->dx,lr(&m,-6),0x8000);
    if(!(c->flags&0x40))goto predecessor;
    sub(c,c->ax,lr(&m,-8),0x8000);if(!(c->flags&0x40))goto predecessor;
    les(&m,-8);c->ax=er(&m,5);c->dx=er(&m,7);les(&m,12);ew(&m,0x7F,c->ax);ew(&m,0x81,c->dx);goto release;
predecessor:
    les(&m,12);c->ax=er(&m,0x7F);c->dx=er(&m,0x81);lw(&m,-4,c->ax);lw(&m,-2,c->dx);
walk:
    c->ax=lr(&m,-4);or_local(&m,-2);if(c->flags&0x40)goto splice;
    les(&m,-4);c->ax=er(&m,5);c->dx=er(&m,7);sub(c,c->dx,lr(&m,-6),0x8000);
    if(!(c->flags&0x40))goto advance;
    sub(c,c->ax,lr(&m,-8),0x8000);if(c->flags&0x40)goto splice;
advance:
    les(&m,-4);c->ax=er(&m,5);c->dx=er(&m,7);lw(&m,-4,c->ax);lw(&m,-2,c->dx);
    c->ip=0xE5;if(loops==l->max_list_loopbacks)return PR_U9E_0026_LIMIT;++loops;goto walk;
splice:
    les(&m,-8);c->ax=er(&m,5);c->dx=er(&m,7);les(&m,-4);ew(&m,5,c->ax);ew(&m,7,c->dx);
release:
    push(&m,lr(&m,-6));push(&m,lr(&m,-8));c->ax=9;push(&m,c->ax);c->ip=0x135;
    if(byte_at(&m,c->cs,c->ip)!=0x9A || rd(&m,c->cs,0x136)!=0x254)return PR_U9E_0026_CODE;
    caller=c->cs;segment=rd(&m,c->cs,0x138);push(&m,caller);push(&m,0x13A);c->cs=segment;c->ip=0x254;
    memcpy(&inner,c,sizeof inner);d->freemem_status=(int)pr_system_freemem_0254(ram,size,&inner,l->max_heap_loopbacks);memcpy(c,&inner,sizeof inner);
    if(d->freemem_status!=0)return PR_U9E_0026_CALLEE;
    if(c->cs!=caller || c->ip!=0x13A)return PR_U9E_0026_TRANSFER;
done:
    c->sp=c->bp;c->bp=rd(&m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2);
    p=physical(c->ss,c->sp);c->ip=word_at(ram,p);c->cs=word_at(ram,(p+2)&MASK);c->sp=(uint16_t)(c->sp+14);
    return PR_U9E_0026_OK;
}
