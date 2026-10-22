/* RES_0464 [2595,2632), decoded from loader-expanded START.EXE.
 * Memory/FLAGS idioms reuse the raw U6C release and SYSTEM FreeMem models. */
#include "poolrad/recovered/resident_0464_bitmap_release_2595_cpu.h"
#define MASK (PR_SYSTEM_FREEMEM_RAM_SIZE-1u)
typedef struct { uint8_t *r; pr_r0464_2595_cpu *c; } machine;
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word(const uint8_t *r,size_t p) {
    return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8));
}
static uint16_t rd(machine *m,uint16_t s,uint16_t o) { return word(m->r,physical(s,o)); }
static void wr(machine *m,uint16_t s,uint16_t o,uint16_t v) {
    size_t p=physical(s,o);m->r[p]=(uint8_t)v;m->r[(p+1)&MASK]=(uint8_t)(v>>8);
}
static uint16_t lr(machine *m,int d) { return rd(m,m->c->ss,(uint16_t)(m->c->bp+d)); }
static void lw(machine *m,int d,uint16_t v) { wr(m,m->c->ss,(uint16_t)(m->c->bp+d),v); }
static void push(machine *m,uint16_t v) { m->c->sp=(uint16_t)(m->c->sp-2u);wr(m,m->c->ss,m->c->sp,v); }
static void les_at(machine *m,uint16_t s,uint16_t o) {
    size_t p=physical(s,o);m->c->di=word(m->r,p);m->c->es=word(m->r,(p+2)&MASK);
}
static void les(machine *m,int d) { les_at(m,m->c->ss,(uint16_t)(m->c->bp+d)); }
static uint16_t er(machine *m,unsigned d) { return rd(m,m->c->es,(uint16_t)(m->c->di+d)); }
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
static void szp(pr_r0464_2595_cpu *c,uint16_t r,uint16_t sign) {
    if(parity((uint8_t)r))c->flags|=4;
    if(r==0)c->flags|=0x40;
    if(r&sign)c->flags|=0x80;
}
static void logic(pr_r0464_2595_cpu *c,uint16_t r) { c->flags&=(uint16_t)~0x8D5u;szp(c,r,0x8000); }
static void sub(pr_r0464_2595_cpu *c,uint16_t a,uint16_t b,uint16_t sign) {
    uint16_t r=(uint16_t)((a-b)&(uint16_t)((sign<<1)-1u));c->flags&=(uint16_t)~0x8D5u;
    if(a<b)c->flags|=1;
    if((a^b^r)&0x10)c->flags|=0x10;
    if((a^b)&(a^r)&sign)c->flags|=0x800;
    szp(c,r,sign);
}
static void add_ax(pr_r0464_2595_cpu *c,uint16_t b) {
    uint16_t a=c->ax;c->ax=(uint16_t)(a+b);c->flags&=(uint16_t)~0x8D5u;
    if((uint32_t)a+b>65535)c->flags|=1;
    if((a^b^c->ax)&0x10)c->flags|=0x10;
    if((uint16_t)~(a^b)&(a^c->ax)&0x8000)c->flags|=0x800;
    szp(c,c->ax,0x8000);
}
static void shr_ax(pr_r0464_2595_cpu *c) {
    uint16_t before=c->ax;c->ax>>=1;logic(c,c->ax);c->flags|=0x10;
    if(before&1)c->flags|=1;
    if(before&0x8000)c->flags|=0x800;
}
static pr_r0464_2595_status release(machine *m,uint16_t site,uint16_t system,
    unsigned cap,pr_r0464_2595_diagnostic *d) {
    pr_r0464_2595_cpu *c=m->c;uint16_t caller=c->cs,next=(uint16_t)(site+5u);
    d->call_site=site;++d->calls;push(m,caller);push(m,next);c->cs=system;c->ip=0x254;
    d->freemem_status=(int)pr_system_freemem_0254(m->r,PR_SYSTEM_FREEMEM_RAM_SIZE,c,cap);
    if(d->freemem_status!=PR_SYSTEM_FREEMEM_RETURNED)return PR_R0464_2595_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_R0464_2595_TRANSFER;
    return PR_R0464_2595_RETURNED;
}
pr_r0464_2595_status pr_r0464_bitmap_release_2595_cpu(uint8_t *ram,size_t size,
    pr_r0464_2595_cpu *c,uint16_t system,unsigned cap,pr_r0464_2595_diagnostic *d) {
    machine m={ram,c};pr_r0464_2595_diagnostic local={-1,0,0};
    pr_r0464_2595_status status;size_t p;uint32_t product;
    const uint16_t sites[]={0x25F0,0x2605,0x261B};unsigned i;
    if(!ram || !c || size!=PR_SYSTEM_FREEMEM_RAM_SIZE || c->ip!=0x2595 ||
       c->sp<0x40 || c->sp>0xFFF8)return PR_R0464_2595_ARGUMENT;
    for(i=0;i<3;++i) {
        if(ram[physical(c->cs,sites[i])]!=0x9A || rd(&m,c->cs,(uint16_t)(sites[i]+1u))!=0x254 ||
           rd(&m,c->cs,(uint16_t)(sites[i]+3u))!=system)return PR_R0464_2595_ARGUMENT;
    }
    if(!d)d=&local;
    *d=local;
    push(&m,c->bp);c->bp=c->sp;sub(c,c->sp,6,0x8000);c->sp=(uint16_t)(c->sp-6); /* 2595..2598 */
    les(&m,6);c->ax=er(&m,0);c->ax|=er(&m,2);logic(c,c->ax); /* 259B..25A1 */
    if(c->flags&0x40)goto done; /* 25A5/25A7 */
    les(&m,6);les_at(&m,c->es,c->di);lw(&m,-6,c->di);lw(&m,-4,c->es); /* 25AA..25B3 */
    les(&m,-6);c->ax=(uint16_t)((c->ax&0xFF00u)|ram[physical(c->es,(uint16_t)(c->di+8u))]);
    c->ax&=0xFFu;logic(c,0);les(&m,-6); /* 25B6..25BF */
    product=(uint32_t)c->ax*er(&m,0x11);c->ax=(uint16_t)product;c->dx=(uint16_t)(product>>16);
    /* MUL's undefined arithmetic bits are overwritten by OR at25D0 before
     * any branch, call, or externally observable stop. CF/OF are exact. */
    c->flags&=(uint16_t)~0x801u;if(c->dx)c->flags|=0x801;
    lw(&m,-2,c->ax);les(&m,-6);c->ax=er(&m,0x13);c->ax|=er(&m,0x15);logic(c,c->ax); /* 25C6..25D0 */
    if(!(c->flags&0x40)) { /* 25D4 */
        sub(c,ram[physical(c->ds,0x4A1E)],1,0x80); /* 25D6 */
        if(c->flags&0x40) { /* 25DB */
            les(&m,-6);push(&m,er(&m,0x15));push(&m,er(&m,0x13));c->ax=lr(&m,-2);
            shr_ax(c);shr_ax(c);push(&m,c->ax); /* 25DD..25EF */
            status=release(&m,0x25F0,system,cap,d); /* 25F0/25F5 */
        } else {
            les(&m,-6);push(&m,er(&m,0x15));push(&m,er(&m,0x13));push(&m,lr(&m,-2));
            status=release(&m,0x2605,system,cap,d); /* 25F7..2605 */
        }
        if(status!=PR_R0464_2595_RETURNED)return status;
    }
    les(&m,6);push(&m,er(&m,2));push(&m,er(&m,0));c->ax=lr(&m,-2);add_ax(c,0x17);push(&m,c->ax);
    status=release(&m,0x261B,system,cap,d);if(status!=PR_R0464_2595_RETURNED)return status;
    les(&m,6);c->ax=0;logic(c,0);wr(&m,c->es,c->di,c->ax);wr(&m,c->es,(uint16_t)(c->di+2u),c->ax);
done:
    c->sp=c->bp;c->bp=rd(&m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    p=physical(c->ss,c->sp);c->ip=word(ram,p);c->cs=word(ram,(p+2)&MASK);
    c->sp=(uint16_t)(c->sp+8u);return PR_R0464_2595_RETURNED;
}
