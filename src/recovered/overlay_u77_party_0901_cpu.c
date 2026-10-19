/* Complete U77 +0901; see U77_PARTY_0901_CPU.md. */
#include "poolrad/recovered/overlay_u77_party_0901_cpu.h"
#include "poolrad/recovered/resident_00ba_byte_string_1037_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#define MASK ((1u<<20)-1u)
typedef struct { pr_bus *b; pr_u77_0901_cpu *c; } machine;
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
/* DS/SS and all flat-RAM callee operands are outside the EGA aperture. */
static uint16_t at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static uint16_t rd(machine *m,uint16_t s,uint16_t o) { return at(m->b->ram,physical(s,o)); }
static uint8_t rb(machine *m,uint16_t s,uint16_t o) { return m->b->ram[physical(s,o)]; }
static void wr(machine *m,uint16_t s,uint16_t o,uint16_t v) { size_t p=physical(s,o);m->b->ram[p]=(uint8_t)v;m->b->ram[(p+1)&MASK]=(uint8_t)(v>>8); }
static void wb(machine *m,uint16_t s,uint16_t o,uint8_t v) { m->b->ram[physical(s,o)]=v; }
static uint16_t local(machine *m,int d) { return (uint16_t)(m->c->bp+d); }
static void push(machine *m,uint16_t v) { m->c->sp=(uint16_t)(m->c->sp-2u);wr(m,m->c->ss,m->c->sp,v); }
static void al(pr_u77_0901_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&0xFF00u)|v); }
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
static void arithmetic(pr_u77_0901_cpu *c,uint16_t a,uint16_t b,unsigned bits,int subtract,int increment) {
    uint32_t mask=bits==8?255u:65535u,sign=bits==8?128u:32768u;
    uint32_t full=subtract?(uint32_t)a-b:(uint32_t)a+b,r=full&mask;
    c->flags&=(uint16_t)~(increment?0x8D4u:0x8D5u);
    if(!increment && (subtract?a<b:full>mask))c->flags|=1;
    if(parity((uint8_t)r))c->flags|=4;
    if((a^b^r)&0x10)c->flags|=0x10;
    if(r==0)c->flags|=0x40;
    if(r&sign)c->flags|=0x80;
    if((subtract?(a^b):~(a^b))&(a^r)&sign)c->flags|=0x800;
}
static void inc_byte(machine *m,uint16_t s,uint16_t o) { uint8_t v=rb(m,s,o);arithmetic(m->c,v,1,8,0,1);wb(m,s,o,(uint8_t)(v+1u)); }
#define COPY(to,from) do {     (to).ax=(from).ax; (to).bx=(from).bx; (to).cx=(from).cx;     (to).dx=(from).dx; (to).si=(from).si; (to).di=(from).di;     (to).bp=(from).bp; (to).sp=(from).sp; (to).cs=(from).cs;     (to).ip=(from).ip; (to).ds=(from).ds; (to).es=(from).es;     (to).ss=(from).ss; (to).flags=(from).flags; } while(0)
static void les_record(machine *m) {
    size_t p=physical(m->c->ds,0x5D92);
    m->c->di=at(m->b->ram,p);m->c->es=at(m->b->ram,(p+2)&MASK);
}
static void row(machine *m) {
    /* 98 is CBW in 16-bit mode (some Capstone versions print CWDE). */
    al(m->c,rb(m,m->c->ss,local(m,8)));
    m->c->ax=(uint16_t)((m->c->ax&0x80)?(m->c->ax|0xFF00):(m->c->ax&255));
}
static void y_arg(machine *m) { row(m);arithmetic(m->c,m->c->ax,7,16,0,0);m->c->ax=(uint16_t)(m->c->ax+7u);push(m,m->c->ax); }
static void local_ptr(machine *m,int offset) { m->c->di=local(m,offset);push(m,m->c->ss);push(m,m->c->di); }
static void literal(machine *m,uint16_t off) { m->c->di=off;push(m,m->c->cs);push(m,m->c->di); }
static void store_args(machine *m) { local_ptr(m,-0x29);m->c->ax=0x28;push(m,m->c->ax); }
static pr_u77_0901_status call(machine *m,uint16_t site,uint16_t target,pr_u77_0901_callee which,
    const pr_u77_0901_limits *l,pr_u77_0901_diagnostic *d)
{
    pr_u77_0901_cpu *c=m->c;uint16_t caller=c->cs,next=(uint16_t)(site+5u),seg;
    c->ip=site;
    if(rb(m,caller,site)!=0x9A || rd(m,caller,(uint16_t)(site+1u))!=target)return PR_U77_0901_CODE;
    seg=rd(m,caller,(uint16_t)(site+3u));push(m,caller);push(m,next);c->cs=seg;c->ip=target;d->callee=(int)which;
    if(which==PR_U77_0901_CLEAR) {
        pr_r03f1_clear_cpu inner;COPY(inner,*c);
        d->status=(int)pr_r03f1_clear_0137_cpu(m->b,1u<<20,&inner,l->clear_loopbacks,l->rect_loopbacks,&d->clear);COPY(*c,inner);
    } else if(which==PR_U77_0901_DECIMAL) {
        pr_res_1037_cpu inner;pr_res_1037_callee dc;COPY(inner,*c);
        d->status=(int)pr_res_00ba_1037_cpu(m->b->ram,1u<<20,&inner,&dc,&d->decimal_status);
        COPY(*c,inner);d->decimal_callee=(int)dc;
    } else if(which==PR_U77_0901_PRINTER) {
        pr_r0709_printer_cpu inner;COPY(inner,*c);
        d->status=(int)pr_r0709_printer_0626_cpu(m->b,1u<<20,&inner,l->printer_loopbacks,l->glyph_loopbacks,&d->printer);COPY(*c,inner);
    } else if(which==PR_U77_0901_STRSTORE) {
        pr_system_0b25_cpu inner;COPY(inner,*c);
        d->status=(int)pr_system_strstore_0b25(m->b->ram,1u<<20,&inner);COPY(*c,inner);
    } else {
        pr_system_leaf_cpu inner;COPY(inner,*c);
        d->status=(int)(which==PR_U77_0901_STRLOAD?pr_system_strload_0b0b(m->b->ram,1u<<20,&inner):
            pr_system_strcat_0b98(m->b->ram,1u<<20,&inner));COPY(*c,inner);
    }
    if(d->status)return PR_U77_0901_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U77_0901_TRANSFER;
    return PR_U77_0901_OK;
}
#define CALL(site,target,which) do { status=call(&m,site,target,which,l,d);if(status)return status; } while(0)
static int plain_segment(uint16_t s) { return s<=0x9000 || s>=0xB000; }
pr_u77_0901_status pr_u77_party_0901_cpu(pr_bus *b,size_t size,pr_u77_0901_cpu *c,
    const pr_u77_0901_limits *l,pr_u77_0901_diagnostic *d)
{
    pr_u77_0901_diagnostic fallback={-1,-1,{-1,-1,{-1,-1}},-1,-1,{-1,-1,-1,-1}};
    machine m={b,c};pr_u77_0901_status status;size_t p;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(b==NULL || b->ram==NULL || b->ega==NULL || c==NULL || l==NULL || size!=(1u<<20) ||
       c->ip!=0x901 || c->sp<0x300 || c->sp>0xFFF4 || !plain_segment(c->ds) || !plain_segment(c->ss))return PR_U77_0901_ARGUMENT;
    push(&m,c->bp);c->bp=c->sp;arithmetic(c,c->sp,0x12C,16,1,0);c->sp=(uint16_t)(c->sp-0x12Cu);
    arithmetic(c,rb(&m,c->ss,local(&m,6)),0,8,1,0);wb(&m,c->ss,local(&m,-0x2B),(c->flags&0x40)?10:13);
    wb(&m,c->ss,local(&m,-0x2A),5);
    al(c,rb(&m,c->ss,local(&m,-0x2A)));push(&m,c->ax);y_arg(&m);al(c,11);push(&m,c->ax);y_arg(&m);
    CALL(0x933,0x137,PR_U77_0901_CLEAR);
    row(&m);les_record(&m);arithmetic(c,c->di,c->ax,16,0,0);c->di=(uint16_t)(c->di+c->ax);
    arithmetic(c,rb(&m,c->es,(uint16_t)(c->di+0x10u)),10,8,1,0);
    if(c->flags&1)inc_byte(&m,c->ss,local(&m,-0x2A));
    al(c,rb(&m,c->ss,local(&m,-0x2A)));push(&m,c->ax);y_arg(&m);al(c,rb(&m,c->ss,local(&m,-0x2B)));push(&m,c->ax);
    local_ptr(&m,-0x12B);row(&m);les_record(&m);arithmetic(c,c->di,c->ax,16,0,0);c->di=(uint16_t)(c->di+c->ax);
    al(c,rb(&m,c->es,(uint16_t)(c->di+0x10u)));push(&m,c->ax);
    CALL(0x971,0x1037,PR_U77_0901_DECIMAL);
    CALL(0x976,0x626,PR_U77_0901_PRINTER);
    arithmetic(c,rb(&m,c->ss,local(&m,8)),0,8,1,0);if(!(c->flags&0x40))goto finish;
    les_record(&m);arithmetic(c,rb(&m,c->es,(uint16_t)(c->di+0x10u)),18,8,1,0);if(!(c->flags&0x40))goto finish;
    les_record(&m);arithmetic(c,rb(&m,c->es,(uint16_t)(c->di+0x16u)),0,8,1,0);if(c->flags&0x41)goto finish;
    local_ptr(&m,-0x12B);les_record(&m);al(c,rb(&m,c->es,(uint16_t)(c->di+0x16u)));push(&m,c->ax);
    CALL(0x9AF,0x1037,PR_U77_0901_DECIMAL);
    store_args(&m);CALL(0x9BD,0xB25,PR_U77_0901_STRSTORE);
    les_record(&m);arithmetic(c,rb(&m,c->es,(uint16_t)(c->di+0x16u)),10,8,1,0);
    if(c->flags&1) {
        local_ptr(&m,-0x12B);literal(&m,0x8F8);CALL(0x9D8,0xB0B,PR_U77_0901_STRLOAD);
        local_ptr(&m,-0x29);CALL(0x9E2,0xB98,PR_U77_0901_STRCAT);
        store_args(&m);CALL(0x9F0,0xB25,PR_U77_0901_STRSTORE);
    }
    les_record(&m);arithmetic(c,rb(&m,c->es,(uint16_t)(c->di+0x16u)),100,8,1,0);
    if(c->flags&0x40) { literal(&m,0x8FA);store_args(&m);CALL(0xA0E,0xB25,PR_U77_0901_STRSTORE); }
    al(c,7);push(&m,c->ax);al(c,7);push(&m,c->ax);al(c,rb(&m,c->ss,local(&m,-0x2B)));push(&m,c->ax);
    local_ptr(&m,-0x12B);literal(&m,0x8FD);CALL(0xA28,0xB0B,PR_U77_0901_STRLOAD);
    local_ptr(&m,-0x29);CALL(0xA32,0xB98,PR_U77_0901_STRCAT);
    literal(&m,0x8FF);CALL(0xA3C,0xB98,PR_U77_0901_STRCAT);
    CALL(0xA41,0x626,PR_U77_0901_PRINTER);
finish:
    c->sp=c->bp;c->bp=rd(&m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    p=physical(c->ss,c->sp);c->ip=at(b->ram,p);c->cs=at(b->ram,(p+2)&MASK);c->sp=(uint16_t)(c->sp+8u);
    return PR_U77_0901_OK;
}
