/* U6C record unlink, exact [3291,33D7); see U6C_RECORD_UNLINK_3291_CPU.md.
 * Memory/flag idioms follow merged overlay_u6c_record_release_0000_cpu.c; the
 * +0000 and U3CE +014E models are composed unchanged. */
#include "poolrad/recovered/overlay_u6c_record_unlink_3291_cpu.h"
#define MASK (PR_U6C_3291_RAM_SIZE-1u)
#define BASE 0x3291u
#define AT(x) ((uint16_t)(BASE+(x)))
typedef struct { uint8_t *r; pr_u6c_3291_cpu *c; } machine;
_Static_assert(sizeof(pr_u6c_3291_cpu)==sizeof(pr_u3ce_014e_cpu),"+014E CPU layout differs");
_Static_assert(sizeof(pr_u6c_3291_cpu)==sizeof(pr_u6c_0000_cpu),"+0000 CPU layout differs");
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word_at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static uint16_t rd(machine *m,uint16_t s,uint16_t o) { return word_at(m->r,physical(s,o)); }
static uint8_t rd8(machine *m,uint16_t s,uint16_t o) { return m->r[physical(s,o)]; }
static void wr(machine *m,uint16_t s,uint16_t o,uint16_t v) {
    size_t p=physical(s,o);m->r[p]=(uint8_t)v;m->r[(p+1)&MASK]=(uint8_t)(v>>8);
}
static uint16_t lr(machine *m,int d) { return rd(m,m->c->ss,(uint16_t)(m->c->bp+d)); }
static void lw(machine *m,int d,uint16_t v) { wr(m,m->c->ss,(uint16_t)(m->c->bp+d),v); }
static uint16_t dsr(machine *m,uint16_t o) { return rd(m,m->c->ds,o); }
static void dsw(machine *m,uint16_t o,uint16_t v) { wr(m,m->c->ds,o,v); }
static void push(machine *m,uint16_t v) { m->c->sp=(uint16_t)(m->c->sp-2u);wr(m,m->c->ss,m->c->sp,v); }
static void les(machine *m,uint16_t s,uint16_t o) {
    size_t p=physical(s,o);
    m->c->di=word_at(m->r,p);m->c->es=word_at(m->r,(p+2)&MASK);
}
static uint16_t er(machine *m,unsigned d) { return rd(m,m->c->es,(uint16_t)(m->c->di+d)); }
static void ew(machine *m,unsigned d,uint16_t v) { wr(m,m->c->es,(uint16_t)(m->c->di+d),v); }
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
static void szp(pr_u6c_3291_cpu *c,uint16_t r,uint16_t sign) {
    if(parity((uint8_t)r))c->flags|=4;
    if(r==0)c->flags|=0x40;
    if(r&sign)c->flags|=0x80;
}
static void sub16(pr_u6c_3291_cpu *c,uint16_t a,uint16_t b) {
    uint16_t r=(uint16_t)(a-b);c->flags&=(uint16_t)~0x08D5u;
    if(a<b)c->flags|=1;
    if((a^b^r)&0x10)c->flags|=0x10;
    if((a^b)&(a^r)&0x8000)c->flags|=0x800;
    szp(c,r,0x8000);
}
static void sub8(pr_u6c_3291_cpu *c,uint8_t a,uint8_t b) {
    uint8_t r=(uint8_t)(a-b);c->flags&=(uint16_t)~0x08D5u;
    if(a<b)c->flags|=1;
    if((a^b^r)&0x10)c->flags|=0x10;
    if((a^b)&(a^r)&0x80)c->flags|=0x800;
    szp(c,r,0x80);
}
static void logic(pr_u6c_3291_cpu *c,uint16_t r) { c->flags&=(uint16_t)~0x08D5u;szp(c,r,0x8000); }
static void dec_es(machine *m,unsigned d) {
    pr_u6c_3291_cpu *c=m->c;uint16_t a=er(m,d),r=(uint16_t)(a-1u);
    c->flags&=(uint16_t)~0x08D4u; /* CF is preserved */
    if((a^1u^r)&0x10)c->flags|=0x10;
    if(a==0x8000u)c->flags|=0x800;
    szp(c,r,0x8000);ew(m,d,r);
}
#define COPY_CPU(to,from) do { \
    (to).ax=(from).ax; (to).bx=(from).bx; (to).cx=(from).cx; \
    (to).dx=(from).dx; (to).si=(from).si; (to).di=(from).di; \
    (to).bp=(from).bp; (to).sp=(from).sp; (to).cs=(from).cs; \
    (to).ip=(from).ip; (to).ds=(from).ds; (to).es=(from).es; \
    (to).ss=(from).ss; (to).flags=(from).flags; \
} while(0)
/* LCALL 03E9:002F (RETF-2 callee): operands are checked before the pushes;
 * the stub is read after them, when the CPU reaches it. */
static pr_u6c_3291_status pair_release(machine *m,uint16_t site,uint16_t resident,
    uint16_t system,const pr_u6c_3291_limits *l,pr_u6c_3291_diagnostic *d)
{
    pr_u6c_3291_cpu *c=m->c;pr_u3ce_014e_cpu inner;
    uint16_t caller=c->cs,continuation=(uint16_t)(site+5u),stub;
    c->ip=site;d->call_site=site;
    if(rd8(m,caller,site)!=0x9A || rd(m,caller,(uint16_t)(site+1u))!=0x2F)return PR_U6C_3291_CODE;
    stub=rd(m,caller,(uint16_t)(site+3u));push(m,caller);push(m,continuation);
    c->cs=stub;c->ip=0x2F;
    if(rd8(m,stub,0x2F)!=0xEA || rd(m,stub,0x30)!=0x14E)return PR_U6C_3291_CODE;
    c->cs=rd(m,stub,0x32);c->ip=0x14E;COPY_CPU(inner,*c);
    d->callee=PR_U6C_3291_PAIR_RELEASE;++d->pair_calls;
    d->callee_status=(int)pr_u3ce_bitmap_pair_release_014e_cpu(m->r,PR_U6C_3291_RAM_SIZE,
        &inner,resident,system,l->max_heap_steps,&d->pair);
    COPY_CPU(*c,inner);
    if(d->callee_status!=(int)PR_U3CE_014E_RETURNED)return PR_U6C_3291_CALLEE;
    if(c->cs!=caller || c->ip!=continuation)return PR_U6C_3291_TRANSFER;
    return PR_U6C_3291_OK;
}
/* The two PUSHes of the record far pointer have run; PUSH CS / CALL +0000. */
static pr_u6c_3291_status record_release(machine *m,uint16_t site,
    const pr_u6c_3291_limits *l,pr_u6c_3291_diagnostic *d)
{
    pr_u6c_3291_cpu *c=m->c;pr_u6c_0000_cpu inner;pr_u6c_0000_limits limits;
    uint16_t caller=c->cs,continuation=(uint16_t)(site+3u);
    c->ip=(uint16_t)(site-1u);d->call_site=site;
    if(rd8(m,caller,(uint16_t)(site-1u))!=0x0E || rd8(m,caller,site)!=0xE8 ||
       rd(m,caller,(uint16_t)(site+1u))!=(uint16_t)(0u-continuation))return PR_U6C_3291_CODE;
    push(m,caller);push(m,continuation);c->ip=0;COPY_CPU(inner,*c);
    limits.max_list_loopbacks=l->max_list_loopbacks;limits.max_heap_loopbacks=l->max_heap_steps;
    d->callee=PR_U6C_3291_RECORD_RELEASE;++d->record_calls;
    d->callee_status=(int)pr_u6c_record_release_0000_cpu(m->r,PR_U6C_3291_RAM_SIZE,&inner,&limits,&d->record);
    COPY_CPU(*c,inner);
    if(d->callee_status!=(int)PR_U6C_0000_OK)return PR_U6C_3291_CALLEE;
    if(c->cs!=caller || c->ip!=continuation)return PR_U6C_3291_TRANSFER;
    return PR_U6C_3291_OK;
}
pr_u6c_3291_status pr_u6c_record_unlink_3291_cpu(uint8_t *ram,size_t size,
    pr_u6c_3291_cpu *c,uint16_t resident,uint16_t system,
    const pr_u6c_3291_limits *l,pr_u6c_3291_diagnostic *d)
{
    machine m={ram,c};pr_u6c_3291_diagnostic fallback;
    pr_u6c_3291_status status;unsigned loops=0;size_t p;
    fallback.callee=PR_U6C_3291_NO_CALLEE;fallback.callee_status=-1;
    fallback.pair.resident_status=-1;fallback.pair.resident.freemem_status=-1;
    fallback.pair.resident.call_site=0;fallback.pair.resident.calls=0;
    fallback.pair.calls=0;fallback.pair.call_site=0;
    fallback.record.freemem_status=-1;fallback.record.call_site=0;
    fallback.pair_calls=0;fallback.record_calls=0;fallback.call_site=0;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || c==NULL || l==NULL || size!=PR_U6C_3291_RAM_SIZE ||
       c->ip!=BASE || c->sp<0x8C || c->sp>0xFFF8)return PR_U6C_3291_ARGUMENT;
    push(&m,c->bp);c->bp=c->sp;sub16(c,c->sp,4);c->sp=(uint16_t)(c->sp-4u);
    c->ax=dsr(&m,0x5D96);c->ax|=dsr(&m,0x5D98);logic(c,c->ax);
    if(c->flags&0x40)goto leave;
    c->ax=dsr(&m,0x5D96);c->dx=dsr(&m,0x5D98);lw(&m,-4,c->ax);lw(&m,-2,c->dx);
    c->ax=lr(&m,-4);c->dx=lr(&m,-2);
    sub16(c,c->dx,dsr(&m,0x5D94));
    if(c->flags&0x40) { sub16(c,c->ax,dsr(&m,0x5D92)); if(c->flags&0x40)goto head; }
walk:
    les(&m,c->ss,(uint16_t)(c->bp-4));c->ax=er(&m,0x104);c->dx=er(&m,0x106);
    sub16(c,c->dx,dsr(&m,0x5D94));
    if(c->flags&0x40) { sub16(c,c->ax,dsr(&m,0x5D92)); if(c->flags&0x40)goto found; }
    les(&m,c->ss,(uint16_t)(c->bp-4));c->ax=er(&m,0x104);c->ax|=er(&m,0x106);logic(c,c->ax);
    if(c->flags&0x40)goto found;
    les(&m,c->ss,(uint16_t)(c->bp-4));c->ax=er(&m,0x104);c->dx=er(&m,0x106);lw(&m,-4,c->ax);lw(&m,-2,c->dx);
    c->ip=AT(0x34);if(loops==l->max_walk_loopbacks)return PR_U6C_3291_LIMIT;
    ++loops;goto walk;
found:
    les(&m,c->ss,(uint16_t)(c->bp-4));c->ax=er(&m,0x104);c->ax|=er(&m,0x106);logic(c,c->ax);
    if(c->flags&0x40)goto leave;
    sub8(c,(uint8_t)lr(&m,6),0);
    if(!(c->flags&0x40)) {
        les(&m,c->ds,0x5D92);c->ax=(uint16_t)((c->ax&0xFF00u)|rd8(&m,c->es,(uint16_t)(c->di+0xBF)));
        push(&m,c->ax);status=pair_release(&m,AT(0x90),resident,system,l,d);
        if(status!=PR_U6C_3291_OK)return status;
    }
    les(&m,c->ds,0x5D92);c->ax=er(&m,0x104);c->dx=er(&m,0x106);
    les(&m,c->ss,(uint16_t)(c->bp-4));ew(&m,0x104,c->ax);ew(&m,0x106,c->dx);
    push(&m,dsr(&m,0x5D94));push(&m,dsr(&m,0x5D92));
    status=record_release(&m,AT(0xB9),l,d);if(status!=PR_U6C_3291_OK)return status;
    c->ax=lr(&m,-4);c->dx=lr(&m,-2);dsw(&m,0x5D92,c->ax);dsw(&m,0x5D94,c->dx);
    goto tail;
head:
    sub8(c,(uint8_t)lr(&m,6),0);
    if(!(c->flags&0x40)) {
        les(&m,c->ds,0x5D92);c->ax=(uint16_t)((c->ax&0xFF00u)|rd8(&m,c->es,(uint16_t)(c->di+0xBF)));
        push(&m,c->ax);status=pair_release(&m,AT(0xEA),resident,system,l,d);
        if(status!=PR_U6C_3291_OK)return status;
    }
    les(&m,c->ds,0x5D92);c->ax=er(&m,0x104);c->dx=er(&m,0x106);dsw(&m,0x5D96,c->ax);dsw(&m,0x5D98,c->dx);
    les(&m,c->ds,0x5D92);c->ax=er(&m,0x104);c->dx=er(&m,0x106);lw(&m,-4,c->ax);lw(&m,-2,c->dx);
    push(&m,dsr(&m,0x5D94));push(&m,dsr(&m,0x5D92));
    status=record_release(&m,AT(0x121),l,d);if(status!=PR_U6C_3291_OK)return status;
    c->ax=lr(&m,-4);c->dx=lr(&m,-2);dsw(&m,0x5D92,c->ax);dsw(&m,0x5D94,c->dx);
tail:
    sub8(c,(uint8_t)lr(&m,8),0);
    if(c->flags&0x40) {
        les(&m,c->ds,0x49D6);dec_es(&m,0x67C);
    }
leave:
    c->sp=c->bp;c->bp=rd(&m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    p=physical(c->ss,c->sp);c->ip=word_at(ram,p);c->cs=word_at(ram,(p+2)&MASK);
    c->sp=(uint16_t)(c->sp+8u);return PR_U6C_3291_OK;
}
