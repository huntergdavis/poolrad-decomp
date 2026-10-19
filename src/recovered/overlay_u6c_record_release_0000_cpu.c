/* U6C record release, exact [0000,00E2); see U6C_RECORD_RELEASE_0000_CPU.md.
 * Memory/flag idioms follow merged overlay_u9e_list_release_0026_cpu.c;
 * the existing SYSTEM FreeMem implementation is composed unchanged. */
#include "poolrad/recovered/overlay_u6c_record_release_0000_cpu.h"
#include "poolrad/recovered/system_freemem_0254_cpu.h"
#define MASK (PR_U6C_0000_RAM_SIZE-1u)
typedef struct { uint8_t *r; pr_u6c_0000_cpu *c; } machine;
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word_at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static uint16_t rd(machine *m,uint16_t s,uint16_t o) { return word_at(m->r,physical(s,o)); }
static void wr(machine *m,uint16_t s,uint16_t o,uint16_t v) {
    size_t p=physical(s,o);m->r[p]=(uint8_t)v;m->r[(p+1)&MASK]=(uint8_t)(v>>8);
}
static uint16_t lr(machine *m,int d) { return rd(m,m->c->ss,(uint16_t)(m->c->bp+d)); }
static void lw(machine *m,int d,uint16_t v) { wr(m,m->c->ss,(uint16_t)(m->c->bp+d),v); }
static void push(machine *m,uint16_t v) { m->c->sp=(uint16_t)(m->c->sp-2u);wr(m,m->c->ss,m->c->sp,v); }
static void les(machine *m,int d) {
    size_t p=physical(m->c->ss,(uint16_t)(m->c->bp+d));
    m->c->di=word_at(m->r,p);m->c->es=word_at(m->r,(p+2)&MASK);
}
static uint16_t er(machine *m,unsigned d) { return rd(m,m->c->es,(uint16_t)(m->c->di+d)); }
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
static void szp(pr_u6c_0000_cpu *c,uint16_t r) {
    if(parity((uint8_t)r))c->flags|=4;
    if(r==0)c->flags|=0x40;
    if(r&0x8000)c->flags|=0x80;
}
static void sub(pr_u6c_0000_cpu *c,uint16_t a,uint16_t b) {
    uint16_t r=(uint16_t)(a-b);c->flags&=(uint16_t)~0x08D5u;
    if(a<b)c->flags|=1;
    if((a^b^r)&0x10)c->flags|=0x10;
    if((a^b)&(a^r)&0x8000)c->flags|=0x800;
    szp(c,r);
}
static void logic(pr_u6c_0000_cpu *c,uint16_t r) { c->flags&=(uint16_t)~0x08D5u;szp(c,r); }
#define COPY_CPU(to,from) do { \
    (to).ax=(from).ax; (to).bx=(from).bx; (to).cx=(from).cx; \
    (to).dx=(from).dx; (to).si=(from).si; (to).di=(from).di; \
    (to).bp=(from).bp; (to).sp=(from).sp; (to).cs=(from).cs; \
    (to).ip=(from).ip; (to).ds=(from).ds; (to).es=(from).es; \
    (to).ss=(from).ss; (to).flags=(from).flags; \
} while(0)
static pr_u6c_0000_status release(machine *m,uint16_t site,
    const pr_u6c_0000_limits *l,pr_u6c_0000_diagnostic *d)
{
    pr_u6c_0000_cpu *c=m->c;pr_system_freemem_cpu inner;
    uint16_t caller=c->cs,segment,continuation=(uint16_t)(site+5u);
    c->ip=site;d->call_site=site;
    if(m->r[physical(caller,site)]!=0x9A || rd(m,caller,(uint16_t)(site+1u))!=0x254)
        return PR_U6C_0000_CODE;
    segment=rd(m,caller,(uint16_t)(site+3u));push(m,caller);push(m,continuation);
    c->cs=segment;c->ip=0x254;COPY_CPU(inner,*c);
    d->freemem_status=(int)pr_system_freemem_0254(m->r,PR_U6C_0000_RAM_SIZE,&inner,l->max_heap_loopbacks);
    COPY_CPU(*c,inner);
    if(d->freemem_status!=PR_SYSTEM_FREEMEM_RETURNED)return PR_U6C_0000_CALLEE;
    if(c->cs!=caller || c->ip!=continuation)return PR_U6C_0000_TRANSFER;
    return PR_U6C_0000_OK;
}
pr_u6c_0000_status pr_u6c_record_release_0000_cpu(uint8_t *ram,size_t size,
    pr_u6c_0000_cpu *c,const pr_u6c_0000_limits *l,pr_u6c_0000_diagnostic *d)
{
    machine m={ram,c};pr_u6c_0000_diagnostic fallback={-1,0};
    pr_u6c_0000_status status;unsigned loops=0;size_t p;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || c==NULL || l==NULL || size!=PR_U6C_0000_RAM_SIZE ||
       c->ip!=0 || c->sp<0x40 || c->sp>0xFFF8)return PR_U6C_0000_ARGUMENT;
    push(&m,c->bp);c->bp=c->sp;sub(c,c->sp,0x14);c->sp=(uint16_t)(c->sp-0x14);
    les(&m,6);lw(&m,-0x14,c->di);lw(&m,-0x12,c->es);
    les(&m,-0x14);c->ax=er(&m,0x108);c->ax|=er(&m,0x10A);logic(c,c->ax);
    if(!(c->flags&0x40)) {
        les(&m,-0x14);push(&m,er(&m,0x10A));push(&m,er(&m,0x108));c->ax=0x16;push(&m,c->ax);
        status=release(&m,0x2F,l,d);if(status!=PR_U6C_0000_OK)return status;
    }
    les(&m,-0x14);c->ax=er(&m,0xC8);c->dx=er(&m,0xCA);lw(&m,-8,c->ax);lw(&m,-6,c->dx);
list63:
    c->ax=lr(&m,-8);c->ax|=lr(&m,-6);logic(c,c->ax);
    if(!(c->flags&0x40)) {
        c->ax=lr(&m,-8);c->dx=lr(&m,-6);lw(&m,-4,c->ax);lw(&m,-2,c->dx);
        les(&m,-8);c->ax=er(&m,0x2A);c->dx=er(&m,0x2C);lw(&m,-8,c->ax);lw(&m,-6,c->dx);
        push(&m,lr(&m,-2));push(&m,lr(&m,-4));c->ax=0x3F;push(&m,c->ax);
        status=release(&m,0x76,l,d);if(status!=PR_U6C_0000_OK)return status;
        c->ip=0x47;if(loops==l->max_list_loopbacks)return PR_U6C_0000_LIMIT;
        ++loops;goto list63;
    }
    les(&m,-0x14);c->ax=er(&m,0x7F);c->dx=er(&m,0x81);lw(&m,-0x10,c->ax);lw(&m,-0xE,c->dx);
list9:
    c->ax=lr(&m,-0x10);c->ax|=lr(&m,-0xE);logic(c,c->ax);
    if(!(c->flags&0x40)) {
        c->ax=lr(&m,-0x10);c->dx=lr(&m,-0xE);lw(&m,-0xC,c->ax);lw(&m,-0xA,c->dx);
        les(&m,-0x10);c->ax=er(&m,5);c->dx=er(&m,7);lw(&m,-0x10,c->ax);lw(&m,-0xE,c->dx);
        push(&m,lr(&m,-0xA));push(&m,lr(&m,-0xC));c->ax=9;push(&m,c->ax);
        status=release(&m,0xBE,l,d);if(status!=PR_U6C_0000_OK)return status;
        c->ip=0x8F;if(loops==l->max_list_loopbacks)return PR_U6C_0000_LIMIT;
        ++loops;goto list9;
    }
    push(&m,lr(&m,8));push(&m,lr(&m,6));c->ax=0x11D;push(&m,c->ax);
    status=release(&m,0xCF,l,d);if(status!=PR_U6C_0000_OK)return status;
    c->ax=0;logic(c,c->ax);lw(&m,6,c->ax);lw(&m,8,c->ax);
    c->sp=c->bp;c->bp=rd(&m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    p=physical(c->ss,c->sp);c->ip=word_at(ram,p);c->cs=word_at(ram,(p+2)&MASK);
    c->sp=(uint16_t)(c->sp+8u);return PR_U6C_0000_OK;
}
