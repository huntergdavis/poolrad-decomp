/* U3B9 [048F,04F8): signed-count bitmap-set release and fixed 66-byte clear. */
#include "poolrad/recovered/overlay_u3b9_bitmap_set_release_048f_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#define MASK (PR_SYSTEM_FREEMEM_RAM_SIZE-1u)
typedef struct { uint8_t *r; pr_u3b9_048f_cpu *c; } machine;
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static uint16_t rd(machine *m,uint16_t s,uint16_t o) { return word(m->r,physical(s,o)); }
static void wr(machine *m,uint16_t s,uint16_t o,uint16_t v) {
    size_t p=physical(s,o);m->r[p]=(uint8_t)v;m->r[(p+1)&MASK]=(uint8_t)(v>>8);
}
static void push(machine *m,uint16_t v) { m->c->sp=(uint16_t)(m->c->sp-2u);wr(m,m->c->ss,m->c->sp,v); }
static void les(machine *m,int d) {
    size_t p=physical(m->c->ss,(uint16_t)(m->c->bp+d));m->c->di=word(m->r,p);m->c->es=word(m->r,(p+2)&MASK);
}
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
static void szp(pr_u3b9_048f_cpu *c,uint16_t r,uint16_t sign) {
    if(parity((uint8_t)r))c->flags|=4;
    if(r==0)c->flags|=0x40;
    if(r&sign)c->flags|=0x80;
}
static void sub(pr_u3b9_048f_cpu *c,uint16_t a,uint16_t b,uint16_t sign) {
    uint16_t r=(uint16_t)((a-b)&(uint16_t)((sign<<1)-1u));c->flags&=(uint16_t)~0x8D5u;
    if(a<b)c->flags|=1;
    if((a^b^r)&0x10)c->flags|=0x10;
    if((a^b)&(a^r)&sign)c->flags|=0x800;
    szp(c,r,sign);
}
static uint16_t add(pr_u3b9_048f_cpu *c,uint16_t a,uint16_t b) {
    uint16_t r=(uint16_t)(a+b);c->flags&=(uint16_t)~0x8D5u;
    if((uint32_t)a+b>65535)c->flags|=1;
    if((a^b^r)&0x10)c->flags|=0x10;
    if((uint16_t)~(a^b)&(a^r)&0x8000)c->flags|=0x800;
    szp(c,r,0x8000);return r;
}
#define COPY_CPU(dst,src) do { \
 (dst).ax=(src).ax;(dst).bx=(src).bx;(dst).cx=(src).cx;(dst).dx=(src).dx; \
 (dst).si=(src).si;(dst).di=(src).di;(dst).bp=(src).bp;(dst).sp=(src).sp; \
 (dst).cs=(src).cs;(dst).ip=(src).ip;(dst).ds=(src).ds;(dst).es=(src).es; \
 (dst).ss=(src).ss;(dst).flags=(src).flags; } while(0)
pr_u3b9_048f_status pr_u3b9_bitmap_set_release_048f_cpu(uint8_t *ram,size_t size,
    pr_u3b9_048f_cpu *c,uint16_t resident,uint16_t system,unsigned cap,unsigned heap_cap,
    pr_u3b9_048f_diagnostic *d) {
    machine m={ram,c};pr_u3b9_048f_diagnostic local={-1,{-1,0,0},0,0};
    uint16_t caller;size_t p;pr_system_leaf_cpu leaf;unsigned v;
    if(!ram || !c || size!=PR_SYSTEM_FREEMEM_RAM_SIZE || c->ip!=0x48F ||
       c->sp<0x80 || c->sp>0xFFF8)return PR_U3B9_048F_ARGUMENT;
    if(ram[physical(c->cs,0x4CA)]!=0x9A || rd(&m,c->cs,0x4CB)!=0x2595 || rd(&m,c->cs,0x4CD)!=resident ||
       ram[physical(c->cs,0x4E3)]!=0x9A || rd(&m,c->cs,0x4E4)!=0x16A8 || rd(&m,c->cs,0x4E6)!=system)
        return PR_U3B9_048F_ARGUMENT;
    if(!d)d=&local;
    *d=local;caller=c->cs;
    push(&m,c->bp);c->bp=c->sp;sub(c,c->sp,6,0x8000);c->sp=(uint16_t)(c->sp-6); /* 048F..0492 */
    les(&m,6);wr(&m,c->ss,(uint16_t)(c->bp-4),c->di);wr(&m,c->ss,(uint16_t)(c->bp-2),c->es);
    les(&m,-4);c->ax=(uint16_t)((c->ax&0xFF00u)|ram[physical(c->es,c->di)]);
    ram[physical(c->ss,(uint16_t)(c->bp-5))]=(uint8_t)c->ax;c->ax=(uint16_t)((c->ax&0xFF00u)|1u);
    sub(c,1,ram[physical(c->ss,(uint16_t)(c->bp-5))],0x80); /* 04A9 */
    if(!(c->flags&0x40) && !!(c->flags&0x80)==!!(c->flags&0x800))goto fill; /* JG */
    ram[physical(c->ds,0x70C7)]=(uint8_t)c->ax;
    for(;;) {
        c->ax=(uint16_t)((c->ax&0xFF00u)|ram[physical(c->ds,0x70C7)]); /* 04B7 */
        c->ax=(uint16_t)(int16_t)(int8_t)c->ax;c->cx=(uint16_t)((c->cx&0xFF00u)|3u);c->ax=(uint16_t)(c->ax<<3);
        /* SHL's flags are overwritten by ADD before any observable stop. */
        les(&m,-4);c->di=add(c,c->di,c->ax);c->di=add(c,c->di,0xFFFE);
        push(&m,c->es);push(&m,c->di);c->ip=0x4CA;
        if(d->release_calls==cap)return PR_U3B9_048F_LIMIT;
        ++d->release_calls;push(&m,caller);push(&m,0x4CF);c->cs=resident;c->ip=0x2595;
        d->resident_status=(int)pr_r0464_bitmap_release_2595_cpu(ram,size,c,system,heap_cap,&d->resident);
        if(d->resident_status!=PR_R0464_2595_RETURNED)return PR_U3B9_048F_CALLEE;
        if(c->cs!=caller || c->ip!=0x4CF)return PR_U3B9_048F_TRANSFER;
        c->ax=(uint16_t)((c->ax&0xFF00u)|ram[physical(c->ds,0x70C7)]);
        sub(c,(uint8_t)c->ax,ram[physical(c->ss,(uint16_t)(c->bp-5))],0x80);
        if(c->flags&0x40)break;
        /* INC byte keeps CF; its other flags are overwritten before CALL. */
        p=physical(c->ds,0x70C7);v=(unsigned)ram[p]+1u;ram[p]=(uint8_t)v;
    }
fill:
    les(&m,6);push(&m,c->es);push(&m,c->di);c->ax=0x42;push(&m,c->ax);c->ax=0;push(&m,c->ax);
    ++d->fill_calls;push(&m,caller);push(&m,0x4E8);c->cs=system;c->ip=0x16A8;
    COPY_CPU(leaf,*c);(void)pr_system_fillchar_16a8(ram,size,&leaf);COPY_CPU(*c,leaf);
    if(c->cs!=caller || c->ip!=0x4E8)return PR_U3B9_048F_TRANSFER;
    ram[physical(c->ds,0x6B0A)]=0;ram[physical(c->ds,0x6B1C)]=0xFF;
    c->sp=c->bp;c->bp=rd(&m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2);
    p=physical(c->ss,c->sp);c->ip=word(ram,p);c->cs=word(ram,(p+2)&MASK);c->sp=(uint16_t)(c->sp+8);
    return PR_U3B9_048F_RETURNED;
}
