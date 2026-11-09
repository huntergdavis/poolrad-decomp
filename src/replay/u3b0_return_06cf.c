#include "poolrad/replay/u3b0_return_06cf.h"
#define MASK (PR_R0899_KEY_RAM_SIZE-1u)
static size_t addr(uint16_t s,uint16_t o) { return (((size_t)s<<4u)+o)&MASK; }
static uint16_t word(const uint8_t *m,uint16_t s,uint16_t o) {
    size_t a=addr(s,o);return (uint16_t)(m[a]|((uint16_t)m[(a+1u)&MASK]<<8u));
}
static void zero_word(uint8_t *m,uint16_t s,uint16_t o) {
    size_t a=addr(s,o);m[a]=0;m[(a+1u)&MASK]=0;
}
static void flags8(pr_r0899_cpu *c,uint8_t v) {
    unsigned p=v;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags & (uint16_t)~0x08d5u)|(v==0 ? 0x40u : 0u)|
                        (v&0x80u)|((p&1u)==0 ? 4u : 0u));
}
pr_u3b0_return_status pr_u3b0_return_06cf(uint8_t *m,size_t n,
    pr_r0899_cpu *c,const pr_u3b0_return_target *t) {
    uint16_t off,seg,ip,cs; uint8_t v;
    if(!m || !c || !t || n!=PR_R0899_KEY_RAM_SIZE || c->ip!=0x06cfu ||
       c->bp<0x0063u || c->bp>0xffe3u) return PR_U3B0_RETURN_ARGUMENT;
    v=m[addr(c->ss,(uint16_t)(c->bp-0x63u))]; flags8(c,v);
    if(v==0u) { c->ip=0x048eu; return PR_U3B0_RETURN_LOOP; }
    off=word(m,c->ds,0x49d2u);seg=word(m,c->ds,0x49d4u); /* LES DI,[49D2] */
    c->di=off;c->es=seg;c->ax=0u;flags8(c,0u); /* XOR AX,AX */
    zero_word(m,c->es,(uint16_t)(c->di+0x03feu));
    c->ax=(uint16_t)((c->ax&0xff00u)|m[addr(c->ss,(uint16_t)(c->bp-0x60u))]);
    m[addr(c->ss,(uint16_t)(c->bp-1u))]=(uint8_t)c->ax;
    off=word(m,c->ss,(uint16_t)(c->bp+6u));seg=word(m,c->ss,(uint16_t)(c->bp+8u));
    c->di=off;c->es=seg;
    c->ax=(uint16_t)((c->ax&0xff00u)|m[addr(c->es,c->di)]);
    m[addr(c->ds,0x6b2cu)]=(uint8_t)c->ax;
    c->ax=(uint16_t)((c->ax&0xff00u)|m[addr(c->ss,(uint16_t)(c->bp-1u))]);
    c->sp=c->bp;c->bp=word(m,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);c->ip=0x06f8u;
    ip=word(m,c->ss,c->sp);cs=word(m,c->ss,(uint16_t)(c->sp+2u));
    if(ip!=t->ip || cs!=t->cs) return PR_U3B0_RETURN_TRANSFER;
    c->ip=ip;c->cs=cs;c->sp=(uint16_t)(c->sp+0x1au);
    return PR_U3B0_RETURN_DONE;
}
