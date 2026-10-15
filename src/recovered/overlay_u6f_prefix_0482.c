#include "poolrad/recovered/overlay_u6f_prefix_0482.h"

static size_t phys(uint16_t seg, uint16_t off) {
    return (((size_t)seg << 4) + off) & (PR_U6F_0482_RAM_SIZE - 1u);
}
static uint8_t rb(const uint8_t *m, uint16_t s, uint16_t o) { return m[phys(s,o)]; }
static void wb(uint8_t *m, uint16_t s, uint16_t o, uint8_t v) { m[phys(s,o)] = v; }
static uint16_t rw(const uint8_t *m, uint16_t s, uint16_t o) {
    size_t a = phys(s,o);
    return (uint16_t)(m[a] | ((uint16_t)m[(a+1u)&0xfffffu] << 8));
}
static void ww(uint8_t *m, uint16_t s, uint16_t o, uint16_t v) {
    size_t a = phys(s,o); m[a] = (uint8_t)v; m[(a+1u)&0xfffffu] = (uint8_t)(v >> 8);
}
static void far_read(const uint8_t *m, uint16_t s, uint16_t o,
                     uint16_t *off, uint16_t *seg) {
    size_t a=phys(s,o);
    *off=(uint16_t)(m[a] | ((uint16_t)m[(a+1u)&0xfffffu]<<8));
    *seg=(uint16_t)(m[(a+2u)&0xfffffu] | ((uint16_t)m[(a+3u)&0xfffffu]<<8));
}
static void les(const uint8_t *m, pr_u6f_0482_cpu *c, uint16_t s, uint16_t o) {
    uint16_t off,seg;far_read(m,s,o,&off,&seg);c->di=off;c->es=seg;
}
static void al(pr_u6f_0482_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&0xff00u)|v); }
static void cl(pr_u6f_0482_cpu *c,uint8_t v) { c->cx=(uint16_t)((c->cx&0xff00u)|v); }
static uint16_t flags_result(uint16_t old,uint16_t a,uint16_t b,
                             uint32_t result,unsigned bits,int sub) {
    uint32_t mask=bits==8u?255u:65535u,sign=1u<<(bits-1u),r=result&mask;
    uint16_t f=(uint16_t)(old&~0x08d5u);unsigned ones=0;uint8_t low=(uint8_t)r;
    for(unsigned i=0;i<8u;++i)ones+=(low>>i)&1u;
    if((ones&1u)==0u)f|=4u;
    if(r==0u)f|=0x40u;
    if(r&sign)f|=0x80u;
    if((a^b^r)&0x10u)f|=0x10u;
    if(sub ? a<b : result>mask)f|=1u;
    if(sub ? (((a^b)&(a^r)&sign)!=0u) : (((~(a^b))&(a^r)&sign)!=0u))f|=0x800u;
    return f;
}
static void xor_high_zero(pr_u6f_0482_cpu *c,int cx) {
    if(cx)c->cx&=255u;else c->ax&=255u;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|0x44u);
}
static void push(uint8_t *m,pr_u6f_0482_cpu *c,uint16_t v) {
    c->sp=(uint16_t)(c->sp-2u);ww(m,c->ss,c->sp,v);
}
static void pop_bp(const uint8_t *m,pr_u6f_0482_cpu *c) {
    c->bp=rw(m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
}
static void retf(const uint8_t *m,pr_u6f_0482_cpu *c,uint16_t n) {
    uint16_t ip,cs;far_read(m,c->ss,c->sp,&ip,&cs);
    c->sp=(uint16_t)(c->sp+4u+n);c->ip=ip;c->cs=cs;
}
static void lodsb(const uint8_t *m,pr_u6f_0482_cpu *c) {
    al(c,rb(m,c->ds,c->si));c->si=(uint16_t)(c->si+1u);
}
static void stosb(uint8_t *m,pr_u6f_0482_cpu *c) {
    wb(m,c->es,c->di,(uint8_t)c->ax);c->di=(uint16_t)(c->di+1u);
}
static void rep_movsb(uint8_t *m,pr_u6f_0482_cpu *c) {
    while(c->cx!=0u) {
        uint8_t v=rb(m,c->ds,c->si);wb(m,c->es,c->di,v);
        c->si=(uint16_t)(c->si+1u);c->di=(uint16_t)(c->di+1u);--c->cx;
    }
}
static void selector(uint8_t *m,pr_u6f_0482_cpu *c) {
    push(m,c,c->bp);c->bp=c->sp;
    al(c,rb(m,c->ds,0x5376u));wb(m,c->ds,0x5377u,(uint8_t)c->ax);
    al(c,rb(m,c->ss,(uint16_t)(c->bp+6u)));wb(m,c->ds,0x5376u,(uint8_t)c->ax);
    pop_bp(m,c);retf(m,c,2u);
}
static void strload(uint8_t *m,pr_u6f_0482_cpu *c) {
    uint16_t off,seg;c->flags&=(uint16_t)~0x400u;c->bx=c->sp;c->dx=c->ds;
    les(m,c,c->ss,(uint16_t)(c->bx+8u));
    far_read(m,c->ss,(uint16_t)(c->bx+4u),&off,&seg);c->si=off;c->ds=seg;
    lodsb(m,c);stosb(m,c);cl(c,(uint8_t)c->ax);xor_high_zero(c,1);
    rep_movsb(m,c);c->ds=c->dx;retf(m,c,4u);
}
static void charstr(uint8_t *m,pr_u6f_0482_cpu *c) {
    c->flags&=(uint16_t)~0x400u;c->bx=c->sp;
    les(m,c,c->ss,(uint16_t)(c->bx+6u));al(c,1u);stosb(m,c);
    al(c,rb(m,c->ss,(uint16_t)(c->bx+4u)));stosb(m,c);retf(m,c,2u);
}
static void strcat16(uint8_t *m,pr_u6f_0482_cpu *c) {
    uint16_t off,seg,old,carry;uint8_t a,b;uint32_t sum;
    c->flags&=(uint16_t)~0x400u;c->bx=c->sp;c->dx=c->ds;
    les(m,c,c->ss,(uint16_t)(c->bx+8u));
    far_read(m,c->ss,(uint16_t)(c->bx+4u),&off,&seg);c->si=off;c->ds=seg;
    cl(c,rb(m,c->es,c->di));xor_high_zero(c,1);lodsb(m,c);
    a=rb(m,c->es,c->di);b=(uint8_t)c->ax;sum=(uint32_t)a+b;
    c->flags=flags_result(c->flags,a,b,sum,8u,0);wb(m,c->es,c->di,(uint8_t)sum);
    if(c->flags&1u) { wb(m,c->es,c->di,255u);al(c,(uint8_t)c->cx);al(c,(uint8_t)~c->ax); }
    old=c->di;sum=(uint32_t)old+c->cx;c->di=(uint16_t)sum;
    c->flags=flags_result(c->flags,old,c->cx,sum,16u,0);
    old=c->di;carry=c->flags&1u;c->di=(uint16_t)(old+1u);
    c->flags=(uint16_t)((flags_result(c->flags,old,1u,(uint32_t)old+1u,16u,0)&~1u)|carry);
    cl(c,(uint8_t)c->ax);rep_movsb(m,c);c->ds=c->dx;retf(m,c,4u);
}
static int closed_call(uint8_t *m,pr_u6f_0482_cpu *c,uint16_t site,
                        void (*body)(uint8_t *,pr_u6f_0482_cpu *)) {
    uint16_t cs=c->cs,ip=(uint16_t)(site+5u);
    uint16_t target_ip=rw(m,cs,(uint16_t)(site+1u));
    uint16_t target_cs=rw(m,cs,(uint16_t)(site+3u));
    push(m,c,cs);push(m,c,ip);c->cs=target_cs;c->ip=target_ip;body(m,c);
    return c->cs==cs && c->ip==ip;
}
pr_u6f_0482_status pr_u6f_prefix_0482(uint8_t *m,size_t size,
                                     pr_u6f_0482_cpu *c,
                                     pr_u6f_0482_pending_call *pending) {
    uint16_t old;
    if(!m || size<PR_U6F_0482_RAM_SIZE || !c || !pending || c->ip!=0x482u)
        return PR_U6F_0482_ARGUMENT;
    push(m,c,c->bp);c->bp=c->sp;old=c->sp;c->sp=(uint16_t)(old-0x22au);
    c->flags=flags_result(c->flags,old,0x22au,(uint32_t)old-0x22au,16u,1);
    al(c,3u);push(m,c,c->ax);
    if(!closed_call(m,c,0x48cu,selector))return PR_U6F_0482_CONTROL_STOP;
    les(m,c,c->ds,0x5d92u);
    ww(m,c->ss,(uint16_t)(c->bp-0x29u),c->di);
    ww(m,c->ss,(uint16_t)(c->bp-0x27u),c->es);
    c->di=(uint16_t)(c->bp-0x229u);push(m,c,c->ss);push(m,c,c->di);
    c->di=0x476u;push(m,c,c->cs);push(m,c,c->di);
    if(!closed_call(m,c,0x4a6u,strload))return PR_U6F_0482_CONTROL_STOP;
    c->di=(uint16_t)(c->bp-0x129u);push(m,c,c->ss);push(m,c,c->di);
    les(m,c,c->ss,(uint16_t)(c->bp-0x29u));
    al(c,rb(m,c->es,(uint16_t)(c->di+0xc0u)));xor_high_zero(c,0);
    c->di=c->ax;al(c,rb(m,c->ds,(uint16_t)(c->di+0x8c5u)));push(m,c,c->ax);
    if(!closed_call(m,c,0x4c2u,charstr))return PR_U6F_0482_CONTROL_STOP;
    if(!closed_call(m,c,0x4c7u,strcat16))return PR_U6F_0482_CONTROL_STOP;
    les(m,c,c->ss,(uint16_t)(c->bp-0x29u));
    al(c,rb(m,c->es,(uint16_t)(c->di+0xbdu)));push(m,c,c->ax);
    al(c,11u);push(m,c,c->ax);c->ip=0x4d8u;
    pending->call_ip=c->ip;pending->continuation_ip=0x4ddu;
    pending->target_ip=rw(m,c->cs,0x4d9u);pending->target_cs=rw(m,c->cs,0x4dbu);
    return PR_U6F_0482_CALLBACK;
}
