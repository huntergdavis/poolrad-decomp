/* Complete raw U1C RETURN; see U1C_RETURN_0A9F_CPU.md. */
#include "poolrad/recovered/overlay_u1c_return_0a9f_cpu.h"
#define MASK ((1u<<20)-1u)
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&MASK]<<8)); }
static uint16_t rd(const uint8_t *r,uint16_t s,uint16_t o) { return at(r,physical(s,o)); }
static void wr(uint8_t *r,uint16_t s,uint16_t o,uint16_t v) { size_t p=physical(s,o);r[p]=(uint8_t)v;r[(p+1u)&MASK]=(uint8_t)(v>>8); }
static void push(uint8_t *r,pr_u1c_return_cpu *c,uint16_t v) { c->sp=(uint16_t)(c->sp-2u);wr(r,c->ss,c->sp,v); }
static void les_head(const uint8_t *r,pr_u1c_return_cpu *c) { size_t p=physical(c->ds,0x6F70);c->di=at(r,p);c->es=at(r,(p+2u)&MASK); }
static void logic(pr_u1c_return_cpu *c,uint16_t v) {
    uint8_t p=(uint8_t)v;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags&=(uint16_t)~0x8D5u;
    if(!(p&1u))c->flags|=4;
    if(!v)c->flags|=0x40;
    if(v&0x8000u)c->flags|=0x80;
}
pr_u1c_return_cpu_status pr_u1c_return_0a9f_cpu(uint8_t *r,size_t size,
    pr_u1c_return_cpu *c,unsigned max_heap_steps,int *heap_status)
{
    uint16_t caller,system;size_t p;int status;
    if(heap_status)*heap_status=-1;
    if(!r || !c || size!=(1u<<20) || c->ip!=0x0A9F || c->sp<0x2C || c->sp>0xFFFC)
        return PR_U1C_RETURN_CPU_ARGUMENT;
    push(r,c,c->bp);c->bp=c->sp;
    wr(r,c->ds,0x49ED,(uint16_t)(rd(r,c->ds,0x49ED)+1u));
    /* INC flags are overwritten by OR before any observable stop. */
    c->ax=rd(r,c->ds,0x6F70);c->ax|=rd(r,c->ds,0x6F72);logic(c,c->ax);
    if(c->ax) {
        les_head(r,c);c->ax=rd(r,c->es,c->di);wr(r,c->ds,0x49ED,c->ax);
        les_head(r,c);c->ax=rd(r,c->es,(uint16_t)(c->di+2u));c->dx=rd(r,c->es,(uint16_t)(c->di+4u));
        wr(r,c->ds,0x6F74,c->ax);wr(r,c->ds,0x6F76,c->dx);
        push(r,c,rd(r,c->ds,0x6F72));push(r,c,rd(r,c->ds,0x6F70));c->ax=6;push(r,c,c->ax);
        c->ip=0x0AD8;caller=c->cs;
        if(r[physical(caller,c->ip)]!=0x9A || rd(r,caller,0x0AD9)!=0x254)return PR_U1C_RETURN_CPU_CODE;
        system=rd(r,caller,0x0ADB);push(r,c,caller);push(r,c,0x0ADD);c->cs=system;c->ip=0x254;
        status=(int)pr_system_freemem_0254(r,size,c,max_heap_steps);if(heap_status)*heap_status=status;
        if(status!=PR_SYSTEM_FREEMEM_RETURNED)return PR_U1C_RETURN_CPU_CALLEE;
        if(c->cs!=caller || c->ip!=0x0ADD)return PR_U1C_RETURN_CPU_TRANSFER;
        c->ax=rd(r,c->ds,0x6F74);c->dx=rd(r,c->ds,0x6F76);
        wr(r,c->ds,0x6F70,c->ax);wr(r,c->ds,0x6F72,c->dx);
    }
    c->bp=rd(r,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    p=physical(c->ss,c->sp);c->ip=at(r,p);c->cs=at(r,(p+2u)&MASK);c->sp=(uint16_t)(c->sp+4u);
    return PR_U1C_RETURN_CPU_RETURNED;
}
