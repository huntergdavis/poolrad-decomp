#include "poolrad/recovered/overlay_u3ce_gate_014e.h"
static size_t phys(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static uint8_t rb(const uint8_t *m,uint16_t s,uint16_t o) { return m[phys(s,o)]; }
static uint16_t rw(const uint8_t *m,uint16_t s,uint16_t o) {
    size_t a=phys(s,o);return (uint16_t)(m[a]|((uint16_t)m[(a+1u)&0xfffffu]<<8));
}
static void ww(uint8_t *m,uint16_t s,uint16_t o,uint16_t v) {
    size_t a=phys(s,o);m[a]=(uint8_t)v;m[(a+1u)&0xfffffu]=(uint8_t)(v>>8);
}
static void far_read(const uint8_t *m,uint16_t s,uint16_t o,uint16_t *off,uint16_t *seg) {
    size_t a=phys(s,o);*off=(uint16_t)(m[a]|((uint16_t)m[(a+1u)&0xfffffu]<<8));
    *seg=(uint16_t)(m[(a+2u)&0xfffffu]|((uint16_t)m[(a+3u)&0xfffffu]<<8));
}
static uint16_t result_flags(uint16_t old,uint16_t r) {
    unsigned ones=0;uint16_t f=(uint16_t)(old&~0x08d5u);
    for(unsigned i=0;i<8u;++i)ones+=(r>>i)&1u;
    if(!(ones&1u))f|=4u;
    if(r==0u)f|=0x40u;
    if(r&0x8000u)f|=0x80u;
    return f;
}
static uint16_t arithmetic(uint16_t old,uint16_t a,uint16_t b,int sub) {
    uint32_t full=sub?(uint32_t)a-b:(uint32_t)a+b;uint16_t r=(uint16_t)full;
    uint16_t f=result_flags(old,r);
    if(sub?a<b:full>65535u)f|=1u;
    if((a^b^r)&0x10u)f|=0x10u;
    if(sub?((a^b)&(a^r)&0x8000u):((~(a^b))&(a^r)&0x8000u))f|=0x800u;
    return f;
}
static void push(uint8_t *m,pr_u3ce_014e_cpu *c,uint16_t v) {
    c->sp=(uint16_t)(c->sp-2u);ww(m,c->ss,c->sp,v);
}
static void index_slot(const uint8_t *m,pr_u3ce_014e_cpu *c) {
    c->ax=rb(m,c->ss,(uint16_t)(c->bp+6u));
    c->di=(uint16_t)(c->ax<<3);c->cx=(uint16_t)((c->cx&0xff00u)|3u);
    /* XOR/SHL arithmetic flags are overwritten by OR or ADD before a stop. */
}
static int nonzero(const uint8_t *m,pr_u3ce_014e_cpu *c,uint16_t base) {
    index_slot(m,c);
    c->ax=rw(m,c->ds,(uint16_t)(c->di+base));
    c->ax|=rw(m,c->ds,(uint16_t)(c->di+base+2u));
    c->flags=result_flags(c->flags,c->ax);return c->ax!=0u;
}
static pr_u3ce_014e_status callback(uint8_t *m,pr_u3ce_014e_cpu *c,
                                   pr_u3ce_014e_call *p,uint16_t base,uint16_t site) {
    uint16_t old;index_slot(m,c);old=c->di;c->di=(uint16_t)(old+base);
    c->flags=arithmetic(c->flags,old,base,0);push(m,c,c->ds);push(m,c,c->di);
    c->ip=site;p->call_ip=site;p->continuation_ip=(uint16_t)(site+5u);
    far_read(m,c->cs,(uint16_t)(site+1u),&p->target_ip,&p->target_cs);
    return PR_U3CE_014E_CALLBACK;
}
pr_u3ce_014e_status pr_u3ce_gate_014e(uint8_t *m,size_t size,
                                     pr_u3ce_014e_cpu *c,pr_u3ce_014e_call *p) {
    uint16_t old,ip,cs;
    if(!m || size<PR_U3CE_014E_RAM_SIZE || !c || !p ||
       (c->ip!=0x14eu && c->ip!=0x17fu))return PR_U3CE_014E_ARGUMENT;
    if(c->ip==0x14eu) {
        push(m,c,c->bp);c->bp=c->sp;old=c->sp;c->sp=(uint16_t)(old-2u);
        c->flags=arithmetic(c->flags,old,2u,1);
        if(nonzero(m,c,0x5e5au))return callback(m,c,p,0x5e5au,0x17au);
    }
    if(nonzero(m,c,0x5e5eu))return callback(m,c,p,0x5e5eu,0x1a5u);
    c->sp=c->bp;c->bp=rw(m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    ip=rw(m,c->ss,c->sp);
    cs=rw(m,c->ss,(uint16_t)(c->sp+2u));
    c->sp=(uint16_t)(c->sp+6u);c->cs=cs;c->ip=ip;
    return PR_U3CE_014E_RETURNED;
}
