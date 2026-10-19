/* Complete U3B0 entry7; see U3B0_DISPOSE_11F3_CPU.md. */
#include "poolrad/recovered/overlay_u3b0_dispose_11f3_cpu.h"
#define MASK ((1u<<20)-1u)
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&MASK]<<8)); }
static uint16_t rd(const uint8_t *r,uint16_t s,uint16_t o) { return at(r,physical(s,o)); }
static void wr(uint8_t *r,uint16_t s,uint16_t o,uint16_t v) { size_t p=physical(s,o);r[p]=(uint8_t)v;r[(p+1u)&MASK]=(uint8_t)(v>>8); }
static void push(uint8_t *r,pr_u3b0_11f3_cpu *c,uint16_t v) { c->sp=(uint16_t)(c->sp-2u);wr(r,c->ss,c->sp,v); }
static uint16_t local(const pr_u3b0_11f3_cpu *c,int d) { return (uint16_t)(c->bp+d); }
static void les(const uint8_t *r,pr_u3b0_11f3_cpu *c,uint16_t s,uint16_t o) { size_t p=physical(s,o);c->di=at(r,p);c->es=at(r,(p+2u)&MASK); }
static void logic(pr_u3b0_11f3_cpu *c,uint16_t v) {
    uint8_t p=(uint8_t)v;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags&=(uint16_t)~0x8D5u;
    if(!(p&1u))c->flags|=4;
    if(!v)c->flags|=0x40;
    if(v&0x8000u)c->flags|=0x80;
}
pr_u3b0_11f3_status pr_u3b0_dispose_11f3_cpu(uint8_t *r,size_t size,
    pr_u3b0_11f3_cpu *c,unsigned max_loopbacks,unsigned max_heap_steps,int *heap_status)
{
    unsigned loops=0;uint16_t caller,system;size_t p;int status;
    if(heap_status)*heap_status=-1;
    if(!r || !c || size!=(1u<<20) || c->ip!=0x11F3 || c->sp<0x34 || c->sp>0xFFF8)
        return PR_U3B0_11F3_ARGUMENT;
    push(r,c,c->bp);c->bp=c->sp;c->sp=(uint16_t)(c->sp-8u);
    /* SUB flags are overwritten by OR before any observable stop. */
    les(r,c,c->ss,local(c,6));c->ax=rd(r,c->es,c->di);c->dx=rd(r,c->es,(uint16_t)(c->di+2u));
    wr(r,c->ss,local(c,-4),c->ax);wr(r,c->ss,local(c,-2),c->dx);
    for(;;) {
        c->ax=rd(r,c->ss,local(c,-4));c->ax|=rd(r,c->ss,local(c,-2));logic(c,c->ax);
        if(!c->ax)break;
        c->ax=rd(r,c->ss,local(c,-4));c->dx=rd(r,c->ss,local(c,-2));
        wr(r,c->ss,local(c,-8),c->ax);wr(r,c->ss,local(c,-6),c->dx);
        les(r,c,c->ss,local(c,-4));c->ax=rd(r,c->es,(uint16_t)(c->di+0x2Au));c->dx=rd(r,c->es,(uint16_t)(c->di+0x2Cu));
        wr(r,c->ss,local(c,-4),c->ax);wr(r,c->ss,local(c,-2),c->dx);
        push(r,c,rd(r,c->ss,local(c,-6)));push(r,c,rd(r,c->ss,local(c,-8)));c->ax=0x2E;push(r,c,c->ax);
        c->ip=0x1238;caller=c->cs;
        if(r[physical(caller,c->ip)]!=0x9A || rd(r,caller,0x1239)!=0x254)return PR_U3B0_11F3_CODE;
        system=rd(r,caller,0x123B);push(r,c,caller);push(r,c,0x123D);c->cs=system;c->ip=0x254;
        status=(int)pr_system_freemem_0254(r,size,c,max_heap_steps);if(heap_status)*heap_status=status;
        if(status!=PR_SYSTEM_FREEMEM_RETURNED)return PR_U3B0_11F3_CALLEE;
        if(c->cs!=caller || c->ip!=0x123D)return PR_U3B0_11F3_TRANSFER;
        c->ip=0x1209;if(loops==max_loopbacks)return PR_U3B0_11F3_LIMIT;++loops;
    }
    c->sp=c->bp;c->bp=rd(r,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    p=physical(c->ss,c->sp);c->ip=at(r,p);c->cs=at(r,(p+2u)&MASK);c->sp=(uint16_t)(c->sp+8u);
    return PR_U3B0_11F3_RETURNED;
}
