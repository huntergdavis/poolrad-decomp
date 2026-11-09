#include "poolrad/replay/u3b0_poll_0566.h"
#include <string.h>

#define MASK (PR_R0899_KEY_RAM_SIZE - 1u)
#define ARITH 0x08d5u
static size_t addr(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t byte(const uint8_t *m, uint16_t s, uint16_t o) { return m[addr(s,o)]; }
static uint16_t word(const uint8_t *m, uint16_t s, uint16_t o) {
    size_t a=addr(s,o); return (uint16_t)(m[a] | ((uint16_t)m[(a+1u)&MASK] << 8u));
}
static void putword(uint8_t *m, uint16_t s, uint16_t o, uint16_t v) {
    size_t a=addr(s,o); m[a]=(uint8_t)v; m[(a+1u)&MASK]=(uint8_t)(v>>8u);
}
static void push(uint8_t *m, pr_r0899_cpu *c, uint16_t v) { c->sp=(uint16_t)(c->sp-2u); putword(m,c->ss,c->sp,v); }
/* OR AL,AL and CMP r/m8,0 share these modeled flags. */
static void flags8(pr_r0899_cpu *c, uint8_t v) {
    unsigned p=v; p^=p>>4; p^=p>>2; p^=p>>1;
    c->flags=(uint16_t)((c->flags & (uint16_t)~ARITH) | (v==0 ? 0x40u : 0u) |
                        (v&0x80u) | ((p&1u)==0 ? 4u : 0u));
}
#define FIELDS(X) X(ax) X(bx) X(cx) X(dx) X(si) X(di) X(bp) X(sp) X(cs) X(ip) X(ds) X(es) X(ss) X(flags)
#define TO_CRT(f) t.f=c->f;
#define FROM_CRT(f) c->f=t.f;
static int target(const uint8_t *m, const pr_r0899_cpu *c, uint16_t site, uint16_t off, uint16_t seg) {
    return byte(m,c->cs,site)==0x9au && word(m,c->cs,(uint16_t)(site+1u))==off &&
           word(m,c->cs,(uint16_t)(site+3u))==seg;
}
static void call(uint8_t *m, pr_r0899_cpu *c, uint16_t seg, uint16_t off, uint16_t next) {
    push(m,c,c->cs); push(m,c,next); c->cs=seg; c->ip=off;
}
static pr_u3b0_poll_status key(uint8_t *m, size_t n, pr_r0899_cpu *c,
    const pr_u3b0_poll_env *e, pr_u3b0_poll_diagnostic *d, uint16_t site, uint16_t next) {
    uint16_t caller=c->cs, sp=c->sp;
    d->site=site;
    if (!target(m,c,site,0x0059u,e->key_segment)) return PR_U3B0_POLL_CALL_TARGET;
    if (c->sp < 0x0116u || c->sp > 0xfffeu) return PR_U3B0_POLL_ARGUMENT;
    call(m,c,e->key_segment,0x0059u,next);
    d->key_status=pr_r0899_readkey_0059_cpu(m,n,c,e->key_env,&d->key);
    if (d->key_status!=PR_R0899_KEY_RETURN) return PR_U3B0_POLL_KEY_STOP;
    if (c->cs!=caller || c->ip!=next || c->sp!=sp) return PR_U3B0_POLL_TRANSFER;
    return PR_U3B0_POLL_EXTENDED; /* internal continuation */
}
pr_u3b0_poll_status pr_u3b0_poll_0566(uint8_t *m, size_t n, pr_r0899_cpu *c,
    const pr_u3b0_poll_env *e, pr_u3b0_poll_diagnostic *d) {
    pr_crt_cpu t; pr_u3b0_poll_status s; uint16_t caller, sp, off, seg;
    if (!m || !c || !e || !e->key_env || !e->key_env->keyboard || !d ||
        n!=PR_R0899_KEY_RAM_SIZE || c->ip!=0x0566u || c->sp<0x0116u ||
        c->sp>0xfffeu || c->bp<0x0063u || c->bp>0xfff1u) return PR_U3B0_POLL_ARGUMENT;
    memset(d,0,sizeof *d); d->site=0x0566u;
    if (!target(m,c,0x0566u,0x02fdu,e->crt_segment)) return PR_U3B0_POLL_CALL_TARGET;
    caller=c->cs; sp=c->sp; call(m,c,e->crt_segment,0x02fdu,0x056bu);
    FIELDS(TO_CRT)
    d->crt_status=pr_crt_keypressed_02fd(m,n,&t,e->key_env->keyboard,e->key_env->keyboard_context);
    FIELDS(FROM_CRT)
    if (d->crt_status!=PR_CRT_RETURNED) return PR_U3B0_POLL_CRT_STOP;
    if (c->cs!=caller || c->ip!=0x056bu || c->sp!=sp) return PR_U3B0_POLL_TRANSFER;
    flags8(c,(uint8_t)c->ax);
    if ((uint8_t)c->ax==0u) { c->ip=0x06cfu; return PR_U3B0_POLL_NO_EVENT; }
    c->ip=0x0572u; s=key(m,n,c,e,d,0x0572u,0x0577u); if (s!=PR_U3B0_POLL_EXTENDED) return s;
    m[addr(c->ss,(uint16_t)(c->bp-0x60u))]=(uint8_t)c->ax;
    flags8(c,byte(m,c->ss,(uint16_t)(c->bp-0x60u)));
    if (byte(m,c->ss,(uint16_t)(c->bp-0x60u))!=0u) { c->ip=0x059cu; return PR_U3B0_POLL_NON_EXTENDED; }
    c->ip=0x0580u; s=key(m,n,c,e,d,0x0580u,0x0585u); if (s!=PR_U3B0_POLL_EXTENDED) return s;
    m[addr(c->ss,(uint16_t)(c->bp-0x60u))]=(uint8_t)c->ax;
    flags8(c,byte(m,c->ss,(uint16_t)(c->bp+0x0cu)));
    if (byte(m,c->ss,(uint16_t)(c->bp+0x0cu))==0u) { c->ip=0x06cfu; return PR_U3B0_POLL_EXTENDED_DISABLED; }
    off=word(m,c->ss,(uint16_t)(c->bp+6u)); seg=word(m,c->ss,(uint16_t)(c->bp+8u));
    c->di=off; c->es=seg;
    m[addr(seg,off)]=1u;
    m[addr(c->ss,(uint16_t)(c->bp-0x63u))]=1u;
    c->ip=0x06cfu; return PR_U3B0_POLL_EXTENDED;
}
