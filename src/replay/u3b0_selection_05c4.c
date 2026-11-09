#include "poolrad/replay/u3b0_selection_05c4.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE-1u)
static size_t addr(uint16_t s,uint16_t o) { return (((size_t)s<<4u)+o)&MASK; }
static uint8_t rb(pr_bus *b,uint16_t s,uint16_t o) { return pr_bus_read8(b,addr(s,o)); }
static uint16_t rw(pr_bus *b,uint16_t s,uint16_t o) { size_t a=addr(s,o);return (uint16_t)(pr_bus_read8(b,a)|((uint16_t)pr_bus_read8(b,(a+1u)&MASK)<<8u)); }
static void wb(pr_bus *b,uint16_t s,uint16_t o,uint8_t v) { pr_bus_write8(b,addr(s,o),v); }
static void push(pr_bus *b,pr_r0899_cpu *c,uint16_t v) { size_t a;c->sp=(uint16_t)(c->sp-2u);a=addr(c->ss,c->sp);pr_bus_write8(b,a,(uint8_t)v);pr_bus_write8(b,(a+1u)&MASK,(uint8_t)(v>>8u)); }
static int ordinary(uint16_t s) { return s<=0x9000u || s>=0xb000u; }
static void arith(pr_r0899_cpu *c,unsigned a,unsigned b,int sub,int inc) {
    unsigned r=(sub?a-b:a+b)&0xffu,p=r,carry=c->flags&1u;
    p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u) | (inc?carry:(sub?a<b:a+b>255u)?1u:0u) |
        ((p&1u)==0u?4u:0u) | ((a^b^r)&0x10u) | (r==0u?0x40u:0u) | (r&0x80u) |
        ((((sub?(a^b):~(a^b))&(a^r))&0x80u)<<4u));
}
#define FIELDS(X) X(ax) X(bx) X(cx) X(dx) X(si) X(di) X(bp) X(sp) X(cs) X(ip) X(ds) X(es) X(ss) X(flags)
#define TO_DRAW(f) t.f=c->f;
#define FROM_DRAW(f) c->f=t.f;
pr_u3b0_selection_status pr_u3b0_selection_05c4(pr_bus *b,size_t n,pr_r0899_cpu *c,
    const pr_u3b0_01da_limits *l,pr_u3b0_selection_diagnostic *d) {
    uint8_t k,v,limit; uint16_t site,next,caller,sp,rel; pr_u3b0_01da_cpu t;
    if(!b || !b->ram || !b->ega || !c || !l || !d || n!=PR_BUS_RAM_SIZE || c->ip!=0x05c4u ||
       c->bp<0x008eu || c->sp<0x0048u || c->sp>0xfffcu ||
       !ordinary(c->ds) || !ordinary(c->ss) || !ordinary(c->cs)) return PR_U3B0_SELECTION_ARGUMENT;
    memset(d,0,sizeof *d);k=(uint8_t)c->ax;arith(c,k,0x2cu,1,0);
    if(k==0x2cu) {
        v=rb(b,c->ds,0x6b2bu);arith(c,v,1u,1,0);
        if(v==1u) { v=rb(b,c->ss,(uint16_t)(c->bp-0x8eu));c->ax=(uint16_t)((c->ax&0xff00u)|v);wb(b,c->ds,0x6b2bu,v); }
        else { arith(c,v,1u,1,1);wb(b,c->ds,0x6b2bu,(uint8_t)(v-1u)); }
        site=0x05e2u;next=0x05e5u;
    } else {
        arith(c,k,0x2eu,1,0);
        if(k!=0x2eu) { c->ip=0x060cu;return PR_U3B0_SELECTION_OTHER; }
        v=rb(b,c->ds,0x6b2bu);c->ax=(uint16_t)((c->ax&0xff00u)|v);
        limit=rb(b,c->ss,(uint16_t)(c->bp-0x8eu));arith(c,v,limit,1,0);
        if(v==limit) wb(b,c->ds,0x6b2bu,1u);
        else { v=rb(b,c->ds,0x6b2bu);arith(c,v,1u,0,1);wb(b,c->ds,0x6b2bu,(uint8_t)(v+1u)); }
        site=0x0606u;next=0x0609u;
    }
    c->ax=(uint16_t)((c->ax&0xff00u)|rb(b,c->ds,0x6b2bu));
    caller=c->cs;sp=c->sp;push(b,c,c->ax);push(b,c,c->bp);push(b,c,c->cs);
    c->ip=site;d->call_site=site;
    rel=rw(b,c->cs,(uint16_t)(site+1u));
    if(rb(b,c->cs,site)!=0xe8u || (uint16_t)(next+rel)!=0x01dau) return PR_U3B0_SELECTION_CODE;
    push(b,c,next);c->ip=0x01dau;
    FIELDS(TO_DRAW)
    d->status=pr_u3b0_entry_01da_cpu(b,n,&t,l,&d->drawer);
    FIELDS(FROM_DRAW)
    if(d->status!=PR_U3B0_01DA_OK) return PR_U3B0_SELECTION_CALLEE;
    if(c->cs!=caller || c->ip!=next || c->sp!=sp) return PR_U3B0_SELECTION_TRANSFER;
    c->ip=0x06cfu;return PR_U3B0_SELECTION_DRAWN;
}
