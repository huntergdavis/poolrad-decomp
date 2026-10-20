/* U3CE [014E,01B0): release two indexed bitmap pointers through raw resident2595. */
#include "poolrad/recovered/overlay_u3ce_bitmap_pair_release_014e_cpu.h"
#define MASK (PR_SYSTEM_FREEMEM_RAM_SIZE-1u)
typedef struct { uint8_t *r; pr_u3ce_014e_cpu *c; } machine;
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static uint16_t rd(machine *m,uint16_t s,uint16_t o) { return word(m->r,physical(s,o)); }
static void wr(machine *m,uint16_t s,uint16_t o,uint16_t v) {
    size_t p=physical(s,o);m->r[p]=(uint8_t)v;m->r[(p+1)&MASK]=(uint8_t)(v>>8);
}
static void push(machine *m,uint16_t v) { m->c->sp=(uint16_t)(m->c->sp-2u);wr(m,m->c->ss,m->c->sp,v); }
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
static void szp(pr_u3ce_014e_cpu *c,uint16_t r,uint16_t sign) {
    if(parity((uint8_t)r))c->flags|=4;
    if(r==0)c->flags|=0x40;
    if(r&sign)c->flags|=0x80;
}
static void sub(pr_u3ce_014e_cpu *c,uint16_t a,uint16_t b,uint16_t sign) {
    uint16_t r=(uint16_t)((a-b)&(uint16_t)((sign<<1)-1u));c->flags&=(uint16_t)~0x8D5u;
    if(a<b)c->flags|=1;
    if((a^b^r)&0x10)c->flags|=0x10;
    if((a^b)&(a^r)&sign)c->flags|=0x800;
    szp(c,r,sign);
}
static uint16_t add(pr_u3ce_014e_cpu *c,uint16_t a,uint16_t b) {
    uint16_t r=(uint16_t)(a+b);c->flags&=(uint16_t)~0x8D5u;
    if((uint32_t)a+b>65535)c->flags|=1;
    if((a^b^r)&0x10)c->flags|=0x10;
    if((uint16_t)~(a^b)&(a^r)&0x8000)c->flags|=0x800;
    szp(c,r,0x8000);return r;
}
static void logic(pr_u3ce_014e_cpu *c,uint16_t v) { c->flags&=(uint16_t)~0x8D5u;szp(c,v,0x8000); }
static void index_di(machine *m) {
    pr_u3ce_014e_cpu *c=m->c;c->ax=m->r[physical(c->ss,(uint16_t)(c->bp+6))];
    c->di=c->ax;c->cx=(uint16_t)((c->cx&0xFF00u)|3u);c->di=(uint16_t)(c->di<<3);
    /* XOR/SHL flags are overwritten by OR or ADD before any visible stop. */
}
static pr_u3ce_014e_status release(machine *m,uint16_t base,uint16_t site,
    uint16_t resident,uint16_t system,unsigned cap,pr_u3ce_014e_diagnostic *d) {
    pr_u3ce_014e_cpu *c=m->c;uint16_t caller=c->cs,next=(uint16_t)(site+5u);
    index_di(m);c->di=add(c,c->di,base);push(m,c->ds);push(m,c->di);
    ++d->calls;d->call_site=site;push(m,caller);push(m,next);c->cs=resident;c->ip=0x2595;
    d->resident_status=(int)pr_r0464_bitmap_release_2595_cpu(m->r,PR_SYSTEM_FREEMEM_RAM_SIZE,c,system,cap,&d->resident);
    if(d->resident_status!=PR_R0464_2595_RETURNED)return PR_U3CE_014E_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U3CE_014E_TRANSFER;
    return PR_U3CE_014E_RETURNED;
}
pr_u3ce_014e_status pr_u3ce_bitmap_pair_release_014e_cpu(uint8_t *ram,size_t size,
    pr_u3ce_014e_cpu *c,uint16_t resident,uint16_t system,unsigned cap,pr_u3ce_014e_diagnostic *d) {
    machine m={ram,c};pr_u3ce_014e_diagnostic local={-1,{-1,0,0},0,0};
    const uint16_t sites[]={0x17A,0x1A5};pr_u3ce_014e_status status;size_t p;
    if(!ram || !c || size!=PR_SYSTEM_FREEMEM_RAM_SIZE || c->ip!=0x14E || c->sp<0x80 || c->sp>0xFFFA)
        return PR_U3CE_014E_ARGUMENT;
    for(unsigned n=0;n<2;++n)
        if(ram[physical(c->cs,sites[n])]!=0x9A || rd(&m,c->cs,(uint16_t)(sites[n]+1))!=0x2595 ||
           rd(&m,c->cs,(uint16_t)(sites[n]+3))!=resident)return PR_U3CE_014E_ARGUMENT;
    if(!d)d=&local;
    *d=local;push(&m,c->bp);c->bp=c->sp;sub(c,c->sp,2,0x8000);c->sp=(uint16_t)(c->sp-2);
    index_di(&m);c->ax=rd(&m,c->ds,(uint16_t)(c->di+0x5E5A));
    c->ax|=rd(&m,c->ds,(uint16_t)(c->di+0x5E5C));logic(c,c->ax);
    if(!(c->flags&0x40)) {
        status=release(&m,0x5E5A,0x17A,resident,system,cap,d);
        if(status!=PR_U3CE_014E_RETURNED)return status;
    }
    index_di(&m);c->ax=rd(&m,c->ds,(uint16_t)(c->di+0x5E5E));
    c->ax|=rd(&m,c->ds,(uint16_t)(c->di+0x5E60));logic(c,c->ax);
    if(!(c->flags&0x40)) {
        status=release(&m,0x5E5E,0x1A5,resident,system,cap,d);
        if(status!=PR_U3CE_014E_RETURNED)return status;
    }
    c->sp=c->bp;c->bp=rd(&m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2);
    p=physical(c->ss,c->sp);c->ip=word(ram,p);c->cs=word(ram,(p+2)&MASK);c->sp=(uint16_t)(c->sp+6);
    return PR_U3CE_014E_RETURNED;
}
