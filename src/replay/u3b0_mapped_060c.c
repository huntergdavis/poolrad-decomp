#include "poolrad/replay/u3b0_mapped_060c.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE-1u)
static size_t a(uint16_t s,uint16_t o) { return (((size_t)s<<4u)+o)&MASK; }
static uint8_t rb(pr_bus *b,uint16_t s,uint16_t o) { return pr_bus_read8(b,a(s,o)); }
static uint16_t rw(pr_bus *b,uint16_t s,uint16_t o) { size_t p=a(s,o);return (uint16_t)(pr_bus_read8(b,p)|((uint16_t)pr_bus_read8(b,(p+1u)&MASK)<<8u)); }
static void wb(pr_bus *b,uint16_t s,uint16_t o,uint8_t v) { pr_bus_write8(b,a(s,o),v); }
static void push(pr_bus *b,pr_r0899_cpu *c,uint16_t v) { size_t p;c->sp=(uint16_t)(c->sp-2u);p=a(c->ss,c->sp);pr_bus_write8(b,p,(uint8_t)v);pr_bus_write8(b,(p+1u)&MASK,(uint8_t)(v>>8u)); }
static int ordinary(uint16_t s) { return s<=0x9000u || s>=0xb000u; }
static void cmp8(pr_r0899_cpu *c,unsigned x,unsigned y) {
    unsigned r=(x-y)&255u,p=r;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags & (uint16_t)~0x08d5u)|(x<y?1u:0u)|((p&1u)==0u?4u:0u)|
        ((x^y^r)&0x10u)|(r==0u?0x40u:0u)|(r&0x80u)|(((x^y)&(x^r)&0x80u)<<4u));
}
static void xor_ah(pr_r0899_cpu *c) { c->ax&=0x00ffu;c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|0x44u); }
static void sub31(pr_r0899_cpu *c) {
    unsigned x=c->ax,y=0x31u,r=(x-y)&0xffffu,p=r&255u;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|(x<y?1u:0u)|((p&1u)==0u?4u:0u)|
        ((x^y^r)&0x10u)|(r==0u?0x40u:0u)|(r&0x8000u?0x80u:0u)|(((x^y)&(x^r)&0x8000u)?0x800u:0u));
    c->ax=(uint16_t)r;
}
#define FIELDS(X) X(ax) X(bx) X(cx) X(dx) X(si) X(di) X(bp) X(sp) X(cs) X(ip) X(ds) X(es) X(ss) X(flags)
#define TO_LEAF(f) t.f=c->f;
#define FROM_LEAF(f) c->f=t.f;
static pr_u3b0_mapped_status leaf(pr_bus *b,size_t n,pr_r0899_cpu *c,const pr_u3b0_mapped_env *e,
    pr_u3b0_mapped_diagnostic *d,uint16_t site,uint16_t target,uint16_t next,uint16_t argbytes) {
    pr_system_leaf_cpu t;uint16_t caller=c->cs,sp=c->sp;
    c->ip=site;d->site=site;
    if(rb(b,c->cs,site)!=0x9au || rw(b,c->cs,(uint16_t)(site+1u))!=target ||
       rw(b,c->cs,(uint16_t)(site+3u))!=e->system_segment) return PR_U3B0_MAPPED_CODE;
    push(b,c,c->cs);push(b,c,next);c->cs=e->system_segment;c->ip=target;
    FIELDS(TO_LEAF)
    d->status=target==0x182au?pr_system_upcase_182a(b->ram,n,&t):pr_system_set_in_0dc0(b->ram,n,&t);
    FIELDS(FROM_LEAF)
    if(d->status!=PR_SYSTEM_LEAF_RETURNED) return PR_U3B0_MAPPED_LEAF;
    if(c->cs!=caller || c->ip!=next || c->sp!=(uint16_t)(sp+argbytes)) return PR_U3B0_MAPPED_TRANSFER;
    return PR_U3B0_MAPPED_BOUNDARY;
}
pr_u3b0_mapped_status pr_u3b0_mapped_060c(pr_bus *b,size_t n,pr_r0899_cpu *c,
    const pr_u3b0_mapped_env *e,pr_u3b0_mapped_diagnostic *d) {
    pr_u3b0_mapped_status s;uint8_t v;uint16_t off,seg;
    if(!b || !b->ram || !b->ega || !c || !e || !d || n!=PR_BUS_RAM_SIZE || c->ip!=0x060cu ||
       c->bp<0x0090u || c->bp>0xfff3u || c->sp<0x000au || c->sp>0xfff4u ||
       !ordinary(c->cs) || !ordinary(c->ds) || !ordinary(c->ss) || !ordinary(e->system_segment)) return PR_U3B0_MAPPED_ARGUMENT;
    memset(d,0,sizeof *d);
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-0x60u)));
    push(b,c,c->ax);s=leaf(b,n,c,e,d,0x0610u,0x182au,0x0615u,2u);if(s!=PR_U3B0_MAPPED_BOUNDARY)return s;
    wb(b,c->ss,(uint16_t)(c->bp-0x60u),(uint8_t)c->ax);
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-0x60u)));
    push(b,c,c->ax);c->di=0x037du;push(b,c,c->cs);push(b,c,c->di);
    s=leaf(b,n,c,e,d,0x0621u,0x0dc0u,0x0626u,6u);if(s!=PR_U3B0_MAPPED_BOUNDARY)return s;
    if((c->flags&0x40u)==0u) {
        v=rb(b,c->ss,(uint16_t)(c->bp-0x60u));cmp8(c,v,0x20u);
        if(v!=0x20u) { c->ip=0x0634u;return PR_U3B0_MAPPED_TEXT_SEARCH; }
        wb(b,c->ss,(uint16_t)(c->bp-0x63u),1u);
    }
    v=rb(b,c->ss,(uint16_t)(c->bp+0x0cu));cmp8(c,v,0u);
    if(v==0u) { c->ip=0x06cfu;return PR_U3B0_MAPPED_BOUNDARY; }
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ss,(uint16_t)(c->bp-0x60u)));
    push(b,c,c->ax);c->di=0x039du;push(b,c,c->cs);push(b,c,c->di);
    s=leaf(b,n,c,e,d,0x06a0u,0x0dc0u,0x06a5u,6u);if(s!=PR_U3B0_MAPPED_BOUNDARY)return s;
    if(c->flags&0x40u) { c->ip=0x06cfu;return PR_U3B0_MAPPED_BOUNDARY; }
    v=rb(b,c->ss,(uint16_t)(c->bp-0x60u));cmp8(c,v,0x5cu);
    if(v==0x5cu) wb(b,c->ss,(uint16_t)(c->bp-0x60u),0x37u);
    else {
        c->ax=(uint16_t)((c->ax&0xff00u)|v);xor_ah(c);sub31(c);c->di=c->ax;
        c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ds,(uint16_t)(c->di+0x288cu)));
        wb(b,c->ss,(uint16_t)(c->bp-0x60u),(uint8_t)c->ax);
    }
    off=rw(b,c->ss,(uint16_t)(c->bp+6u));seg=rw(b,c->ss,(uint16_t)(c->bp+8u));c->di=off;c->es=seg;
    wb(b,c->es,c->di,1u);wb(b,c->ss,(uint16_t)(c->bp-0x63u),1u);c->ip=0x06cfu;
    return PR_U3B0_MAPPED_BOUNDARY;
}
